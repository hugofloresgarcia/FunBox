#include "daisy_petal.h"
#include "daisysp.h"
#include "funbox.h"
#include <cmath>

using namespace daisy;
using namespace daisysp;
using namespace funbox;

DaisyPetal hw;
Parameter knobBitDepth, knobCutoff, knobRes, knobSampleRate, knobRingFreq, knobMix;

Oscillator carrier;
Decimator  decimator;
Svf        tone;
CrossFade  mix;

bool bypass;

bool pswitch1[2], pswitch2[2], pswitch3[2], pdip[4];
int  switch1[2], switch2[2], switch3[2], dip[4];

Led led1, led2;

int octave_mode = 0; // 0=off, 1=sub, 2=up
bool last_positive = true;
float sub_osc = 1.0f;

bool bypass_rm = true;
uint8_t rm_waveform = Oscillator::WAVE_SIN;

bool bypass_bc = true;
bool bc_extreme = false;

bool reverse_order = false;
bool rm_freq_high = false;
bool tone_pre = false;

bool chaos_active = false;


void updateSwitch1() // Octave: left=off, center=sub, right=up
{
    if (pswitch1[0] == true) {        // left: off
        octave_mode = 0;
    } else if (pswitch1[1] == true) { // right: octave up
        octave_mode = 2;
    } else {                          // center: sub octave
        octave_mode = 1;
    }
}

void updateSwitch2() // Ring Mod: left=bypass, center=sine, right=square
{
    if (pswitch2[0] == true) {        // left: bypass
        bypass_rm = true;
        rm_waveform = Oscillator::WAVE_SIN;
    } else if (pswitch2[1] == true) { // right: square carrier
        bypass_rm = false;
        rm_waveform = Oscillator::WAVE_POLYBLEP_SQUARE;
    } else {                          // center: sine carrier
        bypass_rm = false;
        rm_waveform = Oscillator::WAVE_SIN;
    }
}

void updateSwitch3() // Bitcrusher: left=bypass, center=on, right=extreme
{
    if (pswitch3[0] == true) {        // left: bypass
        bypass_bc = true;
        bc_extreme = false;
    } else if (pswitch3[1] == true) { // right: on + extreme SR reduction
        bypass_bc = false;
        bc_extreme = true;
    } else {                          // center: on
        bypass_bc = false;
        bc_extreme = false;
    }
}


