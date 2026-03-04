#pragma once
#include <cmath>

namespace daisysp
{

class EnvelopeFollower
{
  public:
    EnvelopeFollower() {}
    ~EnvelopeFollower() {}

    void Init(float sample_rate)
    {
        sr_      = sample_rate;
        env_     = 0.f;
        enabled_ = false;
        SetAttack(0.001f);
        SetRelease(0.05f);
    }

    /// Attack time in seconds
    void SetAttack(float seconds)
    {
        if (seconds < 0.0001f) seconds = 0.0001f;
        attack_coeff_ = 1.f - expf(-1.f / (seconds * sr_));
    }

    /// Release time in seconds
    void SetRelease(float seconds)
    {
        if (seconds < 0.001f) seconds = 0.001f;
        release_coeff_ = 1.f - expf(-1.f / (seconds * sr_));
    }

    void SetEnabled(bool e) { enabled_ = e; }
    bool IsEnabled() const  { return enabled_; }

    /// Process one sample; returns the envelope value (0..1)
    float Process(float in)
    {
        float rect = fabsf(in);
        float coeff = (rect > env_) ? attack_coeff_ : release_coeff_;
        env_ += coeff * (rect - env_);
        return env_;
    }

    float GetEnvelope() const { return env_; }

  private:
    float sr_;
    float env_;
    float attack_coeff_;
    float release_coeff_;
    bool  enabled_;
};

} // namespace daisysp
