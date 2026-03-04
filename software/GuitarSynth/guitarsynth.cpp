#include "daisy_petal.h"
#include "daisysp.h"
#include "funbox.h"

#include "poly_pitch.h"
#include "synth_voice.h"
#include "envelope_follower.h"

using namespace daisy;
using namespace daisysp;
using namespace funbox;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// CONSTANTS
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

static constexpr float kMomentaryFswTimeMs = 300.f;
static constexpr float kNoiseGateThresh    = 0.002f;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// GLOBALS - HARDWARE
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

DaisyPetal hw;
Led led1, led2;

Parameter knob1,  // waveform select (saw..square..sine)
          knob2,  // filter cutoff
          knob3,  // filter resonance
          knob4,  // envelope depth
          knob5,  // synth level
          knob6;  // dry/wet mix

bool pswitch1[2], pswitch2[2], pswitch3[2], pdip[4];
int  switch1[2], switch2[2], switch3[2], dip[4];

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// SWITCH STATE
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Switch 1: Waveform  L=Saw, C=Square, R=Sine
int osc_waveform = SynthVoice::WAVE_SAW;

// Switch 2: Filter mode  L=LP, C=BP, R=HP
enum FilterMode { FILT_LP = 0, FILT_BP, FILT_HP };
int filter_mode = FILT_LP;

// Switch 3: Octave  L=-1, C=Unison, R=+1
float octave_multiplier = 1.f;

// DIP state
bool env_slow      = false;  // DIP 1
bool env_inverted  = false;  // DIP 2
bool portamento_on = false;  // DIP 3
bool filter_drive  = false;  // DIP 4

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

FswState fsw1, fsw2;  // fsw1 = bypass, fsw2 = freeze

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// GLOBALS - DSP
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

PolyPitchDetector detector;
SynthVoice        voices[kMaxVoices];
EnvelopeFollower  env_follower;
EnvelopeFollower  gate_follower;
Svf               filt;

// DC blocker state
float dc_z1_in  = 0.f;
float dc_z1_out = 0.f;
float dc_coeff  = 0.f;

// Gate ramp for noise gate
float gate_gain      = 0.f;
float gate_ramp_coeff = 0.f;

// Freeze state: snapshot of voices at freeze moment
bool  frozen = false;
float frozen_freqs[kMaxVoices] = {};
float frozen_amps[kMaxVoices]  = {};

// ISR <-> main-loop handshake for deferred FFT analysis
volatile bool analysis_pending = false;
volatile bool results_ready    = false;

// Noise source for unpitched input
static uint32_t noise_seed = 1;
inline float noise_sample()
{
    noise_seed ^= noise_seed << 13;
    noise_seed ^= noise_seed >> 17;
    noise_seed ^= noise_seed << 5;
    return static_cast<float>(static_cast<int32_t>(noise_seed)) * (1.f / 2147483648.f);
}

// Pitched/unpitched crossfade (0 = all oscillators, 1 = all noise)
float noise_blend        = 0.f;
float noise_blend_target = 0.f;
float noise_blend_coeff  = 0.f;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// SWITCH CALLBACKS
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void updateSwitch1()
{
    if (pswitch1[0])
        osc_waveform = SynthVoice::WAVE_SAW;
    else if (pswitch1[1])
        osc_waveform = SynthVoice::WAVE_SIN;
    else
        osc_waveform = SynthVoice::WAVE_SQUARE;
}

void updateSwitch2()
{
    if (pswitch2[0])
        filter_mode = FILT_LP;
    else if (pswitch2[1])
        filter_mode = FILT_HP;
    else
        filter_mode = FILT_BP;
}

