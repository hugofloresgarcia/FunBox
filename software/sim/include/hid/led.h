#pragma once
#include "daisy_core.h"
#include "per/gpio.h"

namespace daisy
{

class Led
{
  public:
    Led() {}
    ~Led() {}

    void Init(dsy_gpio_pin pin, bool invert, float samplerate = 1000.0f) { (void)pin; (void)invert; (void)samplerate; }
    void Set(float val) { bright_ = val; }
    void Update() {}
    inline void SetSampleRate(float sample_rate) { (void)sample_rate; }

    float bright_ = 0.f;
};

class RgbLed
{
  public:
    RgbLed() {}
    ~RgbLed() {}
    void Init(dsy_gpio_pin red, dsy_gpio_pin green, dsy_gpio_pin blue, bool invert)
    {
        (void)red; (void)green; (void)blue; (void)invert;
    }
    void Set(float r, float g, float b) { r_ = r; g_ = g; b_ = b; }
    void SetRed(float r) { r_ = r; }
    void SetGreen(float g) { g_ = g; }
    void SetBlue(float b) { b_ = b; }
    float r_ = 0.f, g_ = 0.f, b_ = 0.f;
};

} // namespace daisy
