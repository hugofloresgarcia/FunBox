#pragma once
#ifndef SUBNUP_FAST_MATH_H
#define SUBNUP_FAST_MATH_H

#include <cstdint>
#include <cstring>

// ────────────────────────────────────────────────────────────────────────────
// Fast inverse square root (Quake III style).
// Uses memcpy for standards-compliant type punning.
// ────────────────────────────────────────────────────────────────────────────

inline float fastInvSqrt(float x)
{
    float half = 0.5f * x;
    uint32_t i;
    std::memcpy(&i, &x, sizeof(i));
    i = 0x5F1FFFF9 - (i >> 1);
    float y;
    std::memcpy(&y, &i, sizeof(y));
    y *= (0.703952253f * (2.38924456f - (x * y * y)));
    return y;
}

inline float fastSqrt(float x)
{
    return fastInvSqrt(x) * x;
}

#endif // SUBNUP_FAST_MATH_H
