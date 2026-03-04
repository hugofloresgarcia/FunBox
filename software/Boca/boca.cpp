#include "daisy_petal.h"
#include "daisysp.h"
#include "funbox.h"

#include "envelope_filter.h"
#include "formant_filter.h"
#include "xfade.h"

using namespace daisy;
using namespace daisysp;
using namespace funbox;

#ifdef SIM_BUILD
#include <cstdio>
#include <cmath>
#include <cstdlib>

struct SignalProbe {
    const char* name;
    float peak = 0.f;
    double sum_sq = 0.0;
    size_t count = 0;
    size_t soft_clip_count = 0;
    size_t hard_clip_count = 0;

    void tap(float x) {
        float a = fabsf(x);
        if (a > peak) peak = a;
        sum_sq += (double)x * x;
        count++;
        if (a > 0.95f) soft_clip_count++;
        if (a > 1.0f) hard_clip_count++;
    }

    void report() const {
        float rms = (count > 0) ? sqrtf((float)(sum_sq / count)) : 0.f;
        float peak_db = (peak > 0.f) ? 20.f * log10f(peak) : -120.f;
        float rms_db  = (rms > 0.f) ? 20.f * log10f(rms) : -120.f;
        fprintf(stderr, "  %-14s peak=%7.4f (%+6.1fdB)  rms=%7.4f (%+6.1fdB)  "
                        "soft_clip=%zu  hard_clip=%zu  (n=%zu)\n",
                name, peak, peak_db, rms, rms_db,
                soft_clip_count, hard_clip_count, count);
    }
};

static SignalProbe p_input{"input"};
static SignalProbe p_env_mod{"env_mod"};
static SignalProbe p_filtered{"filtered"};
static SignalProbe p_output{"output"};

static void print_probes() {
    fprintf(stderr, "\n=== Signal Probe Report ===\n");
    p_input.report();
    p_env_mod.report();
    p_filtered.report();
    p_output.report();
    fprintf(stderr, "===========================\n");
}

static struct ProbeInit {
    ProbeInit() { atexit(print_probes); }
} _probe_init;

#define SIM_PROBE(probe, val) probe.tap(val)
#else
#define SIM_PROBE(probe, val)
#endif

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// CONSTANTS
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

static constexpr float kMomentaryFswTimeMs = 300.0f;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// GLOBALS — HARDWARE
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

DaisyPetal hw;

Led led1, led2;

Parameter knob1,  // sensitivity
          knob2,  // range
          knob3,  // resonance
          knob4,  // LFO rate
          knob5,  // LFO depth
          knob6;  // mix

bool     pswitch1[2], pswitch2[2], pswitch3[2], pdip[4];
int      switch1[2], switch2[2], switch3[2], dip[4];

// Switch-derived state
bool formant_mode = false;

EnvelopeFilterEngine::FilterType filter_type = EnvelopeFilterEngine::FilterType::BP;
EnvelopeFilterEngine::Direction  direction   = EnvelopeFilterEngine::Direction::UP;
EnvelopeFilterEngine::Response   response    = EnvelopeFilterEngine::Response::MEDIUM;

FormantFilter::VowelPair vowel_pair = FormantFilter::VowelPair::OO_AH;

struct FswState {
    bool  state     = false;
    bool  momentary = false;
    bool  pressed   = false;
    bool  rising    = false;
    bool  falling   = false;
    float time_held = 0.0f;

    inline operator bool() const { return state; }
};

FswState fsw1, fsw2;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// GLOBALS — DSP
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

EnvelopeFilterEngine env_filter;
FormantFilter        formant_filter;

daisysp::Line bypass_ramp;
float         ramp_time_ms = 25.0f;

Xfade xfade;

bool prev_bypass_state = false;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// SWITCH CALLBACKS
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void updateSwitch1()
{
    if (formant_mode) {
        // Formant mode: vowel pair selection
        if (pswitch1[0]) {
            vowel_pair = FormantFilter::VowelPair::AH_EE;
        } else if (pswitch1[1]) {
            vowel_pair = FormantFilter::VowelPair::OO_AH;
        } else {
            vowel_pair = FormantFilter::VowelPair::OH_EE;
        }
        formant_filter.SetVowelPair(vowel_pair);
    } else {
        // Standard mode: LP / BP / HP
        if (pswitch1[0]) {
            filter_type = EnvelopeFilterEngine::FilterType::LP;
        } else if (pswitch1[1]) {
            filter_type = EnvelopeFilterEngine::FilterType::HP;
        } else {
            filter_type = EnvelopeFilterEngine::FilterType::BP;
        }
        env_filter.SetFilterType(filter_type);
    }
}

