#pragma once
#include <cmath>

class EnvelopeFollower
{
  public:
    EnvelopeFollower() {}
    ~EnvelopeFollower() {}

    void Init(float sample_rate)
    {
        sr_  = sample_rate;
        env_ = 0.f;
        SetAttack(0.001f);
        SetRelease(0.05f);
    }

    void SetAttack(float seconds)
    {
        if (seconds < 0.0001f) seconds = 0.0001f;
        attack_coeff_ = 1.f - expf(-1.f / (seconds * sr_));
    }

    void SetRelease(float seconds)
    {
        if (seconds < 0.001f) seconds = 0.001f;
        release_coeff_ = 1.f - expf(-1.f / (seconds * sr_));
    }

    float Process(float in)
    {
        float rect  = fabsf(in);
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
};
