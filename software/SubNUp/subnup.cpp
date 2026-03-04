// SubNUp -- Polyphonic Octave Pedal
//
// ERB-PS2 algorithm ported from terrarium-poly-octave by Steve Schulteis
// (MIT license), implementing Etienne Thuillier's thesis algorithm
// (Aalto University, 2016, in cooperation with TC Electronic).

#include "daisy_petal.h"
#include "daisysp.h"
#include "funbox.h"

#include "octave_engine.h"
#include "multirate.h"
#include "biquad.h"
#include "mathutils.h"

using namespace daisy;
using namespace funbox;

// ────────────────────────────────────────────────────────────────────────────
// SIM PROBES
// ────────────────────────────────────────────────────────────────────────────

#ifdef SIM_BUILD
#include <cstdio>
#include <cmath>
#include <cstdlib>

struct SignalProbe {
    const char* name;
    float peak = 0.f;
    double sum_sq = 0.0;
    size_t count = 0;
    size_t clip_count = 0;

    void tap(float x) {
        float a = fabsf(x);
        if (a > peak) peak = a;
        sum_sq += (double)x * x;
        count++;
        if (a > 0.95f) clip_count++;
    }

    void report() const {
        float rms = (count > 0) ? sqrtf((float)(sum_sq / count)) : 0.f;
        float peak_db = (peak > 0.f) ? 20.f * log10f(peak) : -120.f;
        float rms_db  = (rms > 0.f) ? 20.f * log10f(rms) : -120.f;
        fprintf(stderr, "  %-14s peak=%7.4f (%+6.1fdB)  rms=%7.4f (%+6.1fdB)  "
                        "clip=%zu  (n=%zu)\n",
                name, peak, peak_db, rms, rms_db, clip_count, count);
    }
};

static SignalProbe p_input{"input"};
static SignalProbe p_oct_up{"oct_up"};
static SignalProbe p_oct_down{"oct_down"};
static SignalProbe p_oct_down2{"oct_down2"};
static SignalProbe p_wet{"wet"};
static SignalProbe p_output{"output"};

static void print_probes() {
    fprintf(stderr, "\n=== SubNUp Signal Probe Report ===\n");
    p_input.report();
    p_oct_up.report();
    p_oct_down.report();
    p_oct_down2.report();
    p_wet.report();
    p_output.report();
    fprintf(stderr, "==================================\n");
}

static struct ProbeInit {
    ProbeInit() { atexit(print_probes); }
} _probe_init;

#define SIM_PROBE(probe, val) probe.tap(val)
#else
#define SIM_PROBE(probe, val)
#endif

// ────────────────────────────────────────────────────────────────────────────
// CONSTANTS
// ────────────────────────────────────────────────────────────────────────────

static constexpr float kSmoothCoeff     = 0.0007f;
static constexpr float kMomentaryTimeMs = 300.0f;
static constexpr float kBypassRampMs    = 20.0f;
static constexpr float kToneMinHz       = 500.f;
static constexpr float kToneMaxHz       = 8000.f;

// ────────────────────────────────────────────────────────────────────────────
// GLOBALS - HARDWARE
// ────────────────────────────────────────────────────────────────────────────

DaisyPetal hw;

Led led1, led2;

Parameter knob_dry_p,
          knob_up_p,
          knob_sub_p,
          knob_sub2_p,
          knob_tone_p,
          knob_output_p;

bool pswitch1[2], pswitch2[2], pswitch3[2], pdip[4];
int  switch1[2], switch2[2], switch3[2], dip[4];

// ────────────────────────────────────────────────────────────────────────────
// GLOBALS - STATE
// ────────────────────────────────────────────────────────────────────────────

bool  bypass        = true;
bool  prev_bypass   = true;
bool  trails        = false;
bool  noise_gate_on = false;
bool  first_start   = true;

// Smoothed parameter values
float cur_dry_level    = 0.f;
float cur_up_level     = 0.f;
float cur_sub_level    = 0.f;
float cur_sub2_level   = 0.f;
float cur_tone_freq    = kToneMaxHz;
float cur_output_level = 0.f;

// Target values
float tgt_dry_level    = 0.f;
float tgt_up_level     = 0.f;
float tgt_sub_level    = 0.f;
float tgt_sub2_level   = 0.f;
float tgt_tone_freq    = kToneMaxHz;
float tgt_output_level = 0.f;

// Momentary sub boost (FSW2)
bool  fsw2_held       = false;
bool  fsw2_momentary  = false;
float fsw2_time_held  = 0.f;

// Saved levels for momentary restore
float saved_dry_level  = 0.f;
float saved_up_level   = 0.f;
float saved_sub_level  = 0.f;
float saved_sub2_level = 0.f;