void UpdateButtons()
{
    // FSW1: bypass toggle
    if (hw.switches[Funbox::FOOTSWITCH_1].FallingEdge())
    {
        bypass = !bypass;
        led1.Set(bypass ? 0.0f : 1.0f);
    }

    // FSW2: momentary chaos
    chaos_active = hw.switches[Funbox::FOOTSWITCH_2].Pressed();
    led2.Set(chaos_active ? 1.0f : 0.0f);

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
    if (changed1)
        updateSwitch1();

    bool changed2 = false;
    for (int i = 0; i < 2; i++) {
        if (hw.switches[switch2[i]].Pressed() != pswitch2[i]) {
            pswitch2[i] = hw.switches[switch2[i]].Pressed();
            changed2 = true;
        }
    }
    if (changed2)
        updateSwitch2();

    bool changed3 = false;
    for (int i = 0; i < 2; i++) {
        if (hw.switches[switch3[i]].Pressed() != pswitch3[i]) {
            pswitch3[i] = hw.switches[switch3[i]].Pressed();
            changed3 = true;
        }
    }
    if (changed3)
        updateSwitch3();

    for (int i = 0; i < 4; i++) {
        if (hw.switches[dip[i]].Pressed() != pdip[i]) {
            pdip[i] = hw.switches[dip[i]].Pressed();
        }
    }

    reverse_order = pdip[1];
    rm_freq_high  = pdip[2];
    tone_pre      = pdip[3];
}


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
    float vRingFreq   = knobRingFreq.Process();
    float vMix        = knobMix.Process();

    // Ring mod carrier frequency
    float rm_freq_min = rm_freq_high ? 200.0f : 20.0f;
    float rm_freq_max = rm_freq_high ? 5000.0f : 1000.0f;
    float rm_freq = chaos_active ? rm_freq_max : vRingFreq;
    carrier.SetFreq(rm_freq_min + rm_freq * (rm_freq_max - rm_freq_min));
    carrier.SetWaveform(rm_waveform);
    carrier.SetAmp(1.0f);

    // Bitcrusher
    decimator.SetBitcrushFactor(vBitDepth);
    float downsample = bc_extreme ? fclamp(vSampleRate * 2.0f, 0.0f, 1.0f) : vSampleRate;
    decimator.SetDownsampleFactor(downsample);
    decimator.SetSmoothCrushing(true);

    // Tone filter
    tone.SetFreq(vCutoff);
    tone.SetRes(vRes);

    // Mix
    mix.SetPos(vMix);

    for (size_t i = 0; i < size; i++)
    {
        if (bypass)
        {
            out[0][i] = in[0][i];
            out[1][i] = in[1][i];
        }
        else
        {
            float dry = in[0][i];
            float sig = dry;

            if (!reverse_order)
            {
                // Oct -> RM -> BC (default)
                if (octave_mode == 1) {
                    bool positive = (sig >= 0.0f);
                    if (positive && !last_positive)
                        sub_osc = -sub_osc;
                    last_positive = positive;
                    sig = sig + sig * sub_osc * 0.5f;
                } else if (octave_mode == 2) {
                    sig = fabsf(sig);
                }

                if (tone_pre) {
                    tone.Process(sig);
                    sig = tone.Low();
                }

                if (!bypass_rm)
                    sig = sig * carrier.Process();

                if (!bypass_bc)
                    sig = decimator.Process(sig);
            }
            else
            {
                // BC -> RM -> Oct (reverse)
                if (!bypass_bc)
                    sig = decimator.Process(sig);

                if (tone_pre) {
                    tone.Process(sig);
                    sig = tone.Low();
                }

                if (!bypass_rm)
                    sig = sig * carrier.Process();

                if (octave_mode == 1) {
                    bool positive = (sig >= 0.0f);
                    if (positive && !last_positive)
                        sub_osc = -sub_osc;
                    last_positive = positive;
                    sig = sig + sig * sub_osc * 0.5f;
                } else if (octave_mode == 2) {
                    sig = fabsf(sig);
                }
            }

            if (!tone_pre) {
                tone.Process(sig);
                sig = tone.Low();
            }

            float out_sample = mix.Process(dry, sig);
            out_sample = SoftClip(out_sample);

            if (pdip[0]) {
                out[0][i] = out_sample;
                out[1][i] = out_sample;
            } else {
                out[0][i] = out_sample;
                out[1][i] = out_sample;
            }
        }
    }
}


int main(void)
{
    float samplerate;

    hw.Init();
    samplerate = hw.AudioSampleRate();

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
    //  Bit Depth  |  Cutoff  |  Resonance
    // Sample Rate | Ring Mod |    Mix
    knobBitDepth.Init(hw.knob[Funbox::KNOB_1], 0.0f, 1.0f, Parameter::LINEAR);
    knobCutoff.Init(hw.knob[Funbox::KNOB_2], 200.0f, 16000.0f, Parameter::LOGARITHMIC);
    knobRes.Init(hw.knob[Funbox::KNOB_3], 0.0f, 0.95f, Parameter::LINEAR);
    knobSampleRate.Init(hw.knob[Funbox::KNOB_4], 0.0f, 0.9f, Parameter::LINEAR);
    knobRingFreq.Init(hw.knob[Funbox::KNOB_5], 0.0f, 1.0f, Parameter::LOGARITHMIC);
    knobMix.Init(hw.knob[Funbox::KNOB_6], 0.0f, 1.0f, Parameter::LINEAR);

    // DSP modules
    carrier.Init(samplerate);
    carrier.SetFreq(440.0f);
    carrier.SetAmp(1.0f);
    carrier.SetWaveform(Oscillator::WAVE_SIN);

    decimator.Init();
    decimator.SetSmoothCrushing(true);

    tone.Init(samplerate);
    tone.SetFreq(16000.0f);
    tone.SetRes(0.1f);

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
