#pragma once
#include <cmath>
#include <cstdint>

namespace daisysp
{

class PitchTracker
{
  public:
    PitchTracker() {}
    ~PitchTracker() {}

    void Init(float sample_rate)
    {
        sr_            = sample_rate;
        min_period_    = sr_ / kMaxFreqHz;
        max_period_    = sr_ / kMinFreqHz;
        frequency_     = 0.f;
        confidence_    = 0.f;
        square_out_    = 0.f;
        prev_sign_     = false;
        sample_count_  = 0;
        period_idx_    = 0;
        valid_         = false;

        for (int i = 0; i < 3; i++)
            period_buf_[i] = 0.f;
    }

    /// Feed one sample, returns true when a new frequency estimate is ready.
    bool Process(float in)
    {
        bool cur_sign = (in >= 0.f);
        square_out_   = cur_sign ? 1.f : -1.f;

        sample_count_++;
        bool new_estimate = false;

        // Rising zero-crossing detected
        if (cur_sign && !prev_sign_)
        {
            float period = static_cast<float>(sample_count_);

            if (period >= min_period_ && period <= max_period_)
            {
                period_buf_[period_idx_] = period;
                period_idx_ = (period_idx_ + 1) % 3;

                if (!valid_)
                {
                    // Need at least 3 measurements before median filtering
                    bool all_filled = true;
                    for (int i = 0; i < 3; i++)
                        if (period_buf_[i] == 0.f) all_filled = false;
                    if (all_filled)
                        valid_ = true;
                }

                if (valid_)
                {
                    float med    = median3(period_buf_[0], period_buf_[1], period_buf_[2]);
                    float spread = maxSpread(period_buf_[0], period_buf_[1], period_buf_[2]);

                    confidence_ = 1.f - fclamp(spread / med, 0.f, 1.f);

                    if (confidence_ > kConfidenceThreshold)
                    {
                        frequency_    = sr_ / med;
                        new_estimate  = true;
                    }
                }
            }
            else if (period > max_period_)
            {
                // Signal too slow or absent -- decay confidence
                confidence_ *= 0.8f;
                if (confidence_ < 0.01f)
                {
                    frequency_  = 0.f;
                    confidence_ = 0.f;
                    valid_      = false;
                    for (int i = 0; i < 3; i++)
                        period_buf_[i] = 0.f;
                }
            }

            sample_count_ = 0;
        }

        // Timeout: if we haven't seen a crossing in a very long time
        if (sample_count_ > static_cast<uint32_t>(max_period_ * 1.5f))
        {
            confidence_ *= 0.7f;
            if (confidence_ < 0.01f)
            {
                frequency_  = 0.f;
                confidence_ = 0.f;
                valid_      = false;
                for (int i = 0; i < 3; i++)
                    period_buf_[i] = 0.f;
            }
            sample_count_ = 0;
        }

        prev_sign_ = cur_sign;
        return new_estimate;
    }

    float GetFrequency()  const { return frequency_; }
    float GetConfidence() const { return confidence_; }

    /// Hard-clipped square wave of the input (+1 / -1)
    float GetSquareOut()  const { return square_out_; }

  private:
    static constexpr float kMinFreqHz           = 30.f;
    static constexpr float kMaxFreqHz           = 1500.f;
    static constexpr float kConfidenceThreshold = 0.3f;

    static inline float fclamp(float x, float lo, float hi)
    {
        return x < lo ? lo : (x > hi ? hi : x);
    }

    static inline float median3(float a, float b, float c)
    {
        if (a > b)
        {
            if (b > c) return b;
            if (a > c) return c;
            return a;
        }
        else
        {
            if (a > c) return a;
            if (b > c) return c;
            return b;
        }
    }

    static inline float maxSpread(float a, float b, float c)
    {
        float mn = a < b ? (a < c ? a : c) : (b < c ? b : c);
        float mx = a > b ? (a > c ? a : c) : (b > c ? b : c);
        return mx - mn;
    }

    float    sr_;
    float    min_period_;
    float    max_period_;
    float    frequency_;
    float    confidence_;
    float    square_out_;
    bool     prev_sign_;
    uint32_t sample_count_;
    float    period_buf_[3];
    int      period_idx_;
    bool     valid_;
};

} // namespace daisysp