// Sub character mode (Switch 2)
enum SubCharacter { SUB_DEEP, SUB_NORMAL, SUB_TIGHT };
SubCharacter sub_character = SUB_NORMAL;

// ────────────────────────────────────────────────────────────────────────────
// GLOBALS - DSP
// ────────────────────────────────────────────────────────────────────────────

Decimator    decimator;
Interpolator interpolator;
OctaveEngine octave;

Biquad eq_highshelf;
Biquad eq_lowshelf;
Biquad tone_filter;
Biquad sub_hp;

daisysp::Line bypass_ramp;

// Noise gate envelope follower
float gate_env = 0.f;
static constexpr float kGateThreshold = 0.001f; // ~-60 dB
static constexpr float kGateAttack    = 0.01f;
static constexpr float kGateRelease   = 0.0001f;

// ────────────────────────────────────────────────────────────────────────────
// SWITCH CALLBACKS
// ────────────────────────────────────────────────────────────────────────────

void updateSwitch1() // Voice presets
{
    // Handled in audio callback via target levels -- switch just gates which
    // knobs feed into targets. All voices always available via knobs;
    // switch provides quick presets by zeroing disabled voices.
    // Left: UP+SUB+SUB2 (all), Center: UP+SUB, Right: SUB+SUB2
}

void updateSwitch2() // Sub character
{
    if (pswitch2[0]) {
        sub_character = SUB_DEEP;
    } else if (pswitch2[1]) {
        sub_character = SUB_TIGHT;
        sub_hp.SetHighpass(80.f);
    } else {
        sub_character = SUB_NORMAL;
    }
}

void updateSwitch3() // Reserved
{
}

// ────────────────────────────────────────────────────────────────────────────
// HARDWARE PROCESSING
// ────────────────────────────────────────────────────────────────────────────

void UpdateButtons()
{
    // FSW1: bypass toggle (rising edge = foot press)
    if (hw.switches[Funbox::FOOTSWITCH_1].RisingEdge())
    {
        bypass = !bypass;
        led1.Set(bypass ? 0.0f : 1.0f);
    }

    // FSW2: momentary sub boost
    fsw2_time_held = hw.switches[Funbox::FOOTSWITCH_2].TimeHeldMs();

    if (hw.switches[Funbox::FOOTSWITCH_2].RisingEdge() && !fsw2_held)
    {
        fsw2_held = true;
        fsw2_momentary = false;
    }

    if (fsw2_held && fsw2_time_held > kMomentaryTimeMs && !fsw2_momentary)
    {
        fsw2_momentary = true;
        saved_dry_level  = tgt_dry_level;
        saved_up_level   = tgt_up_level;
        saved_sub_level  = tgt_sub_level;
        saved_sub2_level = tgt_sub2_level;
    }

    if (hw.switches[Funbox::FOOTSWITCH_2].FallingEdge() && fsw2_held)
    {
        fsw2_held = false;
        fsw2_momentary = false;
    }

    led2.Set(fsw2_momentary ? 1.0f : 0.0f);
    led1.Update();
    led2.Update();
}

void UpdateSwitches()
{
    bool changed1 = false;
    for (int i = 0; i < 2; i++) {
        if (hw.switches[switch1[i]].Pressed() != pswitch1[i]) {
            pswitch1[i] = hw.switches[switch1[i]].Pressed();
            changed1 = true;
        }
    }
    if (changed1 || first_start) updateSwitch1();

    bool changed2 = false;
    for (int i = 0; i < 2; i++) {
        if (hw.switches[switch2[i]].Pressed() != pswitch2[i]) {
            pswitch2[i] = hw.switches[switch2[i]].Pressed();
            changed2 = true;
        }
    }
    if (changed2 || first_start) updateSwitch2();

    bool changed3 = false;
    for (int i = 0; i < 2; i++) {
        if (hw.switches[switch3[i]].Pressed() != pswitch3[i]) {
            pswitch3[i] = hw.switches[switch3[i]].Pressed();
            changed3 = true;
        }
    }
    if (changed3 || first_start) updateSwitch3();

    for (int i = 0; i < 4; i++) {
        if (hw.switches[dip[i]].Pressed() != pdip[i]) {
            pdip[i] = hw.switches[dip[i]].Pressed();
        }
    }

    noise_gate_on = pdip[0];
    trails        = pdip[1];

    first_start = false;
}

// ────────────────────────────────────────────────────────────────────────────
// AUDIO CALLBACK
// ────────────────────────────────────────────────────────────────────────────

