#pragma once
#ifndef SUBNUP_MULTIRATE_H
#define SUBNUP_MULTIRATE_H

// ────────────────────────────────────────────────────────────────────────────
// Multirate filters for 6x decimation (48 kHz → 8 kHz) and interpolation
// (8 kHz → 48 kHz).
//
// Ported from terrarium-poly-octave by Steve Schulteis (MIT license).
// FIR coefficients designed for:
//   Decimator stage 1: 48 kHz, passband 0-1800 Hz, stopband 8-24 kHz (-80 dB)
//   Decimator stage 2: half-band, 16 kHz, passband 0-1800 Hz
//   Interpolator: inverse of decimator, with gain compensation
// ────────────────────────────────────────────────────────────────────────────

#include <cstddef>

#include "ring_buffer.h"

static constexpr size_t kResampleFactor = 6;

// ────────────────────────────────────────────────────────────────────────────
// Decimator: 48 kHz → 8 kHz (factor of 6, two-stage polyphase)
// ────────────────────────────────────────────────────────────────────────────

class Decimator
{
  public:
    float Process(const float* in6)
    {
        buf1_.Push(in6[0]);
        buf1_.Push(in6[1]);
        buf1_.Push(in6[2]);
        buf2_.Push(filter1());

        buf1_.Push(in6[3]);
        buf1_.Push(in6[4]);
        buf1_.Push(in6[5]);
        buf2_.Push(filter1());

        return filter2();
    }

  private:
    // 48 kHz → 16 kHz (decimate by 3)
    // Passband 0-1800 Hz (3 dB ripple), stopband 8-24 kHz (-80 dB)
    float filter1()
    {
        static constexpr size_t off = 32 - 21;
        return
            0.000066177472224418f    * (buf1_[off+0]  + buf1_[off+20]) +
            0.0009613901552378511f   * (buf1_[off+1]  + buf1_[off+19]) +
            0.003835090815380887f    * (buf1_[off+2]  + buf1_[off+18]) +
            0.010496532623165526f    * (buf1_[off+3]  + buf1_[off+17]) +
            0.02272703591356282f     * (buf1_[off+4]  + buf1_[off+16]) +
            0.041464390530886956f    * (buf1_[off+5]  + buf1_[off+15]) +
            0.06591039391505207f     * (buf1_[off+6]  + buf1_[off+14]) +
            0.09309984953947406f     * (buf1_[off+7]  + buf1_[off+13]) +
            0.11829177835273737f     * (buf1_[off+8]  + buf1_[off+12]) +
            0.13620590247679107f     * (buf1_[off+9]  + buf1_[off+11]) +
            0.14270010010002276f     *  buf1_[off+10];
    }

    // 16 kHz → 8 kHz (half-band, decimate by 2)
    // Passband 0-1800 Hz
    float filter2()
    {
        static constexpr size_t off = 16 - 15;
        return
            -0.00299995f  * (buf2_[off+0]  + buf2_[off+14]) +
             0.01858487f  * (buf2_[off+2]  + buf2_[off+12]) +
            -0.06984829f  * (buf2_[off+4]  + buf2_[off+10]) +
             0.30421664f  * (buf2_[off+6]  + buf2_[off+8])  +
             0.5f         *  buf2_[off+7];
    }

    RingBuffer<32> buf1_;
    RingBuffer<16> buf2_;
};

// ────────────────────────────────────────────────────────────────────────────
// Interpolator: 8 kHz → 48 kHz (factor of 6, two-stage polyphase)
// ────────────────────────────────────────────────────────────────────────────

class Interpolator
{
  public:
    // Takes 1 sample at 8 kHz, writes 6 samples at 48 kHz into out6.
    void Process(float sample, float* out6)
    {
        buf1_.Push(sample);

        buf2_.Push(filter1a());
        out6[0] = filter2a();
        out6[1] = filter2b();
        out6[2] = filter2c();

        buf2_.Push(filter1b());
        out6[3] = filter2a();
        out6[4] = filter2b();
        out6[5] = filter2c();
    }

  private:
    // ~~~~~ Stage 1: 8 kHz → 16 kHz (polyphase, two sub-filters) ~~~~~
    // Passband 0-3600 Hz (3 dB ripple), stopband 4400-8000 Hz (-80 dB)
    // Gain = 2 in passband

