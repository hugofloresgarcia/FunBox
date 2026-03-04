#pragma once
#include <stdint.h>
#include "hid/ctrl.h"

namespace daisy
{

class Parameter
{
  public:
    enum Curve { LINEAR, EXPONENTIAL, LOGARITHMIC, CUBE, LAST };

    Parameter() {}
    ~Parameter() {}

    void Init(AnalogControl input, float min, float max, Curve curve);
    float Process();
    inline float Value() { return val_; }

  private:
    AnalogControl in_;
    float         pmin_ = 0.f, pmax_ = 1.f;
    float         lmin_ = 0.f, lmax_ = 1.f;
    float         val_  = 0.f;
    Curve         pcurve_ = LINEAR;
};

} // namespace daisy
