#include "hid/parameter.h"
#include <cmath>

namespace daisy
{

void Parameter::Init(AnalogControl input, float min, float max, Curve curve)
{
    in_      = input;
    pmin_    = min;
    pmax_    = max;
    pcurve_  = curve;
    lmin_    = logf(min < 0.001f ? 0.001f : min);
    lmax_    = logf(max < 0.001f ? 0.001f : max);
    val_     = min;
}

float Parameter::Process()
{
    float raw = in_.Process();
    switch (pcurve_)
    {
        case LINEAR:
            val_ = pmin_ + raw * (pmax_ - pmin_);
            break;
        case EXPONENTIAL:
            val_ = pmin_ + (raw * raw) * (pmax_ - pmin_);
            break;
        case LOGARITHMIC:
            val_ = expf(lmin_ + raw * (lmax_ - lmin_));
            break;
        case CUBE:
            val_ = pmin_ + (raw * raw * raw) * (pmax_ - pmin_);
            break;
        default:
            val_ = raw;
            break;
    }
    return val_;
}

} // namespace daisy
