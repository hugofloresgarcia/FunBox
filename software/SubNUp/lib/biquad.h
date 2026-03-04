#pragma once
#ifndef SUBNUP_BIQUAD_H
#define SUBNUP_BIQUAD_H

// ────────────────────────────────────────────────────────────────────────────
// Biquad filter using Audio EQ Cookbook coefficients.
// Robert Bristow-Johnson:
//   https://webaudio.github.io/Audio-EQ-Cookbook/audio-eq-cookbook.html
//
// Supports: lowpass, highpass, low shelf, high shelf.
// Direct Form II Transposed implementation.
// ────────────────────────────────────────────────────────────────────────────

#include <cmath>

class Biquad
{
  public:
    enum Type
    {
        LOWPASS,
        HIGHPASS,
        LOW_SHELF,
        HIGH_SHELF,
    };

    void Init(float sample_rate)
    {
        sr_ = sample_rate;
        z1_ = z2_ = 0.f;
        b0_ = 1.f;
        b1_ = b2_ = a1_ = a2_ = 0.f;
    }

    void SetLowpass(float freq, float q = 0.707f)
    {
        const float w0    = 2.f * kPi * freq / sr_;
        const float cos_w = cosf(w0);
        const float sin_w = sinf(w0);
        const float alpha = sin_w / (2.f * q);

        const float a0_inv = 1.f / (1.f + alpha);
        b0_ = ((1.f - cos_w) / 2.f) * a0_inv;
        b1_ = (1.f - cos_w) * a0_inv;
        b2_ = b0_;
        a1_ = (-2.f * cos_w) * a0_inv;
        a2_ = (1.f - alpha) * a0_inv;
    }

    void SetHighpass(float freq, float q = 0.707f)
    {
        const float w0    = 2.f * kPi * freq / sr_;
        const float cos_w = cosf(w0);
        const float sin_w = sinf(w0);
        const float alpha = sin_w / (2.f * q);

        const float a0_inv = 1.f / (1.f + alpha);
        b0_ = ((1.f + cos_w) / 2.f) * a0_inv;
        b1_ = -(1.f + cos_w) * a0_inv;
        b2_ = b0_;
        a1_ = (-2.f * cos_w) * a0_inv;
        a2_ = (1.f - alpha) * a0_inv;
    }

    // gain_db: positive = boost, negative = cut
    void SetLowShelf(float freq, float gain_db, float q = 0.707f)
    {
        const float A     = powf(10.f, gain_db / 40.f);
        const float w0    = 2.f * kPi * freq / sr_;
        const float cos_w = cosf(w0);
        const float sin_w = sinf(w0);
        const float alpha = sin_w / (2.f * q);
        const float sqA2a = 2.f * sqrtf(A) * alpha;

        const float a0_inv = 1.f / ((A + 1.f) + (A - 1.f) * cos_w + sqA2a);
        b0_ = (A * ((A + 1.f) - (A - 1.f) * cos_w + sqA2a)) * a0_inv;
        b1_ = (2.f * A * ((A - 1.f) - (A + 1.f) * cos_w)) * a0_inv;
        b2_ = (A * ((A + 1.f) - (A - 1.f) * cos_w - sqA2a)) * a0_inv;
        a1_ = (-2.f * ((A - 1.f) + (A + 1.f) * cos_w)) * a0_inv;
        a2_ = ((A + 1.f) + (A - 1.f) * cos_w - sqA2a) * a0_inv;
    }

    void SetHighShelf(float freq, float gain_db, float q = 0.707f)
    {
        const float A     = powf(10.f, gain_db / 40.f);
        const float w0    = 2.f * kPi * freq / sr_;
        const float cos_w = cosf(w0);
        const float sin_w = sinf(w0);
        const float alpha = sin_w / (2.f * q);
        const float sqA2a = 2.f * sqrtf(A) * alpha;

        const float a0_inv = 1.f / ((A + 1.f) - (A - 1.f) * cos_w + sqA2a);
        b0_ = (A * ((A + 1.f) + (A - 1.f) * cos_w + sqA2a)) * a0_inv;
        b1_ = (-2.f * A * ((A - 1.f) + (A + 1.f) * cos_w)) * a0_inv;
        b2_ = (A * ((A + 1.f) + (A - 1.f) * cos_w - sqA2a)) * a0_inv;
        a1_ = (2.f * ((A - 1.f) - (A + 1.f) * cos_w)) * a0_inv;
        a2_ = ((A + 1.f) - (A - 1.f) * cos_w - sqA2a) * a0_inv;
    }

    float Process(float in)
    {
        const float out = b0_ * in + z1_;
        z1_ = b1_ * in - a1_ * out + z2_;
        z2_ = b2_ * in - a2_ * out;
        return out;
    }

    void Reset()
    {
        z1_ = z2_ = 0.f;
    }

  private:
    static constexpr float kPi = 3.14159265358979f;

    float sr_ = 48000.f;
    float b0_ = 1.f, b1_ = 0.f, b2_ = 0.f;
    float a1_ = 0.f, a2_ = 0.f;
    float z1_ = 0.f, z2_ = 0.f;
};

#endif // SUBNUP_BIQUAD_H