static void AudioCallback(AudioHandle::InputBuffer  in,
                           AudioHandle::OutputBuffer out,
                           size_t                    size)
{
    hw.ProcessAnalogControls();
    hw.ProcessDigitalControls();

    UpdateButtons();
    UpdateSwitches();

    // ~~~~~ Read knobs once per block ~~~~~

    float raw_dry  = knob_dry_p.Process();
    float raw_up   = knob_up_p.Process();
    float raw_sub  = knob_sub_p.Process();
    float raw_sub2 = knob_sub2_p.Process();
    float raw_tone = knob_tone_p.Process();
    float raw_out  = knob_output_p.Process();

    // Switch 1: voice presets (gate knobs to zero for disabled voices)
    bool enable_up   = true;
    bool enable_sub  = true;
    bool enable_sub2 = true;

    if (pswitch1[0]) {
        // Left: all voices enabled
    } else if (pswitch1[1]) {
        // Right: SUB+SUB2 only
        enable_up = false;
    } else {
        // Center: UP+SUB only
        enable_sub2 = false;
    }

    // Set target levels (momentary override if active)
    if (fsw2_momentary)
    {
        tgt_dry_level  = 0.f;
        tgt_up_level   = 0.f;
        tgt_sub_level  = 1.f;
        tgt_sub2_level = 1.f;
    }
    else
    {
        tgt_dry_level  = raw_dry;
        tgt_up_level   = enable_up   ? raw_up   : 0.f;
        tgt_sub_level  = enable_sub  ? raw_sub  : 0.f;
        tgt_sub2_level = enable_sub2 ? raw_sub2 : 0.f;
    }

    tgt_tone_freq    = linexp(raw_tone, 0.f, 1.f, kToneMinHz, kToneMaxHz);
    tgt_output_level = raw_out * 1.5f;

    // Bypass ramp on state transitions
    if (bypass != prev_bypass) {
        float from = prev_bypass ? 0.f : 1.f;
        float to   = bypass     ? 0.f : 1.f;
        bypass_ramp.Start(from, to, kBypassRampMs * 0.001f);
        prev_bypass = bypass;
    }

    // ~~~~~ Per-sample processing in 6-sample chunks ~~~~~

    uint8_t ramp_finished = 0;
    float in_chunk[kResampleFactor];
    float out_chunk[kResampleFactor];

    for (size_t i = 0; i < size; i += kResampleFactor)
    {
        // Smooth parameters once per chunk (every 6 samples)
        daisysp::fonepole(cur_dry_level,    tgt_dry_level,    kSmoothCoeff);
        daisysp::fonepole(cur_up_level,     tgt_up_level,     kSmoothCoeff);
        daisysp::fonepole(cur_sub_level,    tgt_sub_level,    kSmoothCoeff);
        daisysp::fonepole(cur_sub2_level,   tgt_sub2_level,   kSmoothCoeff);
        daisysp::fonepole(cur_tone_freq,    tgt_tone_freq,    kSmoothCoeff);
        daisysp::fonepole(cur_output_level, tgt_output_level, kSmoothCoeff);

        tone_filter.SetLowpass(cur_tone_freq);

        // Gather 6 input samples
        for (size_t j = 0; j < kResampleFactor; ++j)
        {
            float sig = in[0][i + j];
            SIM_PROBE(p_input, sig);

            // Noise gate
            if (noise_gate_on)
            {
                float abs_sig = fabsf(sig);
                float coeff = (abs_sig > gate_env) ? kGateAttack : kGateRelease;
                daisysp::fonepole(gate_env, abs_sig, coeff);
                float gate_gain = (gate_env > kGateThreshold) ? 1.f : (gate_env / kGateThreshold);
                sig *= gate_gain;
            }

            in_chunk[j] = sig;
        }

        // Decimate 6 samples → 1 sample at 8 kHz
        float decimated = decimator.Process(in_chunk);

        // Run 80-band octave engine
        octave.Process(decimated);

        // Mix octave voices at 8 kHz rate
        float up_sig   = octave.Up1()   * cur_up_level;
        float sub_sig  = octave.Down1() * cur_sub_level;
        float sub2_sig = octave.Down2() * cur_sub2_level;

        SIM_PROBE(p_oct_up,    octave.Up1());
        SIM_PROBE(p_oct_down,  octave.Down1());
        SIM_PROBE(p_oct_down2, octave.Down2());

        // Sub character filter (at 8 kHz rate, applied before mixing)
        if (sub_character == SUB_TIGHT)
        {
            sub_sig  = sub_hp.Process(sub_sig);
            sub2_sig = sub_hp.Process(sub2_sig);
        }

        float octave_mix = up_sig + sub_sig + sub2_sig;

        // Interpolate 1 sample → 6 at 48 kHz
        interpolator.Process(octave_mix, out_chunk);

        // Apply EQ + tone + mix with dry at 48 kHz
        for (size_t j = 0; j < kResampleFactor; ++j)
        {
            float wet = out_chunk[j];

            // Compensating EQ (matches terrarium-poly-octave)
            wet = eq_highshelf.Process(wet);
            wet = eq_lowshelf.Process(wet);

            // Tone control
            wet = tone_filter.Process(wet);

            SIM_PROBE(p_wet, wet);

            // Dry signal
            float dry = in_chunk[j] * cur_dry_level;

            // Bypass ramp: 0 = bypassed, 1 = active
            float ramp = bypass_ramp.Process(&ramp_finished);

            // Crossfade between clean input and processed signal
            float clean     = in_chunk[j];
            float processed = (dry + wet) * cur_output_level;
            float sig;
            if (trails)
            {
                // Trails: wet signal continues after bypass
                sig = clean * (1.f - ramp) + (dry + wet) * cur_output_level * ramp;
            }
            else
            {
                sig = clean + ramp * (processed - clean);
            }

            float result = daisysp::SoftClip(sig);
            SIM_PROBE(p_output, result);

            out[0][i + j] = result;
            out[1][i + j] = result;
        }
    }
}

