#include "daisy_petal.h"
#include "daisysp.h"
#include "funbox.h"

#include "octave_engine.h"
#include "multirate.h"
#include "biquad.h"

#include <cmath>

using namespace daisy;
using namespace daisysp;
using namespace funbox;

// ────────────────────────────────────────────────────────────────────────────
// HARDWARE
// ────────────────────────────────────────────────────────────────────────────

DaisyPetal hw;
Parameter knobBitDepth, knobCutoff, knobRes, knobSampleRate, knobTone, knobMix;

Led led1, led2;

bool pswitch1[2], pswitch2[2], pswitch3[2], pdip[4];
int  switch1[2], switch2[2], switch3[2], dip[4];

// ────────────────────────────────────────────────────────────────────────────
// DSP MODULES
// ────────────────────────────────────────────────────────────────────────────

// Octave engine (polyphonic, 80-band ERB-PS2 from SubNUp)
OctaveEngine       octave;
::Decimator        oct_decim;
::Interpolator     oct_interp;
::Biquad           eq_highshelf;
::Biquad           eq_lowshelf;

// Pre-filter (anti-aliasing, OP-1 Terminal style) + post-tone
Svf       pre_filter;
Svf       post_tone;

// Bitcrusher (explicit namespace to avoid collision with multirate::Decimator)
daisysp::Decimator bitcrusher;

// Dry/wet mix
CrossFade mix;

// ────────────────────────────────────────────────────────────────────────────
// STATE
// ────────────────────────────────────────────────────────────────────────────

bool bypass = true;

int octave_mode = 0;       // 0=off, 1=sub (-1 oct), 2=up (+1 oct)
int pre_filter_model = 0;  // 0=LP, 1=BP, 2=HP

bool bypass_bc = true;
bool bc_extreme = false;

bool pre_filter_post = false; // DIP 2: false=before crusher, true=after
bool noise_gate_on   = false; // DIP 3: input noise gate

bool fsw2_oct_up = false;

// Noise gate envelope follower
float gate_env = 0.f;
static constexpr float kGateThreshold = 0.005f; // ~-46 dB
static constexpr float kGateAttack   = 0.01f;
static constexpr float kGateRelease  = 0.0001f;

// ────────────────────────────────────────────────────────────────────────────
// SWITCH CALLBACKS
// ────────────────────────────────────────────────────────────────────────────

void updateSwitch1() // Octave: left=off, center=sub, right=up
{
    if (pswitch1[0] == true) {
        octave_mode = 0;
    } else if (pswitch1[1] == true) {
        octave_mode = 2;
    } else {
        octave_mode = 1;
    }
}

void updateSwitch2() // Pre-filter model: left=HP, center=BP, right=LP
{
    if (pswitch2[0] == true) {
        pre_filter_model = 2;  // left: HP
    } else if (pswitch2[1] == true) {
        pre_filter_model = 0;  // right: LP
    } else {
        pre_filter_model = 1;  // center: BP
    }
}

void updateSwitch3() // Bitcrusher: left=bypass, center=on, right=extreme
{
    if (pswitch3[0] == true) {
        bypass_bc = true;
        bc_extreme = false;
    } else if (pswitch3[1] == true) {
        bypass_bc = false;
        bc_extreme = true;
    } else {
        bypass_bc = false;
        bc_extreme = false;
    }
}

// ────────────────────────────────────────────────────────────────────────────
// HARDWARE PROCESSING
// ────────────────────────────────────────────────────────────────────────────

void UpdateButtons()
{
    if (hw.switches[Funbox::FOOTSWITCH_1].RisingEdge())
    {
        bypass = !bypass;
        led1.Set(bypass ? 0.0f : 1.0f);
    }

    if (hw.switches[Funbox::FOOTSWITCH_2].RisingEdge())
    {
        fsw2_oct_up = !fsw2_oct_up;
        led2.Set(fsw2_oct_up ? 1.0f : 0.0f);
    }

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

    pre_filter_post = pdip[1];
    noise_gate_on   = pdip[2];
}

// ────────────────────────────────────────────────────────────────────────────
// DSP HELPERS
// ────────────────────────────────────────────────────────────────────────────

