#include "daisy_petal.h"
#include "daisysp.h"
#include "funbox.h"
#include "mathutils.h"
#include "delay.h"

using namespace daisy;
using namespace daisysp;
using namespace funbox;

// ────────────────────────────────────────────────────────────────────────────
// HARDWARE
// ────────────────────────────────────────────────────────────────────────────

DaisyPetal hw;
Led        led1, led2;

// Knob parameters (all 0..1 linear; final scaling done inline to match
// the original ChivoDub math exactly)
Parameter knob_lfo_speed_p,   // KNOB_1: LFO rate
          knob_lfo_amt_p,     // KNOB_2: LFO depth
          knob_octave_p,      // KNOB_3: octave range
          knob_cutoff_p,      // KNOB_4: filter cutoff
          knob_delay_time_p,  // KNOB_5: delay time
          knob_delay_fb_p;    // KNOB_6: delay feedback

// 3-way switch and DIP state (previous + index arrays, Cenote pattern)
bool pswitch1[2], pswitch2[2], pswitch3[2], pdip[4];
int  switch1[2],  switch2[2],  switch3[2],  dip[4];

// ────────────────────────────────────────────────────────────────────────────
// DSP
// ────────────────────────────────────────────────────────────────────────────

Adsr          env;
Svf           filter;
Oscillator    osc;
WaveGenerator freq_lfo;
DelayEngine   delay_eng;

static float last_lfo               = 0.0f;
static float last_lfo_frequency     = 0.0f;
static float base_octave_multiplier = 1.0f;

// ────────────────────────────────────────────────────────────────────────────
// RUNTIME STATE
// ────────────────────────────────────────────────────────────────────────────

bool hold_active = false;

// ────────────────────────────────────────────────────────────────────────────
// SWITCH CALLBACKS
// ────────────────────────────────────────────────────────────────────────────

// SWITCH_1: oscillator waveform — LEFT=POLYBLEP_SAW, center=SIN, RIGHT=POLYBLEP_SQUARE
void updateSwitch1()
{
    if (pswitch1[0])
        osc.SetWaveform(Oscillator::WAVE_POLYBLEP_SAW);
    else if (pswitch1[1])
        osc.SetWaveform(Oscillator::WAVE_POLYBLEP_SQUARE);
    else
        osc.SetWaveform(Oscillator::WAVE_SIN);
}

// SWITCH_2: LFO waveform — LEFT=SAW, center=SIN, RIGHT=SQUARE
void updateSwitch2()
{
    if (pswitch2[0])
        freq_lfo.SetWaveform(WaveGenerator::WAVE_SAW);
    else if (pswitch2[1])
        freq_lfo.SetWaveform(WaveGenerator::WAVE_SQUARE);
    else
        freq_lfo.SetWaveform(WaveGenerator::WAVE_SIN);
}

// SWITCH_3 and DIP states are read directly in the callback (no side effects)

// ────────────────────────────────────────────────────────────────────────────
// HARDWARE IO PROCESSING
// ────────────────────────────────────────────────────────────────────────────

void UpdateButtons()
{
    // FOOTSWITCH_1: momentary gate — retrigger ADSR and reset LFO phase on press
    if (hw.switches[Funbox::FOOTSWITCH_1].RisingEdge()) {
        env.Retrigger(true);
        freq_lfo.Reset(0.0f);
    }

    // FOOTSWITCH_2: toggle hold/latch
    if (hw.switches[Funbox::FOOTSWITCH_2].RisingEdge())
        hold_active = !hold_active;
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

    // SWITCH_3: no callback — read inline in AudioCallback
    for (int i = 0; i < 2; i++) {
        if (hw.switches[switch3[i]].Pressed() != pswitch3[i])
            pswitch3[i] = hw.switches[switch3[i]].Pressed();
    }

    for (int i = 0; i < 4; i++) {
        if (hw.switches[dip[i]].Pressed() != pdip[i])
            pdip[i] = hw.switches[dip[i]].Pressed();
    }
}

// ────────────────────────────────────────────────────────────────────────────
// DSP HELPERS
// ────────────────────────────────────────────────────────────────────────────

