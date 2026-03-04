#pragma once
#ifndef SUBNUP_OCTAVE_ENGINE_H
#define SUBNUP_OCTAVE_ENGINE_H

// ────────────────────────────────────────────────────────────────────────────
// OctaveEngine -- 80-band filter bank for polyphonic octave shifting.
//
// Ported from terrarium-poly-octave by Steve Schulteis (MIT license).
// Filter spacing follows an exponential curve tuned to match the
// TC Electronic Sub'N'Up: f(n) = 480 * 2^(0.027*n) - 420.
// ────────────────────────────────────────────────────────────────────────────

#include <cmath>

#include "band_shifter.h"

class OctaveEngine
{
  public:
    void Init(float sample_rate)
    {
        for (int i = 0; i < kNumBands; ++i)
        {
            const float center = centerFreq(i);
            const float bw     = bandwidth(i);
            shifters_[i].Init(center, sample_rate, bw);
        }
        up1_ = down1_ = down2_ = 0.f;
    }

    void Process(float sample)
    {
        up1_   = 0.f;
        down1_ = 0.f;
        down2_ = 0.f;

        for (int i = 0; i < kNumBands; ++i)
        {
            shifters_[i].Update(sample);
            up1_   += shifters_[i].Up1();
            down1_ += shifters_[i].Down1();
            down2_ += shifters_[i].Down2();
        }
    }

    float Up1() const { return up1_; }
    float Down1() const { return down1_; }
    float Down2() const { return down2_; }

  private:
    static constexpr int32_t kNumBands = 80;

    static float centerFreq(int n)
    {
        return 480.f * powf(2.0f, 0.027f * static_cast<float>(n)) - 420.f;
    }

    static float bandwidth(int n)
    {
        const float f0 = centerFreq(n - 1);
        const float f1 = centerFreq(n);
        const float f2 = centerFreq(n + 1);
        const float a  = f2 - f1;
        const float b  = f1 - f0;
        return 2.0f * (a * b) / (a + b);
    }

    BandShifter shifters_[kNumBands];
    float up1_   = 0.f;
    float down1_ = 0.f;
    float down2_ = 0.f;
};

#endif // SUBNUP_OCTAVE_ENGINE_H
