#pragma once
#include <stdint.h>

namespace daisy
{

class AnalogControl
{
  public:
    AnalogControl() {}
    ~AnalogControl() {}

    void Init(uint16_t *adcptr, float sr,
              bool flip = false, bool invert = false, float slew_seconds = 0.002f);

    float Process();
    inline float Value() const { return val_; }
    inline uint16_t GetRawValue() { return raw_ ? *raw_ : 0; }
    inline float GetRawFloat() { return raw_ ? (float)(*raw_) / 65535.f : 0.f; }
    void SetSampleRate(float sample_rate);

    uint16_t *raw_ = nullptr;
    float     val_  = 0.f;

  private:
    float coeff_        = 0.f;
    float samplerate_   = 48000.f;
    float slew_seconds_ = 0.002f;
    bool  flip_         = false;
    bool  invert_       = false;
};

} // namespace daisy
