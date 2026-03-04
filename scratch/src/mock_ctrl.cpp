#include "hid/ctrl.h"
#include <cmath>

namespace daisy
{

void AnalogControl::Init(uint16_t *adcptr, float sr, bool flip, bool invert, float slew_seconds)
{
    raw_          = adcptr;
    samplerate_   = sr;
    flip_         = flip;
    invert_       = invert;
    slew_seconds_ = slew_seconds;
    val_          = 0.f;

    float tc = slew_seconds > 0.f ? slew_seconds : 0.002f;
    coeff_ = 1.f - expf(-1.f / (tc * sr));
}

float AnalogControl::Process()
{
    float raw_f = raw_ ? (float)(*raw_) / 65535.f : 0.f;
    if (flip_) raw_f = 1.f - raw_f;
    if (invert_) raw_f = -raw_f;
    val_ += coeff_ * (raw_f - val_);
    return val_;
}

void AnalogControl::SetSampleRate(float sample_rate)
{
    samplerate_ = sample_rate;
    float tc = slew_seconds_ > 0.f ? slew_seconds_ : 0.002f;
    coeff_ = 1.f - expf(-1.f / (tc * sample_rate));
}

} // namespace daisy