void updateSwitch2()
{
    if (pswitch2[0]) {
        // Left: Env Down
        formant_mode = false;
        direction = EnvelopeFilterEngine::Direction::DOWN;
        env_filter.SetDirection(direction);
    } else if (pswitch2[1]) {
        // Right: Formant mode
        formant_mode = true;
    } else {
        // Center: Env Up
        formant_mode = false;
        direction = EnvelopeFilterEngine::Direction::UP;
        env_filter.SetDirection(direction);
    }
    // Re-evaluate switch 1 since its meaning depends on mode
    updateSwitch1();
}

void updateSwitch3()
{
    if (pswitch3[0]) {
        response = EnvelopeFilterEngine::Response::FAST;
    } else if (pswitch3[1]) {
        response = EnvelopeFilterEngine::Response::SLOW;
    } else {
        response = EnvelopeFilterEngine::Response::MEDIUM;
    }
    env_filter.SetResponse(response);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// HARDWARE PROCESSING
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void UpdateButtons()
{
    fsw1.pressed   = hw.switches[Funbox::FOOTSWITCH_1].Pressed();
    fsw2.pressed   = hw.switches[Funbox::FOOTSWITCH_2].Pressed();
    fsw1.rising    = hw.switches[Funbox::FOOTSWITCH_1].RisingEdge();
    fsw2.rising    = hw.switches[Funbox::FOOTSWITCH_2].RisingEdge();
    fsw1.falling   = hw.switches[Funbox::FOOTSWITCH_1].FallingEdge();
    fsw2.falling   = hw.switches[Funbox::FOOTSWITCH_2].FallingEdge();
    fsw1.time_held = hw.switches[Funbox::FOOTSWITCH_1].TimeHeldMs();
    fsw2.time_held = hw.switches[Funbox::FOOTSWITCH_2].TimeHeldMs();

    // FSW1: bypass toggle (with momentary)
    if (fsw1.rising) {
        fsw1.state = !fsw1.state;
    }
    if (fsw1.pressed && fsw1.time_held > kMomentaryFswTimeMs) {
        fsw1.momentary = true;
    } else if (fsw1.falling && fsw1.momentary) {
        fsw1.momentary = false;
        fsw1.state = false;
    }

    // FSW2: freeze toggle (with momentary)
    if (fsw2.rising) {
        fsw2.state = !fsw2.state;
    }
    if (fsw2.pressed && fsw2.time_held > kMomentaryFswTimeMs) {
        fsw2.momentary = true;
    } else if (fsw2.falling && fsw2.momentary) {
        fsw2.momentary = false;
        fsw2.state = false;
    }

    led1.Set(fsw1.state ? 1.0f : 0.0f);
    led2.Set(fsw2.state ? 1.0f : 0.0f);
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
    if (changed1) updateSwitch1();

    bool changed2 = false;
    for (int i = 0; i < 2; i++) {
        if (hw.switches[switch2[i]].Pressed() != pswitch2[i]) {
            pswitch2[i] = hw.switches[switch2[i]].Pressed();
            changed2 = true;
        }
    }
    if (changed2) updateSwitch2();

    bool changed3 = false;
    for (int i = 0; i < 2; i++) {
        if (hw.switches[switch3[i]].Pressed() != pswitch3[i]) {
            pswitch3[i] = hw.switches[switch3[i]].Pressed();
            changed3 = true;
        }
    }
    if (changed3) updateSwitch3();

    for (int i = 0; i < 4; i++) {
        if (hw.switches[dip[i]].Pressed() != pdip[i]) {
            pdip[i] = hw.switches[dip[i]].Pressed();
        }
    }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// AUDIO CALLBACK
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

static void AudioCallback(AudioHandle::InputBuffer  in,
                           AudioHandle::OutputBuffer out,
                           size_t                    size)
{
    hw.ProcessAnalogControls();
    hw.ProcessDigitalControls();

    UpdateButtons();
    UpdateSwitches();

    // Read knobs once per block
    float vSens  = knob1.Process();
    float vRange = knob2.Process();
    float vReso  = knob3.Process();
    float vRate  = knob4.Process();
    float vDepth = knob5.Process();
    knob6.Process();

    // DIP switches
    bool drive_on     = pdip[0];
    bool bass_recover = pdip[1];
    bool lfo_triangle = pdip[2];

    // Envelope filter parameters
    env_filter.SetSensitivity(vSens);
    env_filter.SetRange(vRange);
    env_filter.SetResonance(vReso);
    env_filter.SetLfoRate(vRate * 15.f);
    env_filter.SetLfoDepth(vDepth);
    env_filter.SetLfoWave(lfo_triangle
        ? EnvelopeFilterEngine::LfoWave::TRIANGLE
        : EnvelopeFilterEngine::LfoWave::SINE);
    env_filter.SetDrive(drive_on);
    env_filter.SetBassRecovery(bass_recover);
    env_filter.SetFreeze(fsw2.state);

    // Formant filter parameters
    formant_filter.SetResonance(vReso);
    formant_filter.SetDrive(drive_on);
    formant_filter.SetBassRecovery(bass_recover);

    // Xfade (active when bypass is engaged)
    xfade.SetCrossfade(fsw1.state ? knob6.Value() : 0.0f);

    // Bypass ramp on state transitions
    bool new_bypass_state = fsw1.state;
    if (new_bypass_state != prev_bypass_state) {
        bypass_ramp.Start(
            (uint8_t)prev_bypass_state,
            (uint8_t)new_bypass_state,
            ramp_time_ms * 0.001f
        );
        prev_bypass_state = new_bypass_state;
    }

    uint8_t ramp_finished = 0;
    for (size_t i = 0; i < size; i++)
    {
        float sig = in[0][i];
        SIM_PROBE(p_input, sig);

        float ramp_val = bypass_ramp.Process(&ramp_finished);

        float filtered;
        if (formant_mode) {
            float mod = env_filter.ProcessMod(sig);
            SIM_PROBE(p_env_mod, mod);
            filtered = formant_filter.Process(sig, mod);
        } else {
            filtered = env_filter.Process(sig);
            SIM_PROBE(p_env_mod, env_filter.GetModValue());
        }

        SIM_PROBE(p_filtered, filtered);

        float wet = filtered * ramp_val;
        sig = xfade.Process(sig, wet);

        float result = SoftClip(sig);
        SIM_PROBE(p_output, result);

        out[0][i] = result;
        out[1][i] = result;
    }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// MAIN
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

int main(void)
{
    hw.Init();
    float sr = hw.AudioSampleRate();

    hw.SetAudioBlockSize(4);

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
        pswitch1[i] = false;
        pswitch2[i] = false;
        pswitch3[i] = false;
    }
    for (int i = 0; i < 4; i++) {
        pdip[i] = false;
    }

    knob1.Init(hw.knob[Funbox::KNOB_1], 0.0f, 1.0f, Parameter::LINEAR);       // sensitivity
    knob2.Init(hw.knob[Funbox::KNOB_2], 0.0f, 1.0f, Parameter::LINEAR);       // range
    knob3.Init(hw.knob[Funbox::KNOB_3], 0.0f, 1.0f, Parameter::LINEAR);       // resonance
    knob4.Init(hw.knob[Funbox::KNOB_4], 0.0f, 1.0f, Parameter::EXPONENTIAL);  // LFO rate
    knob5.Init(hw.knob[Funbox::KNOB_5], 0.0f, 1.0f, Parameter::LINEAR);       // LFO depth
    knob6.Init(hw.knob[Funbox::KNOB_6], 0.0f, 1.0f, Parameter::LINEAR);       // mix

    // LEDs
    led1.Init(hw.seed.GetPin(Funbox::LED_1), false);
    led1.Update();
    led2.Init(hw.seed.GetPin(Funbox::LED_2), false);
    led2.Update();

    // DSP engines
    env_filter.Init(sr);
    formant_filter.Init(sr);

    bypass_ramp.Init(sr);
    bypass_ramp.Start(0.0f, 0.0f, ramp_time_ms * 0.001f);

    xfade.Init(sr, 10.0f);
    xfade.SetCrossfadeType(Xfade::TYPE::ASYMMETRIC_MIX);

    hw.StartAdc();
    hw.StartAudio(AudioCallback);

    while (1)
    {
        System::Delay(10);
    }
}
