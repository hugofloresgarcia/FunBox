#pragma once
#ifndef DSY_PETAL_H
#define DSY_PETAL_H

#include "daisy_seed.h"
#include "hid/switch.h"
#include "hid/parameter.h"

namespace daisy
{

class DaisyPetal
{
  public:
    enum Sw
    {
        SW_1, SW_2, SW_3, SW_4, SW_5, SW_6, SW_7, SW_8,
        SW_9, SW_10, SW_11, SW_12, SW_LAST,
    };

    enum Knob
    {
        KNOB_1, KNOB_2, KNOB_3, KNOB_4, KNOB_5, KNOB_6, KNOB_LAST,
    };

    enum RingLed
    {
        RING_LED_1, RING_LED_2, RING_LED_3, RING_LED_4,
        RING_LED_5, RING_LED_6, RING_LED_7, RING_LED_8,
        RING_LED_LAST,
    };

    enum FootswitchLed
    {
        FOOTSWITCH_LED_1, FOOTSWITCH_LED_2,
        FOOTSWITCH_LED_3, FOOTSWITCH_LED_4,
        FOOTSWITCH_LED_LAST,
    };

    DaisyPetal() {}
    ~DaisyPetal() {}

    void Init(bool boost = false);
    void DelayMs(size_t del);

    void StartAudio(AudioHandle::InterleavingAudioCallback cb);
    void StartAudio(AudioHandle::AudioCallback cb);
    void ChangeAudioCallback(AudioHandle::InterleavingAudioCallback cb);
    void ChangeAudioCallback(AudioHandle::AudioCallback cb);
    void StopAudio();

    void SetAudioSampleRate(SaiHandle::Config::SampleRate samplerate) { (void)samplerate; }
    float AudioSampleRate();
    void SetAudioBlockSize(size_t size);
    size_t AudioBlockSize();
    float AudioCallbackRate();

    void StartAdc() {}
    void StopAdc() {}

    void ProcessAnalogControls();
    void ProcessDigitalControls();
    inline void ProcessAllControls() { ProcessAnalogControls(); ProcessDigitalControls(); }

    float GetKnobValue(Knob k);
    float GetExpression();

    void InitMidi() {}
    void ClearLeds() {}
    void UpdateLeds() {}
    void SetRingLed(RingLed idx, float r, float g, float b) { (void)idx; (void)r; (void)g; (void)b; }
    void SetFootswitchLed(FootswitchLed idx, float bright) { (void)idx; (void)bright; }

    DaisySeed     seed;
    Encoder       encoder;
    AnalogControl knob[KNOB_LAST];
    AnalogControl expression;
    Switch        switches[SW_LAST];
    RgbLed        ring_led[8];
    Led           footswitch_led[4];
    MidiUartHandler midi;

    AudioHandle::AudioCallback             audio_callback_     = nullptr;
    AudioHandle::InterleavingAudioCallback interleaving_callback_ = nullptr;

  private:
    float    sample_rate_ = 48000.f;
    size_t   block_size_  = 48;
    uint16_t adc_values_[KNOB_LAST + 1] = {};
};

} // namespace daisy

#endif