inline float applyPreFilter(float sig)
{
    pre_filter.Process(sig);
    if (pre_filter_model == 0)      return pre_filter.Low();
    else if (pre_filter_model == 1) return pre_filter.Band();
    else                            return pre_filter.High();
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

    float vBitDepth   = knobBitDepth.Process();
    float vCutoff     = knobCutoff.Process();
    float vRes        = knobRes.Process();
    float vSampleRate = knobSampleRate.Process();
    float vTone       = knobTone.Process();
    float vMix        = knobMix.Process();

    // Pre-filter (anti-aliasing before bitcrusher)
    pre_filter.SetFreq(vCutoff);
    pre_filter.SetRes(vRes);

    // Bitcrusher
    bitcrusher.SetBitcrushFactor(vBitDepth);
    float downsample = bc_extreme ? fclamp(vSampleRate * 2.0f, 0.0f, 1.0f) : vSampleRate;
    bitcrusher.SetDownsampleFactor(downsample);

    // Gain compensation: bitcrushing boosts RMS as quantization steps get coarser.
    // Squared curve so attenuation ramps harder at extreme settings.
    float bc_comp = 1.0f - 0.80f * vBitDepth * vBitDepth;

    // Post-tone filter
    post_tone.SetFreq(vTone);

    // Mix
    mix.SetPos(vMix);

    // Process in 6-sample chunks (required by octave engine's multirate pipeline)
    float in_chunk[kResampleFactor];
    float oct_chunk[kResampleFactor];

    for (size_t i = 0; i < size; i += kResampleFactor)
    {
        if (bypass)
        {
            for (size_t j = 0; j < kResampleFactor; ++j)
            {
                out[0][i + j] = in[0][i + j];
                out[1][i + j] = in[0][i + j];
            }
            continue;
        }

        // Gather 6 input samples with optional noise gate
        for (size_t j = 0; j < kResampleFactor; ++j)
        {
            float sig = in[0][i + j];
            if (noise_gate_on)
            {
                float abs_sig = fabsf(sig);
                float coeff = (abs_sig > gate_env) ? kGateAttack : kGateRelease;
                fonepole(gate_env, abs_sig, coeff);
                sig *= (gate_env > kGateThreshold) ? 1.f : (gate_env / kGateThreshold);
            }
            in_chunk[j] = sig;
        }

        // Octave engine: decimate 6 -> 1, process, interpolate 1 -> 6
        bool run_octave = (octave_mode != 0) || fsw2_oct_up;
        if (run_octave)
        {
            float decimated = oct_decim.Process(in_chunk);
            octave.Process(decimated);

            float oct_sig = 0.f;
            if (octave_mode == 1) oct_sig += octave.Down1();
            if (octave_mode == 2 || fsw2_oct_up) oct_sig += octave.Up1();

            oct_interp.Process(oct_sig, oct_chunk);
        }

        // Per-sample output at 48 kHz
        for (size_t j = 0; j < kResampleFactor; ++j)
        {
            float dry = in_chunk[j];
            float sig;

            if (!run_octave)
                sig = dry;
            else
            {
                sig = oct_chunk[j];
                sig = eq_highshelf.Process(sig);
                sig = eq_lowshelf.Process(sig);
            }

            // Pre-filter -> Bitcrusher (or reversed via DIP 2)
            if (!pre_filter_post)
            {
                sig = applyPreFilter(sig);
                if (!bypass_bc) { sig = bitcrusher.Process(sig); sig *= bc_comp; }
            }
            else
            {
                if (!bypass_bc) { sig = bitcrusher.Process(sig); sig *= bc_comp; }
                sig = applyPreFilter(sig);
            }

            // Post-tone
            post_tone.Process(sig);
            sig = post_tone.Low();

            float out_sample = mix.Process(dry, sig);
            out_sample = SoftClip(out_sample);

            out[0][i + j] = out_sample;
            out[1][i + j] = out_sample;
        }
    }
}

// ────────────────────────────────────────────────────────────────────────────
// MAIN
// ────────────────────────────────────────────────────────────────────────────

int main(void)
{
    hw.Init();
    float samplerate = hw.AudioSampleRate();

    hw.SetAudioBlockSize(48);

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

    pswitch1[0] = false;
    pswitch1[1] = false;
    pswitch2[0] = false;
    pswitch2[1] = false;
    pswitch3[0] = false;
    pswitch3[1] = false;
    pdip[0] = false;
    pdip[1] = false;
    pdip[2] = false;
    pdip[3] = false;

    // Knobs: top row = K1 K2 K3, bottom row = K4 K5 K6
    //  Bit Depth   | Pre Cutoff | Pre Reso
    //  Sample Rate |    Tone    |   Mix
    knobBitDepth.Init(hw.knob[Funbox::KNOB_1], 0.0f, 1.0f, Parameter::LINEAR);
    knobCutoff.Init(hw.knob[Funbox::KNOB_2], 200.0f, 16000.0f, Parameter::LOGARITHMIC);
    knobRes.Init(hw.knob[Funbox::KNOB_3], 0.0f, 0.95f, Parameter::LINEAR);
    knobSampleRate.Init(hw.knob[Funbox::KNOB_4], 0.0f, 0.9f, Parameter::LINEAR);
    knobTone.Init(hw.knob[Funbox::KNOB_5], 200.0f, 16000.0f, Parameter::LOGARITHMIC);
    knobMix.Init(hw.knob[Funbox::KNOB_6], 0.0f, 1.0f, Parameter::LINEAR);

    // Octave engine (runs at sr/6 = 8 kHz)
    float octave_sr = samplerate / static_cast<float>(kResampleFactor);
    octave.Init(octave_sr);

    // EQ compensation for octave engine filter bank response
    eq_highshelf.Init(samplerate);
    eq_highshelf.SetHighShelf(140.f, -11.f);
    eq_lowshelf.Init(samplerate);
    eq_lowshelf.SetLowShelf(160.f, 5.f);

    // Pre-filter (anti-aliasing)
    pre_filter.Init(samplerate);
    pre_filter.SetFreq(16000.0f);
    pre_filter.SetRes(0.1f);

    // Post-tone filter
    post_tone.Init(samplerate);
    post_tone.SetFreq(16000.0f);
    post_tone.SetRes(0.1f);

    // Bitcrusher
    bitcrusher.Init();
    bitcrusher.SetSmoothCrushing(true);

    // Dry/wet mix
    mix.Init(CROSSFADE_CPOW);

    // LEDs and bypass
    led1.Init(hw.seed.GetPin(Funbox::LED_1), false);
    led1.Update();
    bypass = true;

    led2.Init(hw.seed.GetPin(Funbox::LED_2), false);
    led2.Update();

    hw.StartAdc();
    hw.StartAudio(AudioCallback);
    while (1)
    {
        System::Delay(10);
    }
}
