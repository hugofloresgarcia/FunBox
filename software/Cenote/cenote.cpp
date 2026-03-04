#include "daisy_petal.h"
#include "daisysp.h"
#include "funbox.h"

#include "cenote_delay.h"
#include "vibrato.h"
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
static SignalProbe p_post_vib{"post_vibrato"};
static SignalProbe p_delay_in{"delay_in"};
static SignalProbe p_delay_out{"delay_out"};
static SignalProbe p_pre_clip{"pre_clip"};
static SignalProbe p_output{"output"};

static void print_probes() {
    fprintf(stderr, "\n=== Signal Probe Report ===\n");
    p_input.report();
    p_post_vib.report();
    p_delay_in.report();
    p_delay_out.report();
    p_pre_clip.report();
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

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// CONSTANTS
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

static constexpr float kMomentaryFswTimeMs = 300.0f;
static constexpr float kShiftMaxLarge  = 150.0f;
static constexpr float kShiftMaxSmall  = 15.0f;
static constexpr float kShiftMaxMedium = 50.0f;

#define MAX_DELAY_MS_LARGE  1500.0f
#define MAX_DELAY_MS_SMALL  112.5f
#define MAX_DELAY_MS_MEDIUM 500.0f

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// GLOBALS - HARDWARE
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

DaisyPetal hw;

Led led1, led2;

Parameter knob1, // vibrato rate
          knob2, // delay time
          knob3, // feedback
          knob4, // vibrato depth
          knob5, // shift amount
          knob6; // level (wet mix)

bool     pswitch1[2], pswitch2[2], pswitch3[2], pdip[4];
int      switch1[2], switch2[2], switch3[2], dip[4];

// Switch-derived state
float vibrato_depth_mult = 0.5f;
float max_delay_ms       = MAX_DELAY_MS_SMALL;
float shift_max_hz       = kShiftMaxSmall;
float shift_direction    = 1.0f;
bool  bypass_freqshift   = false;

struct FswState {
    bool  state     = false;
    bool  momentary = false;
    bool  pressed   = false;
    bool  rising    = false;
    bool  falling   = false;
    float time_held = 0.0f;

    inline bool operator==(const FswState& other) const { return state == other.state; }
    inline bool operator!=(const FswState& other) const { return state != other.state; }
    inline bool operator||(const FswState& other) const { return state || other.state; }
    inline operator bool() const { return state; }
};

FswState fsw1, fsw2;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// GLOBALS - DSP
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

CenoteDelayEngine del;
VibratoEngine     vibrato;

daisysp::Line bypass_ramp;
float         ramp_time_ms = 25.0f;

Xfade xfade;

bool prev_bypass_state = false;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// SWITCH CALLBACKS
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void updateSwitch1() // Vibrato depth: left=reduced, center=half-pot, right=full
{
    if (pswitch1[0]) {
        vibrato_depth_mult = 0.25f;
    } else if (pswitch1[1]) {
        vibrato_depth_mult = 1.0f;  // full depth, ignoring pot
    } else {
        vibrato_depth_mult = 0.5f;
    }
}

void updateSwitch2() // Time/Shift range: left=small, center=medium, right=large
{
    if (pswitch2[0]) {
        max_delay_ms = MAX_DELAY_MS_SMALL;
        shift_max_hz = kShiftMaxSmall;
    } else if (pswitch2[1]) {
        max_delay_ms = MAX_DELAY_MS_LARGE;
        shift_max_hz = kShiftMaxLarge;
    } else {
        max_delay_ms = MAX_DELAY_MS_MEDIUM;
        shift_max_hz = kShiftMaxMedium;
    }
}

void updateSwitch3() // Shift direction: left=down, center=bypass, right=up
{
    if (pswitch3[0]) {
        shift_direction  = -1.0f;
        bypass_freqshift = false;
    } else if (pswitch3[1]) {
        shift_direction  = 1.0f;
        bypass_freqshift = false;
    } else {
        shift_direction  = 0.0f;
        bypass_freqshift = true;
    }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// HARDWARE PROCESSING
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

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

    if (fsw1.rising) {
        fsw1.state = !fsw1.state;
        if (fsw1.state) { fsw2.state = false; }
    }

    if (fsw2.rising) {
        fsw2.state = !fsw2.state;
        if (fsw2.state) { fsw1.state = false; }
    }

    if (fsw1.pressed && fsw1.time_held > kMomentaryFswTimeMs) {
        fsw1.momentary = true;
    } else if (fsw1.falling && fsw1.momentary) {
        fsw1.momentary = false;
        fsw1.state = false;
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

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// AUDIO CALLBACK
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

static void AudioCallback(AudioHandle::InputBuffer  in,
                           AudioHandle::OutputBuffer out,
                           size_t                    size)
{
    hw.ProcessAnalogControls();
    hw.ProcessDigitalControls();

    UpdateButtons();
    UpdateSwitches();

    float vRate  = knob1.Process();
    float vTime  = knob2.Process();
    float vFb    = knob3.Process();
    float vDepth = knob4.Process();
    float vShift = knob5.Process();
    knob6.Process();

    // Delay time (switch 2 selects range)
    del.SetDelayMs(vTime * max_delay_ms);

    // Feedback (fsw2 = infinite hold)
    del.SetFeedback(fsw2.state ? 1.0f : (vFb * 0.9999999999f));

    // Frequency shift (switch 3 selects direction/bypass)
    del.SetBypassFrequencyShift(bypass_freqshift);
    del.SetTransposition(shift_direction * (vShift * shift_max_hz));

    // Vibrato (switch 1 selects depth multiplier)
    float lfodepth = (vibrato_depth_mult >= 1.0f) ? 1.0f : vDepth * vibrato_depth_mult;
    vibrato.SetLfoDepth(lfodepth);
    vibrato.SetLfoFreq(vRate * 15.0f + 0.1f);
    (vDepth < 0.1f) ? vibrato.SetMix(0.0f) : vibrato.SetMix(1.0f);

    // Xfade level active when either footswitch engaged
    xfade.SetCrossfade((fsw1.state || fsw2.state) ? knob6.Value() : 0.0f);

    // Bypass ramp on state transitions
    bool new_bypass_state = (fsw1 || fsw2);
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

        sig = vibrato.Process(sig);
        SIM_PROBE(p_post_vib, sig);

        float delay_in = sig * bypass_ramp.Process(&ramp_finished);
        SIM_PROBE(p_delay_in, delay_in);

        float del_out  = del.Process(delay_in, true, fsw2);
        SIM_PROBE(p_delay_out, del_out);

        sig = xfade.Process(sig, del_out);
        SIM_PROBE(p_pre_clip, sig);

        float result = SoftClip(sig);
        SIM_PROBE(p_output, result);

        out[0][i] = result;
        out[1][i] = result;
    }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// MAIN
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

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

    // Knobs (preserving original parameter curves)
    knob1.Init(hw.knob[Funbox::KNOB_1], 0.0f, 1.0f, Parameter::LINEAR);      // vibrato rate
    knob2.Init(hw.knob[Funbox::KNOB_2], 0.0f, 1.0f, Parameter::CUBE);        // delay time
    knob3.Init(hw.knob[Funbox::KNOB_3], 0.0f, 1.0f, Parameter::LINEAR);      // feedback
    knob4.Init(hw.knob[Funbox::KNOB_4], 0.0f, 1.0f, Parameter::LINEAR);      // vibrato depth
    knob5.Init(hw.knob[Funbox::KNOB_5], 0.0f, 1.0f, Parameter::EXPONENTIAL); // shift amount
    knob6.Init(hw.knob[Funbox::KNOB_6], 0.0f, 1.0f, Parameter::LINEAR);      // level

    // LEDs
    led1.Init(hw.seed.GetPin(Funbox::LED_1), false);
    led1.Update();
    led2.Init(hw.seed.GetPin(Funbox::LED_2), false);
    led2.Update();

    // DSP engines
    del.Init(sr);
    vibrato.Init(sr);

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
