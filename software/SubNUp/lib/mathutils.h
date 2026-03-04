#pragma once
#ifndef SUBNUP_MATHUTILS_H
#define SUBNUP_MATHUTILS_H

#include <cmath>

inline float linlin(float x, float a, float b, float c, float d)
{
    if (a == b) return c;
    if ((b > a && x <= a) || (b < a && x >= a)) return c;
    if ((b > a && x >= b) || (b < a && x <= b)) return d;
    float t = (x - a) / (b - a);
    return c + t * (d - c);
}

inline float linexp(float x, float a, float b, float c, float d)
{
    if (c <= 0.f || d <= 0.f || a == b) return c;
    if ((b > a && x <= a) || (b < a && x >= a)) return c;
    if ((b > a && x >= b) || (b < a && x <= b)) return d;
    float t = (x - a) / (b - a);
    return c * powf(d / c, t);
}

#endif // SUBNUP_MATHUTILS_H