// Advance LFO one block, compute oscillator pitch, and return frequency in Hz.
// Knob values are normalised 0..1 (CCW=0, CW=1).
//   lfo_speed_knob : maps to 0.01–500 Hz
//   lfo_amt_knob   : maps to LFO amplitude 0–1 (CW = full depth)
//   octave_knob    : maps to pitch base 0.125×–4× of the 100–1600 Hz LFO sweep
float calc_osc_freq(float lfo_speed_knob, float lfo_amt_knob, float octave_knob)
{
    bool krazy = pswitch3[0]; // SWITCH_3_LEFT enables audio-rate FM (×32)

    float raw_hz        = linlin(lfo_speed_knob, 0.f, 1.f, 0.01f, 500.f);
    float lfo_frequency = krazy ? raw_hz * 32.f : raw_hz;
    last_lfo_frequency  = lfo_frequency;

    freq_lfo.SetFreq(lfo_frequency);
    freq_lfo.SetAmp(lfo_amt_knob);

    float lfo_val = freq_lfo.Process();
    last_lfo = lfo_val;

    float base    = linlin(octave_knob, 0.f, 1.f, 0.125f, 4.f);
    float freq_hz = linlin(lfo_val, -1.f, 1.f, 100.f, 1600.f) * base;

    return freq_hz * base_octave_multiplier;
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

    // Read all knobs once per block
    float lfo_speed_knob  = knob_lfo_speed_p.Process();
    float lfo_amt_knob    = knob_lfo_amt_p.Process();
    float octave_knob     = knob_octave_p.Process();
    float cutoff_knob     = knob_cutoff_p.Process();
    float delay_time_knob = knob_delay_time_p.Process();
    float delay_fb_knob   = knob_delay_fb_p.Process();

    // Gate: FOOTSWITCH_1 held (momentary) OR hold latch active
    bool gate = hw.switches[Funbox::FOOTSWITCH_1].Pressed() || hold_active;

    // Base octave shift — only meaningful at audio-rate LFO (> 15 Hz):
    //   SWITCH_3_RIGHT → ×1.5 (octave up)
    //   DIP_1          → ×0.5 (octave down)
    if (last_lfo_frequency > 15.0f) {
        if (pswitch3[1])
            base_octave_multiplier = 1.5f;
        else if (pdip[0])
            base_octave_multiplier = 0.5f;
        else
            base_octave_multiplier = 1.0f;
    } else {
        base_octave_multiplier = 1.0f;
    }

    osc.SetFreq(calc_osc_freq(lfo_speed_knob, lfo_amt_knob, octave_knob));

    // Advance envelope once per block (attack/decay/release are ~0.1 ms so this is fine)
    float env_val = env.Process(gate);
    osc.SetAmp(env_val);

    // Delay engine parameters
    delay_eng.SetDelayMs(linlin(delay_time_knob, 0.f, 1.f, 50.f, 2000.f));
    delay_eng.SetFeedback(linlin(delay_fb_knob,  0.f, 1.f, 0.f,  0.999f));
    // Bypass (soft fade to dry) when feedback knob is fully off
    delay_eng.SetBypass(delay_fb_knob < 0.05f);

    filter.SetFreq(linlin(cutoff_knob, 0.f, 1.f, 200.f, 16000.f));

    // LED_1 tracks the envelope amplitude; LED_2 pulses with LFO sign
    led1.Set(env_val);
    led2.Set((last_lfo >= 0.0f) ? 1.0f : 0.0f);
    led1.Update();
    led2.Update();

    for (size_t i = 0; i < size; i++) {
        float sample = osc.Process();

        // Attenuate bandlimited saw/square to match sine RMS (~−3 dB)
        if (pswitch1[0] || pswitch1[1]) {
            sample *= 0.707f;
        }

        filter.Process(sample);
        sample = filter.Low();
        sample = SoftClip(sample);
        sample = delay_eng.Process(sample);

        out[0][i] = sample;
        out[1][i] = sample;
    }
}

// ────────────────────────────────────────────────────────────────────────────
// MAIN
// ────────────────────────────────────────────────────────────────────────────

int main(void)
{
    hw.Init();
    float sr = hw.AudioSampleRate();

    hw.SetAudioBlockSize(64);

    // 3-way switch and DIP switch pin index mapping
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
    for (int i = 0; i < 4; i++) pdip[i] = false;

    // All knobs: 0..1 linear; final Hz/ms/unit scaling happens in the callback
    knob_lfo_speed_p .Init(hw.knob[Funbox::KNOB_1], 0.0f, 1.0f, Parameter::LINEAR); // LFO rate
    knob_lfo_amt_p   .Init(hw.knob[Funbox::KNOB_2], 0.0f, 1.0f, Parameter::LINEAR); // LFO depth
    knob_octave_p    .Init(hw.knob[Funbox::KNOB_3], 0.0f, 1.0f, Parameter::LINEAR); // octave range
    knob_cutoff_p    .Init(hw.knob[Funbox::KNOB_4], 0.0f, 1.0f, Parameter::LINEAR); // filter cutoff
    knob_delay_time_p.Init(hw.knob[Funbox::KNOB_5], 0.0f, 1.0f, Parameter::LINEAR); // delay time
    knob_delay_fb_p  .Init(hw.knob[Funbox::KNOB_6], 0.0f, 1.0f, Parameter::LINEAR); // delay feedback

    // LEDs
    led1.Init(hw.seed.GetPin(Funbox::LED_1), false);
    led1.Update();
    led2.Init(hw.seed.GetPin(Funbox::LED_2), false);
    led2.Update();

    // DSP init
    osc.Init(sr);
    osc.SetWaveform(Oscillator::WAVE_SIN);
    osc.SetAmp(0.0f);

    env.Init(sr);
    env.SetAttackTime(0.0001f);
    env.SetDecayTime(0.0001f);
    env.SetSustainLevel(1.0f);
    env.SetReleaseTime(0.0001f);

    freq_lfo.Init(sr);
    freq_lfo.SetWaveform(WaveGenerator::WAVE_SIN);

    delay_eng.Init(sr, 80.0f); // 80 ms wet fade time (matches original)
    filter.Init(sr);

    hw.StartAdc();
    hw.StartAudio(AudioCallback);

    while (1) {
        System::Delay(10);
    }
}
