#pragma once
#ifndef SUBNUP_RING_BUFFER_H
#define SUBNUP_RING_BUFFER_H

#include <cstddef>
#include <cstdint>

// ────────────────────────────────────────────────────────────────────────────
// Fixed-size ring buffer for FIR filter delay lines.
// N must be a power of 2.
// ────────────────────────────────────────────────────────────────────────────

template <size_t N>
class RingBuffer
{
  public:
    void Push(float val)
    {
        buf_[pos_ & kMask] = val;
        ++pos_;
    }

    // Index 0 is the oldest sample, N-1 is the most recent.
    float operator[](size_t i) const
    {
        return buf_[(pos_ - N + i) & kMask];
    }

    void Clear()
    {
        for (size_t i = 0; i < N; ++i)
            buf_[i] = 0.f;
        pos_ = 0;
    }

  private:
    static constexpr size_t kMask = N - 1;
    float buf_[N] = {};
    uint32_t pos_ = 0;
};

#endif // SUBNUP_RING_BUFFER_H