// ────────────────────────────────────────────────────────────────────────────
// MAIN
// ────────────────────────────────────────────────────────────────────────────

int main(void)
{
    hw.Init();
    float sr = hw.AudioSampleRate();

    hw.SetAudioBlockSize(48);

    // 3-way switch pin mapping
    switch1[0] = Funbox::SWITCH_1_LEFT;
    switch1[1] = Funbox::SWITCH_1_RIGHT;
    switch2[0] = Funbox::SWITCH_2_LEFT;
    switch2[1] = Funbox::SWITCH_2_RIGHT;
    switch3[0] = Funbox::SWITCH_3_LEFT;
    switch3[1] = Funbox::SWITCH_3_RIGHT;
    dip[0] = Funbox::SWITCH_DIP_1;
    dip[1] = Funbox::SWITCH_DIP_2;
    dip[2] = Funbox::SWITCH_DIP_3;
    dip[3] = Funbox::SWITCH_DIP_4;

    for (int i = 0; i < 2; i++) {
        pswitch1[i] = pswitch2[i] = pswitch3[i] = false;
    }
    for (int i = 0; i < 4; i++) {
        pdip[i] = false;
    }

    // Knobs
    knob_dry_p.Init(hw.knob[Funbox::KNOB_1],  0.0f, 1.0f, Parameter::LINEAR);
    knob_up_p.Init(hw.knob[Funbox::KNOB_2],   0.0f, 1.0f, Parameter::LINEAR);
    knob_sub_p.Init(hw.knob[Funbox::KNOB_3],  0.0f, 1.0f, Parameter::LINEAR);
    knob_sub2_p.Init(hw.knob[Funbox::KNOB_4], 0.0f, 1.0f, Parameter::LINEAR);
    knob_tone_p.Init(hw.knob[Funbox::KNOB_5], 0.0f, 1.0f, Parameter::LINEAR);
    knob_output_p.Init(hw.knob[Funbox::KNOB_6], 0.0f, 1.0f, Parameter::LINEAR);

    // LEDs
    led1.Init(hw.seed.GetPin(Funbox::LED_1), false);
    led1.Update();
    led2.Init(hw.seed.GetPin(Funbox::LED_2), false);
    led2.Update();

    // DSP: octave engine runs at sr/6
    float octave_sr = sr / static_cast<float>(kResampleFactor);
    octave.Init(octave_sr);

    // EQ compensation filters (at 48 kHz, applied to interpolated output)
    eq_highshelf.Init(sr);
    eq_highshelf.SetHighShelf(140.f, -11.f);

    eq_lowshelf.Init(sr);
    eq_lowshelf.SetLowShelf(160.f, 5.f);

    // Tone filter (at 48 kHz)
    tone_filter.Init(sr);
    tone_filter.SetLowpass(kToneMaxHz);

    // Sub character highpass (at 8 kHz -- runs on decimated signal)
    sub_hp.Init(octave_sr);
    sub_hp.SetHighpass(80.f);

    // Bypass ramp
    bypass_ramp.Init(sr);
    bypass_ramp.Start(0.f, 0.f, kBypassRampMs * 0.001f);

    hw.StartAdc();
    hw.StartAudio(AudioCallback);

    while (1)
    {
        System::Delay(10);
    }
}
