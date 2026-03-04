#pragma once
#ifndef DSY_SEED_H
#define DSY_SEED_H

#include "daisy_core.h"
#include "sys/system.h"
#include "per/gpio.h"
#include "hid/audio.h"
#include "hid/ctrl.h"
#include "hid/led.h"
#include "hid/encoder.h"
#include "hid/midi.h"

namespace daisy
{

class QSPIHandle { public: };
class SdramHandle { public: };
class DacHandle { public: };
class UsbHandle { public: };
class AdcHandle
{
  public:
    void Init() {}
    void Start() {}
    void Stop() {}
    uint16_t *GetPtr(uint8_t chn) { (void)chn; return nullptr; }
};

class DaisySeed
{
  public:
    DaisySeed() {}
    ~DaisySeed() {}

    void Configure() {}
    void Init(bool boost = false) { (void)boost; }
    void DeInit() {}
    void DelayMs(size_t del);

    static dsy_gpio_pin GetPin(uint8_t pin_idx);

    void StartAudio(AudioHandle::InterleavingAudioCallback cb);
    void StartAudio(AudioHandle::AudioCallback cb);
    void ChangeAudioCallback(AudioHandle::InterleavingAudioCallback cb);
    void ChangeAudioCallback(AudioHandle::AudioCallback cb);
    void StopAudio() {}

    void SetAudioSampleRate(SaiHandle::Config::SampleRate samplerate) { (void)samplerate; }
    float AudioSampleRate();
    void SetAudioBlockSize(size_t blocksize);
    size_t AudioBlockSize();
    float AudioCallbackRate() const;

    void SetLed(bool state) { (void)state; }
    void SetTestPoint(bool state) { (void)state; }

    QSPIHandle    qspi;
    SdramHandle   sdram_handle;
    AudioHandle   audio_handle;
    AdcHandle     adc;
    DacHandle     dac;
    UsbHandle     usb_handle;
    dsy_gpio      led = {}, testpoint = {};
    System        system;
};

namespace seed
{
    constexpr Pin D0  = Pin(PORTB, 12);
    constexpr Pin D1  = Pin(PORTC, 11);
    constexpr Pin D2  = Pin(PORTC, 10);
    constexpr Pin D3  = Pin(PORTC, 9);
    constexpr Pin D4  = Pin(PORTC, 8);
    constexpr Pin D5  = Pin(PORTD, 2);
    constexpr Pin D6  = Pin(PORTC, 12);
    constexpr Pin D7  = Pin(PORTG, 10);
    constexpr Pin D8  = Pin(PORTG, 11);
    constexpr Pin D9  = Pin(PORTB, 4);
    constexpr Pin D10 = Pin(PORTB, 5);
    constexpr Pin D11 = Pin(PORTB, 8);
    constexpr Pin D12 = Pin(PORTB, 9);
    constexpr Pin D13 = Pin(PORTB, 6);
    constexpr Pin D14 = Pin(PORTB, 7);
    constexpr Pin D15 = Pin(PORTC, 0);
    constexpr Pin D16 = Pin(PORTA, 3);
    constexpr Pin D17 = Pin(PORTB, 1);
    constexpr Pin D18 = Pin(PORTA, 7);
    constexpr Pin D19 = Pin(PORTA, 6);
    constexpr Pin D20 = Pin(PORTC, 1);
    constexpr Pin D21 = Pin(PORTC, 4);
    constexpr Pin D22 = Pin(PORTA, 5);
    constexpr Pin D23 = Pin(PORTA, 4);
    constexpr Pin D24 = Pin(PORTA, 1);
    constexpr Pin D25 = Pin(PORTA, 0);
    constexpr Pin D26 = Pin(PORTD, 11);
    constexpr Pin D27 = Pin(PORTG, 9);
    constexpr Pin D28 = Pin(PORTA, 2);
    constexpr Pin D29 = Pin(PORTB, 14);
    constexpr Pin D30 = Pin(PORTB, 15);
}

} // namespace daisy

#endif
