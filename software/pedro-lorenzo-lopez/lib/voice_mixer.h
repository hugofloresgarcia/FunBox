#pragma once
#include <cmath>

namespace daisysp
{

class VoiceMixer
{
  public:
    VoiceMixer() {}
    ~VoiceMixer() {}

    void Init(float sample_rate)
    {
        sr_            = sample_rate;
        square_level_  = 0.f;
        master_level_  = 0.f;
        sub_level_     = 0.f;
        clean_blend_   = false;
        dc_prev_in_    = 0.f;
        dc_prev_out_   = 0.f;
        // DC-block coefficient: ~20 Hz cutoff at 48 kHz
        dc_coeff_      = 1.f - (126.f / sample_rate);
    }

    void SetSquareLevel(float l)  { square_level_ = l; }
    void SetMasterLevel(float l)  { master_level_ = l; }
    void SetSubLevel(float l)     { sub_level_    = l; }
    void SetCleanBlend(bool on)   { clean_blend_  = on; }

    /// Mix all voices and produce one output sample.
    /// clean_in: the original dry guitar signal
    float Process(float square_in, float master_in, float sub_in, float clean_in)
    {
        static constexpr float kVoiceAtten = 0.35f;

        float mix = square_in * square_level_ * kVoiceAtten
                  + master_in * master_level_ * kVoiceAtten
                  + sub_in    * sub_level_    * kVoiceAtten;

        if (clean_blend_)
            mix += clean_in * 0.7f;

        // DC blocking
        float dc_out = mix - dc_prev_in_ + dc_coeff_ * dc_prev_out_;
        dc_prev_in_  = mix;
        dc_prev_out_ = dc_out;

        return SoftClip(dc_out);
    }

  private:
    static inline float SoftClip(float x)
    {
        return tanhf(x);
    }

    float sr_;
    float square_level_;
    float master_level_;
    float sub_level_;
    bool  clean_blend_;
    float dc_prev_in_;
    float dc_prev_out_;
    float dc_coeff_;
};

} // namespace daisysp
