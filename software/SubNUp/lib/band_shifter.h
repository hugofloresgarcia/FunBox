#pragma once
#ifndef SUBNUP_BAND_SHIFTER_H
#define SUBNUP_BAND_SHIFTER_H

// ────────────────────────────────────────────────────────────────────────────
// BandShifter -- single-band complex bandpass filter + octave phase scaling.
//
// Ported from terrarium-poly-octave by Steve Schulteis (MIT license).
// Algorithm: ERB-PS2 from Etienne Thuillier's thesis (Aalto University, 2016).
//
// Prototype filter: LPF from Audio EQ Cookbook (Robert Bristow-Johnson),
// transformed to complex bandpass per Section 3.1 of "Complex Band-Pass
// Filters for Analytic Signal Generation" by Andrew J. Noga.
//
// Phase scaling per "Real-Time Polyphonic Octave Doubling for the Guitar"
// by Etienne Thuillier.
// ────────────────────────────────────────────────────────────────────────────

#include <cmath>
#include <complex>

#include "fast_math.h"

class BandShifter
{
  public:
    BandShifter() = default;

    void Init(float center, float sample_rate, float bw)
    {
        static constexpr double kPi = 3.14159265358979323846;
        using Cd = std::complex<double>;
        constexpr Cd j(0.0, 1.0);

        const double w0     = kPi * bw / sample_rate;
        const double cos_w0 = std::cos(w0);
        const double sin_w0 = std::sin(w0);
        const double sqrt_2 = std::sqrt(2.0);
        const double a0     = 1.0 + sqrt_2 * sin_w0 / 2.0;
        const double g      = (1.0 - cos_w0) / (2.0 * a0);

        const double w1 = 2.0 * kPi * center / sample_rate;
        const Cd e1     = std::exp(j * w1);
        const Cd e2     = std::exp(j * w1 * 2.0);

        d0_ = static_cast<float>(g);
        d1_ = Cf(static_cast<float>((e1 * 2.0 * g).real()),
                  static_cast<float>((e1 * 2.0 * g).imag()));
        d2_ = Cf(static_cast<float>((e2 * g).real()),
                  static_cast<float>((e2 * g).imag()));
        c1_ = Cf(static_cast<float>((e1 * (-2.0 * cos_w0) / a0).real()),
                  static_cast<float>((e1 * (-2.0 * cos_w0) / a0).imag()));
        c2_ = Cf(static_cast<float>((e2 * (1.0 - sqrt_2 * sin_w0 / 2.0) / a0).real()),
                  static_cast<float>((e2 * (1.0 - sqrt_2 * sin_w0 / 2.0) / a0).imag()));

        s1_ = s2_ = y_ = down1_ = Cf(0.f, 0.f);
        up1_ = down2_ = 0.f;
        down1_sign_ = down2_sign_ = 1.f;
    }

    void Update(float sample)
    {
        updateFilter(sample);
        updateUp1();
        updateDown1();
        updateDown2();
    }

    float Up1() const { return up1_; }
    float Down1() const { return down1_.real(); }
    float Down2() const { return down2_; }

  private:
    using Cf = std::complex<float>;

    // ~~~~~ Complex bandpass filter ~~~~~

    void updateFilter(float sample)
    {
        const Cf prev_y = y_;
        y_  = s2_ + d0_ * sample;
        s2_ = s1_ + d1_ * sample - c1_ * y_;
        s1_ = d2_ * sample - c2_ * y_;

        if ((y_.real() < 0.f) &&
            (std::signbit(y_.imag()) != std::signbit(prev_y.imag())))
        {
            down1_sign_ = -down1_sign_;
        }
    }

    // ~~~~~ Phase scaling: out = in * (in / |in|)^(g-1) ~~~~~

    void updateUp1()
    {
        const float a = y_.real();
        const float b = y_.imag();
        up1_ = (a * a - b * b) * fastInvSqrt(a * a + b * b);
    }

    void updateDown1()
    {
        const float a = y_.real();
        const float b = y_.imag();
        const float b_sign = (b < 0.f) ? -1.0f : 1.0f;

        const float x = 0.5f * a * fastInvSqrt(a * a + b * b);
        const float c = fastSqrt(0.5f + x);
        const float d = b_sign * fastSqrt(0.5f - x);

        const Cf prev_down1 = down1_;
        down1_ = down1_sign_ * Cf(a * c + b * d, b * c - a * d);

        if ((down1_.real() < 0.f) &&
            (std::signbit(down1_.imag()) != std::signbit(prev_down1.imag())))
        {
            down2_sign_ = -down2_sign_;
        }
    }

    void updateDown2()
    {
        const float a = down1_.real();
        const float b = down1_.imag();
        const float b_sign = (b < 0.f) ? -1.0f : 1.0f;

        const float x = 0.5f * a * fastInvSqrt(a * a + b * b);
        const float c = fastSqrt(0.5f + x);
        const float d = b_sign * fastSqrt(0.5f - x);

        down2_ = down2_sign_ * (a * c + b * d);
    }

    // Filter coefficients
    float d0_ = 0.f;
    Cf d1_, d2_, c1_, c2_;

    // Filter state
    Cf s1_, s2_, y_;

    // Shift outputs
    float up1_ = 0.f;
    Cf down1_;
    float down2_ = 0.f;

    // Sign tracking for octave-down zero-crossing detection
    float down1_sign_ = 1.f;
    float down2_sign_ = 1.f;
};

#endif // SUBNUP_BAND_SHIFTER_H