    float filter1a()
    {
        static constexpr size_t off = 32 - 25;
        return
            -0.0028536199247471473f  * (buf1_[off+0]  + buf1_[off+24]) +
            -0.040326725115203695f   * (buf1_[off+1]  + buf1_[off+23]) +
            -0.036134596458820015f   * (buf1_[off+2]  + buf1_[off+22]) +
             0.033522051189265496f   * (buf1_[off+3]  + buf1_[off+21]) +
            -0.031442224275585025f   * (buf1_[off+4]  + buf1_[off+20]) +
             0.03258337681750486f    * (buf1_[off+5]  + buf1_[off+19]) +
            -0.03538414864961937f    * (buf1_[off+6]  + buf1_[off+18]) +
             0.038811868988079715f   * (buf1_[off+7]  + buf1_[off+17]) +
            -0.042204493894155204f   * (buf1_[off+8]  + buf1_[off+16]) +
             0.045128824129776035f   * (buf1_[off+9]  + buf1_[off+15]) +
            -0.04736995557907843f    * (buf1_[off+10] + buf1_[off+14]) +
             0.048831901671617876f   * (buf1_[off+11] + buf1_[off+13]) +
             0.9507771467941135f     *  buf1_[off+12];
    }

    float filter1b()
    {
        static constexpr size_t off = 32 - 25;
        return
            -0.015961858776449508f   * (buf1_[off+0]  + buf1_[off+23]) +
            -0.056128740058266235f   * (buf1_[off+1]  + buf1_[off+22]) +
             0.011026026040094625f   * (buf1_[off+2]  + buf1_[off+21]) +
             0.003198795994721635f   * (buf1_[off+3]  + buf1_[off+20]) +
            -0.01108582057161854f    * (buf1_[off+4]  + buf1_[off+19]) +
             0.01951384497860086f    * (buf1_[off+5]  + buf1_[off+18]) +
            -0.030860282826182514f   * (buf1_[off+6]  + buf1_[off+17]) +
             0.04707993944078406f    * (buf1_[off+7]  + buf1_[off+16]) +
            -0.07155908583004919f    * (buf1_[off+8]  + buf1_[off+15]) +
             0.1129220770668398f     * (buf1_[off+9]  + buf1_[off+14]) +
            -0.2033122562119347f     * (buf1_[off+10] + buf1_[off+13]) +
             0.6336728217960803f     * (buf1_[off+11] + buf1_[off+12]);
    }

    // ~~~~~ Stage 2: 16 kHz → 48 kHz (polyphase, three sub-filters) ~~~~~
    // Passband 0-3600 Hz (3 dB ripple), stopband 8-24 kHz (-80 dB)
    // Gain = 3 in passband

    float filter2a()
    {
        static constexpr size_t off = 16 - 11;
        return
             0.00036440608905813593f *  buf2_[off+0]  +
             0.0005821260464558225f  *  buf2_[off+1]  +
            -0.043244023722481956f   *  buf2_[off+2]  +
            -0.10310036386076359f    *  buf2_[off+3]  +
             0.13604229993913602f    *  buf2_[off+4]  +
             0.5503466630244301f     *  buf2_[off+5]  +
             0.4407091552750118f     *  buf2_[off+6]  +
             0.009420000864297772f   *  buf2_[off+7]  +
            -0.09801301258361905f    *  buf2_[off+8]  +
            -0.019627176246818184f   *  buf2_[off+9]  +
             0.001762424830497545f   *  buf2_[off+10];
    }

    float filter2b()
    {
        static constexpr size_t off = 16 - 11;
        return
             0.001112114188613258f   * (buf2_[off+0]  + buf2_[off+10]) +
            -0.005449383064836152f   * (buf2_[off+1]  + buf2_[off+9])  +
            -0.07276547446584428f    * (buf2_[off+2]  + buf2_[off+8])  +
            -0.0709695783332148f     * (buf2_[off+3]  + buf2_[off+7])  +
             0.2904591843823435f     * (buf2_[off+4]  + buf2_[off+6])  +
             0.590541634315722f      *  buf2_[off+5];
    }

    float filter2c()
    {
        static constexpr size_t off = 16 - 11;
        return
             0.001762424830497545f   *  buf2_[off+0]  +
            -0.019627176246818184f   *  buf2_[off+1]  +
            -0.09801301258361905f    *  buf2_[off+2]  +
             0.009420000864297772f   *  buf2_[off+3]  +
             0.4407091552750118f     *  buf2_[off+4]  +
             0.5503466630244301f     *  buf2_[off+5]  +
             0.13604229993913602f    *  buf2_[off+6]  +
            -0.10310036386076359f    *  buf2_[off+7]  +
            -0.043244023722481956f   *  buf2_[off+8]  +
             0.0005821260464558225f  *  buf2_[off+9]  +
             0.00036440608905813593f *  buf2_[off+10];
    }

    RingBuffer<32> buf1_;
    RingBuffer<16> buf2_;
};

#endif // SUBNUP_MULTIRATE_H