void updateSwitch3()
{
    if (pswitch3[0])
        octave_multiplier = 0.5f;   // -1 octave
    else if (pswitch3[1])
        octave_multiplier = 2.f;    // +1 octave
    else
        octave_multiplier = 1.f;    // unison
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// HARDWARE PROCESSING
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void UpdateButtons()
{
    fsw1.pressed   = hw.switches[Funbox::FOOTSWITCH_1].Pressed();
    fsw1.rising    = hw.switches[Funbox::FOOTSWITCH_1].RisingEdge();
    fsw1.falling   = hw.switches[Funbox::FOOTSWITCH_1].FallingEdge();
    fsw1.time_held = hw.switches[Funbox::FOOTSWITCH_1].TimeHeldMs();

    fsw2.pressed   = hw.switches[Funbox::FOOTSWITCH_2].Pressed();
    fsw2.rising    = hw.switches[Funbox::FOOTSWITCH_2].RisingEdge();
    fsw2.falling   = hw.switches[Funbox::FOOTSWITCH_2].FallingEdge();
    fsw2.time_held = hw.switches[Funbox::FOOTSWITCH_2].TimeHeldMs();

    // FSW1 toggle/momentary (bypass)
    if (fsw1.rising)
        fsw1.state = !fsw1.state;
    if (fsw1.pressed && fsw1.time_held > kMomentaryFswTimeMs)
        fsw1.momentary = true;
    else if (fsw1.falling && fsw1.momentary)
    {
        fsw1.momentary = false;
        fsw1.state     = false;
    }

    // FSW2 toggle/momentary (freeze)
    if (fsw2.rising)
    {
        fsw2.state = !fsw2.state;
        if (fsw2.state)
        {
            // Snapshot current detected voices
            frozen = true;
            const DetectedVoice* dv = detector.GetVoices();
            for (int i = 0; i < kMaxVoices; i++)
            {
                frozen_freqs[i] = dv[i].freq;
                frozen_amps[i]  = dv[i].amp;
            }
        }
        else
        {
            frozen = false;
        }
    }
    if (fsw2.pressed && fsw2.time_held > kMomentaryFswTimeMs)
    {
        fsw2.momentary = true;
        if (!fsw2.state)
        {
            fsw2.state = true;
            frozen = true;
            const DetectedVoice* dv = detector.GetVoices();
            for (int i = 0; i < kMaxVoices; i++)
            {
                frozen_freqs[i] = dv[i].freq;
                frozen_amps[i]  = dv[i].amp;
            }
        }
    }
    else if (fsw2.falling && fsw2.momentary)
    {
        fsw2.momentary = false;
        fsw2.state     = false;
        frozen         = false;
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

    for (int i = 0; i < 4; i++) {
        if (hw.switches[dip[i]].Pressed() != pdip[i])
            pdip[i] = hw.switches[dip[i]].Pressed();
    }

    env_slow      = pdip[0];
    env_inverted  = pdip[1];
    portamento_on = pdip[2];
    filter_drive  = pdip[3];
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

    float vWaveform = knob1.Process();
    float vCutoff   = knob2.Process();
    float vReso     = knob3.Process();
    float vEnvDepth = knob4.Process();
    float vSynthLvl = knob5.Process();
    float vMix      = knob6.Process();

    // Map waveform knob to discrete selection
    // (switch already selects, knob could be used for future morphing)
    (void)vWaveform;

    // Configure filter
    float drive_val = filter_drive ? 0.4f : 0.002f;
    filt.SetRes(vReso);
    filt.SetDrive(drive_val);

    // Configure envelope follower speed
    float env_atk = env_slow ? 0.02f : 0.001f;
    float env_rel = 0.06f;
    env_follower.SetAttack(env_atk);
    env_follower.SetRelease(env_rel);

    // Configure portamento
    float porta_time = portamento_on ? 0.08f : 0.f;
    for (int v = 0; v < kMaxVoices; v++)
    {
        voices[v].SetWaveform(osc_waveform);
        voices[v].SetPortamento(porta_time);
    }

    // Apply analysis results once per block (produced by main loop)
    if (results_ready)
    {
        float tonality = detector.GetTonality();

        // Hysteresis: switch to noise below 0.4, back to pitched above 0.6
        if (tonality < 0.4f)
            noise_blend_target = 1.f;
        else if (tonality > 0.6f)
            noise_blend_target = 0.f;

        if (!frozen)
        {
            if (noise_blend_target < 0.5f)
            {
                const DetectedVoice* dv = detector.GetVoices();
                for (int v = 0; v < kMaxVoices; v++)
                {
                    if (dv[v].active)
                    {
                        voices[v].SetTargetFreq(dv[v].freq * octave_multiplier);
                        voices[v].SetTargetAmp(dv[v].amp);
                    }
                    else
                    {
                        voices[v].NoteOff();
                    }
                }
            }
            else
            {
                for (int v = 0; v < kMaxVoices; v++)
                    voices[v].NoteOff();
            }
        }
        else
        {
            for (int v = 0; v < kMaxVoices; v++)
            {
                if (frozen_freqs[v] > 0.f)
                {
                    voices[v].SetTargetFreq(frozen_freqs[v] * octave_multiplier);
                    voices[v].SetTargetAmp(frozen_amps[v]);
                }
                else
                {
                    voices[v].NoteOff();
                }
            }
        }
        results_ready = false;
    }

    for (size_t i = 0; i < size; i++)
    {
        float dry = in[0][i];

        if (!fsw1.state)
        {
            out[0][i] = dry;
            out[1][i] = dry;
            continue;
        }

        // Feed pitch detector ring buffer; flag main loop when a frame is due
        if (detector.Process(dry))
            analysis_pending = true;

        // Envelope follower on input (for filter modulation)
        float env_val = env_follower.Process(dry);

        // Noise gate
        float input_env = gate_follower.Process(dry);
        bool gated = !frozen && (input_env < kNoiseGateThresh);
        float gate_target = gated ? 0.f : 1.f;
        gate_gain += gate_ramp_coeff * (gate_target - gate_gain);

        // Sum all synth voices and crossfade with noise for unpitched input
        float synth_sum = 0.f;
        for (int v = 0; v < kMaxVoices; v++)
            synth_sum += voices[v].Process();

        float noise = noise_sample();
        noise_blend += noise_blend_coeff * (noise_blend_target - noise_blend);
        float src = synth_sum * (1.f - noise_blend) + noise * noise_blend;
        src *= gate_gain * vSynthLvl * 0.25f;

        // Filter with envelope modulation
        float env_mod   = env_inverted ? (1.f - env_val) : env_val;
        float filt_freq = vCutoff + vEnvDepth * env_mod;
        if (filt_freq < 20.f)    filt_freq = 20.f;
        if (filt_freq > 16000.f) filt_freq = 16000.f;
        filt.SetFreq(filt_freq);

        filt.Process(src);
        float filtered = 0.f;
        switch (filter_mode)
        {
            case FILT_LP: filtered = filt.Low();  break;
            case FILT_BP: filtered = filt.Band(); break;
            case FILT_HP: filtered = filt.High(); break;
            default:      filtered = filt.Low();  break;
        }

        // Guard against NaN/Inf from filter blowup
        if (filtered != filtered || filtered > 1e6f || filtered < -1e6f)
        {
            filtered = 0.f;
            filt.Init(hw.AudioSampleRate());
            filt.SetFreq(8000.f);
            filt.SetRes(0.3f);
        }

        // Soft-clip synth before mixing so it can't overwhelm the dry signal
        filtered = tanhf(filtered);

        // Dry/wet mix
        float mixed = dry * (1.f - vMix) + filtered * vMix;

        // DC blocking
        float dc_out = mixed - dc_z1_in + dc_coeff * dc_z1_out;
        dc_z1_in  = mixed;
        dc_z1_out = dc_out;

        float result = dc_out;
        if (result > 1.f) result = 1.f;
        if (result < -1.f) result = -1.f;

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
        pswitch1[i] = false;
        pswitch2[i] = false;
        pswitch3[i] = false;
    }
    for (int i = 0; i < 4; i++)
        pdip[i] = false;

    // Knobs
    knob1.Init(hw.knob[Funbox::KNOB_1], 0.f, 1.f,     Parameter::LINEAR);       // waveform
    knob2.Init(hw.knob[Funbox::KNOB_2], 20.f, 18000.f, Parameter::LOGARITHMIC);  // cutoff
    knob3.Init(hw.knob[Funbox::KNOB_3], 0.0f, 0.9f,    Parameter::LINEAR);       // resonance
    knob4.Init(hw.knob[Funbox::KNOB_4], 0.0f, 10000.f, Parameter::LOGARITHMIC);  // env depth
    knob5.Init(hw.knob[Funbox::KNOB_5], 0.0f, 1.0f,    Parameter::LINEAR);       // synth level
    knob6.Init(hw.knob[Funbox::KNOB_6], 0.0f, 1.0f,    Parameter::LINEAR);       // mix

    // LEDs
    led1.Init(hw.seed.GetPin(Funbox::LED_1), false);
    led1.Update();
    led2.Init(hw.seed.GetPin(Funbox::LED_2), false);
    led2.Update();

    // DSP init
    detector.Init(sr);

    for (int v = 0; v < kMaxVoices; v++)
        voices[v].Init(sr);

    env_follower.Init(sr);
    gate_follower.Init(sr);
    gate_follower.SetAttack(0.0005f);
    gate_follower.SetRelease(0.08f);

    filt.Init(sr);
    filt.SetFreq(8000.f);
    filt.SetRes(0.3f);
    filt.SetDrive(0.002f);

    dc_coeff = 1.f - (126.f / sr);
    gate_ramp_coeff = 1.f - expf(-1.f / (0.01f * sr));
    noise_blend_coeff = 1.f - expf(-1.f / (0.02f * sr));

    hw.StartAdc();
    hw.StartAudio(AudioCallback);

    while (1)
    {
        if (analysis_pending)
        {
            analysis_pending = false;
            detector.Analyze();
            results_ready = true;
        }
        System::Delay(1);
    }
}
