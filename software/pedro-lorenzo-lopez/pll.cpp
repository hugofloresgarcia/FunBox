#include "daisy_petal.h"
#include "daisysp.h"
#include "funbox.h"

#include "pitch_tracker.h"
#include "pll_synth_voice.h"
#include "envelope_follower.h"
#include "voice_mixer.h"

using namespace daisy;
using namespace daisysp;
using namespace funbox;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// INTERVAL TABLES
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Master Oscillator: 8 intervals going up from root (just intonation)
static constexpr float kMasterIntervals[8] = {
    1.f,                // 0: Unison
    6.f / 5.f,          // 1: Minor 3rd
    5.f / 4.f,          // 2: Major 3rd
    4.f / 3.f,          // 3: Perfect 4th
    3.f / 2.f,          // 4: Perfect 5th
    2.f,                // 5: Octave
    3.f,                // 6: Octave + 5th
    4.f,                // 7: 2 Octaves
};

// Subharmonic: 8 intervals going down (divisions)
static constexpr float kSubIntervals[8] = {
    1.f / 2.f,          // 0: -1 Octave
    1.f / 3.f,          // 1: -1 Oct + 5th down
    1.f / 4.f,          // 2: -2 Octaves
    1.f / 5.f,          // 3: -2 Oct + Maj 3rd down
    1.f / 6.f,          // 4: -2 Oct + 5th down
    1.f / 8.f,          // 5: -3 Octaves
    1.f / 10.f,         // 6: -3 Oct + Maj 3rd down
    1.f / 12.f,         // 7: -3 Oct + 5th down
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// CONSTANTS
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

static constexpr float kMomentaryFswTimeMs  = 300.f;
static constexpr int   kNumIntervals        = 8;
static constexpr float kNoiseGateThreshold  = 0.007f;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// GLOBALS - HARDWARE
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

DaisyPetal hw;
Led led1, led2;

Parameter knob1,   // mod rate
          knob2,   // master interval
          knob3,   // sub interval
          knob4,   // square fuzz level
          knob5,   // master osc level
          knob6;   // sub level

bool pswitch1[2], pswitch2[2], pswitch3[2], pdip[4];
int  switch1[2], switch2[2], switch3[2], dip[4];

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// SWITCH STATE
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Switch 1: Master Root octave shift
float root_multiplier = 1.f; // 1.0 = unison, 0.5 = -1 oct, 0.25 = -2 oct

// Switch 2: Freq Mod mode
PllSynthVoice::ModMode mod_mode = PllSynthVoice::MOD_OFF;

// Switch 3: Sub Root source
enum SubRootMode { SUB_ROOT_UNISON, SUB_ROOT_OFF, SUB_ROOT_OSCILLATOR };
SubRootMode sub_root_mode = SUB_ROOT_OFF;

// DIP state
uint8_t synth_waveform = PllSynthVoice::WAVE_SQUARE;
bool    env_follow_on  = false;
bool    clean_blend_on = false;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// FOOTSWITCH STATE
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

struct FswState {
    bool  state     = false;
    bool  momentary = false;
    bool  pressed   = false;
    bool  rising    = false;
    bool  falling   = false;
    float time_held = 0.f;

    inline operator bool() const { return state; }
};

FswState fsw1, fsw2; // fsw1 = bypass, fsw2 = freeze

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// GLOBALS - DSP
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

PitchTracker     tracker;
PllSynthVoice    master_osc;
PllSynthVoice    sub_osc;
EnvelopeFollower env_follower;
EnvelopeFollower gate_follower;
VoiceMixer       mixer;

float frozen_freq = 0.f;
float current_detected_freq = 0.f;

// Noise gate ramp (1ms smoothing to avoid clicks)
float gate_gain = 0.f;
float gate_ramp_coeff = 0.f;

// LED2 pulse state
float led2_phase = 0.f;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// HELPERS
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

static inline int quantizeKnobTo8(float val)
{
    int idx = static_cast<int>(val * kNumIntervals);
    if (idx >= kNumIntervals) idx = kNumIntervals - 1;
    if (idx < 0) idx = 0;
    return idx;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// SWITCH CALLBACKS
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void updateSwitch1() // Master Root: L=-2oct, C=Unison, R=-1oct
{
    if (pswitch1[0])
        root_multiplier = 0.25f;       // -2 octaves
    else if (pswitch1[1])
        root_multiplier = 0.5f;        // -1 octave
    else
        root_multiplier = 1.f;         // Unison
}

void updateSwitch2() // Freq Mod: L=Glide, C=Off, R=Vibrato
{
    if (pswitch2[0])
        mod_mode = PllSynthVoice::MOD_GLIDE;
    else if (pswitch2[1])
        mod_mode = PllSynthVoice::MOD_VIBRATO;
    else
        mod_mode = PllSynthVoice::MOD_OFF;
}

void updateSwitch3() // Sub Root: L=Unison, C=Off, R=Oscillator
{
    if (pswitch3[0])
        sub_root_mode = SUB_ROOT_UNISON;
    else if (pswitch3[1])
        sub_root_mode = SUB_ROOT_OSCILLATOR;
    else
        sub_root_mode = SUB_ROOT_OFF;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// HARDWARE PROCESSING
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void UpdateButtons()
{
    // FSW1: Effect bypass (Flexi-Switch)
    fsw1.pressed   = hw.switches[Funbox::FOOTSWITCH_1].Pressed();
    fsw1.rising    = hw.switches[Funbox::FOOTSWITCH_1].RisingEdge();
    fsw1.falling   = hw.switches[Funbox::FOOTSWITCH_1].FallingEdge();
    fsw1.time_held = hw.switches[Funbox::FOOTSWITCH_1].TimeHeldMs();

    // FSW2: Freeze
    fsw2.pressed   = hw.switches[Funbox::FOOTSWITCH_2].Pressed();
    fsw2.rising    = hw.switches[Funbox::FOOTSWITCH_2].RisingEdge();
    fsw2.falling   = hw.switches[Funbox::FOOTSWITCH_2].FallingEdge();
    fsw2.time_held = hw.switches[Funbox::FOOTSWITCH_2].TimeHeldMs();

    // FSW1 toggle/momentary
    if (fsw1.rising)
        fsw1.state = !fsw1.state;

    if (fsw1.pressed && fsw1.time_held > kMomentaryFswTimeMs)
        fsw1.momentary = true;
    else if (fsw1.falling && fsw1.momentary)
    {
        fsw1.momentary = false;
        fsw1.state = false;
    }

    // FSW2 toggle/momentary (freeze)
    if (fsw2.rising)
    {
        fsw2.state = !fsw2.state;
        if (fsw2.state)
            frozen_freq = current_detected_freq;
    }

    if (fsw2.pressed && fsw2.time_held > kMomentaryFswTimeMs)
    {
        fsw2.momentary = true;
        if (!fsw2.state)
        {
            fsw2.state = true;
            frozen_freq = current_detected_freq;
        }
    }
    else if (fsw2.falling && fsw2.momentary)
    {
        fsw2.momentary = false;
        fsw2.state = false;
    }

    led1.Set(fsw1.state ? 1.f : 0.f);
    led2.Set(fsw2.state ? 1.f : 0.f);
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

    // DIP switches
    for (int i = 0; i < 4; i++) {
        if (hw.switches[dip[i]].Pressed() != pdip[i])
            pdip[i] = hw.switches[dip[i]].Pressed();
    }

    // DIP 1-2: waveform (2 bits)
    synth_waveform = (pdip[0] ? 1 : 0) | (pdip[1] ? 2 : 0);
    if (synth_waveform >= PllSynthVoice::WAVE_LAST)
        synth_waveform = PllSynthVoice::WAVE_SQUARE;

    // DIP 3: envelope follower
    env_follow_on = pdip[2];

    // DIP 4: clean blend
    clean_blend_on = pdip[3];
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

    float vModRate  = knob1.Process();
    float vMasterInt= knob2.Process();
    float vSubInt   = knob3.Process();
    float vSqLevel  = knob4.Process();
    float vMastLevel= knob5.Process();
    float vSubLevel = knob6.Process();

    // Quantize interval knobs to 8 steps
    int master_interval_idx = quantizeKnobTo8(vMasterInt);
    int sub_interval_idx    = quantizeKnobTo8(vSubInt);

    float master_ratio = kMasterIntervals[master_interval_idx];
    float sub_ratio    = kSubIntervals[sub_interval_idx];

    // Configure voices
    master_osc.SetWaveform(synth_waveform);
    master_osc.SetModMode(mod_mode);
    master_osc.SetModRate(vModRate);

    sub_osc.SetWaveform(synth_waveform);
    // Sub gets glide when master does, but no vibrato of its own
    // unless Sub Root = Oscillator (then freq mod is inherited via the master freq)
    if (sub_root_mode == SUB_ROOT_OSCILLATOR)
        sub_osc.SetModMode(PllSynthVoice::MOD_OFF);
    else
        sub_osc.SetModMode(mod_mode);
    sub_osc.SetModRate(vModRate);

    // Mixer levels
    mixer.SetSquareLevel(vSqLevel);
    mixer.SetMasterLevel(vMastLevel);
    mixer.SetSubLevel(vSubLevel);
    mixer.SetCleanBlend(clean_blend_on);

    // Envelope follower
    env_follower.SetEnabled(env_follow_on);

    for (size_t i = 0; i < size; i++)
    {
        float dry = in[0][i];

        if (!fsw1.state)
        {
            // Bypassed
            out[0][i] = dry;
            out[1][i] = dry;
            continue;
        }

        // Pitch tracking (always runs to keep tracker warm)
        tracker.Process(dry);

        if (!fsw2.state)
            current_detected_freq = tracker.GetFrequency();

        // Determine the root frequency
        float base_freq = fsw2.state ? frozen_freq : current_detected_freq;
        float root_freq = base_freq * root_multiplier;

        // Master Oscillator target frequency
        float master_target = root_freq * master_ratio;
        master_osc.SetTargetFreq(master_target);

        // Subharmonic target frequency
        float sub_target = 0.f;
        switch (sub_root_mode)
        {
        case SUB_ROOT_UNISON:
            sub_target = base_freq * sub_ratio;
            break;
        case SUB_ROOT_OSCILLATOR:
            sub_target = master_target * sub_ratio;
            break;
        case SUB_ROOT_OFF:
        default:
            sub_target = 0.f;
            break;
        }
        sub_osc.SetTargetFreq(sub_target);

        // Generate voices
        float sq_voice     = tracker.GetSquareOut();
        float master_voice = master_osc.Process();
        float sub_voice    = sub_osc.Process();

        // Envelope follower modulation
        float env_val = env_follower.Process(dry);
        if (env_follow_on)
        {
            float env_gain = env_val * 3.f; // scale up for usable range
            if (env_gain > 1.f) env_gain = 1.f;
            sq_voice     *= env_gain;
            master_voice *= env_gain;
            sub_voice    *= env_gain;
        }

        // Noise gate: use input amplitude + frequency detection, with 1ms ramp
        float input_env = gate_follower.Process(dry);
        bool gated = !fsw2.state
                     && (input_env < kNoiseGateThreshold || base_freq < 1.f);

        float gate_target = gated ? 0.f : 1.f;
        gate_gain += gate_ramp_coeff * (gate_target - gate_gain);

        sq_voice     *= gate_gain;
        master_voice *= gate_gain;
        sub_voice    *= gate_gain;

        float result = mixer.Process(sq_voice, master_voice, sub_voice, dry);

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
    for (int i = 0; i < 4; i++)
        pdip[i] = false;

    // Knobs
    knob1.Init(hw.knob[Funbox::KNOB_1], 0.f, 1.f, Parameter::LINEAR);      // mod rate
    knob2.Init(hw.knob[Funbox::KNOB_2], 0.f, 1.f, Parameter::LINEAR);      // master interval
    knob3.Init(hw.knob[Funbox::KNOB_3], 0.f, 1.f, Parameter::LINEAR);      // sub interval
    knob4.Init(hw.knob[Funbox::KNOB_4], 0.f, 1.f, Parameter::LINEAR);      // square level
    knob5.Init(hw.knob[Funbox::KNOB_5], 0.f, 1.f, Parameter::LINEAR);      // master level
    knob6.Init(hw.knob[Funbox::KNOB_6], 0.f, 1.f, Parameter::LINEAR);      // sub level

    // LEDs
    led1.Init(hw.seed.GetPin(Funbox::LED_1), false);
    led1.Update();
    led2.Init(hw.seed.GetPin(Funbox::LED_2), false);
    led2.Update();

    // DSP init
    tracker.Init(sr);
    master_osc.Init(sr);
    sub_osc.Init(sr);
    env_follower.Init(sr);
    gate_follower.Init(sr);
    gate_follower.SetAttack(0.0005f);
    gate_follower.SetRelease(0.08f);
    mixer.Init(sr);

    // 1ms ramp for noise gate open/close
    gate_ramp_coeff = 1.f - expf(-1.f / (0.001f * sr));

    hw.StartAdc();
    hw.StartAudio(AudioCallback);

    while (1)
    {
        System::Delay(10);
    }
}
