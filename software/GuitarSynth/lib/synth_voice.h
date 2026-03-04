#pragma once
#include <cmath>
#include <cstdint>

#ifndef M_PI_F
#define M_PI_F 3.14159265358979323846f
#endif

#ifndef M_TWOPI_F
#define M_TWOPI_F (2.f * M_PI_F)
#endif

class SynthVoice
{
  public:
    enum Waveform { WAVE_SAW = 0, WAVE_SQUARE, WAVE_SIN, WAVE_LAST };

    SynthVoice() {}
    ~SynthVoice() {}

    void Init(float sample_rate)
    {
        sr_           = sample_rate;
        sr_recip_     = 1.f / sample_rate;
        phase_        = 0.f;
        freq_         = 0.f;
        target_freq_  = 0.f;
        target_amp_   = 0.f;
        current_amp_  = 0.f;
        waveform_     = WAVE_SAW;
        porta_coeff_  = 1.f;
        tri_integrator_ = 0.f;

        SetPortamento(0.f);
        SetAmpAttack(0.002f);
        SetAmpRelease(0.2f);
    }

    void SetWaveform(uint8_t wf) { waveform_ = wf < WAVE_LAST ? wf : WAVE_SAW; }

    void SetTargetFreq(float hz) { target_freq_ = hz; }
    void SetTargetAmp(float a)   { target_amp_ = a; }

    void SetPortamento(float seconds)
    {
        if (seconds < 0.0005f)
            porta_coeff_ = 1.f;
        else
            porta_coeff_ = 1.f - expf(-1.f / (seconds * sr_));
    }

    void SetAmpAttack(float seconds)
    {
        if (seconds < 0.0001f) seconds = 0.0001f;
        amp_attack_c_ = 1.f - expf(-1.f / (seconds * sr_));
    }

    void SetAmpRelease(float seconds)
    {
        if (seconds < 0.0005f) seconds = 0.0005f;
        amp_release_c_ = 1.f - expf(-1.f / (seconds * sr_));
    }

    /// Release immediately (voice is no longer assigned a pitch)
    void NoteOff() { target_amp_ = 0.f; }

    bool IsActive() const { return current_amp_ > 0.0001f || target_amp_ > 0.f; }

    float Process()
    {
        // Smooth amplitude
        float amp_coeff = (target_amp_ > current_amp_) ? amp_attack_c_ : amp_release_c_;
        current_amp_ += amp_coeff * (target_amp_ - current_amp_);

        if (current_amp_ < 0.0001f)
        {
            current_amp_ = 0.f;
            return 0.f;
        }

        // Portamento
        if (target_freq_ > 0.f)
        {
            if (freq_ <= 0.f)
                freq_ = target_freq_;
            else
                freq_ += porta_coeff_ * (target_freq_ - freq_);
        }

        if (freq_ <= 0.f)
            return 0.f;

        float phase_inc = freq_ * sr_recip_;
        phase_ += phase_inc;
        if (phase_ >= 1.f) phase_ -= 1.f;

        float out = 0.f;
        switch (waveform_)
        {
        case WAVE_SIN:
            out = sinf(phase_ * M_TWOPI_F);
            break;
        case WAVE_SQUARE:
            out = polyBlepSquare(phase_, phase_inc);
            break;
        case WAVE_SAW:
        default:
            out = polyBlepSaw(phase_, phase_inc);
            break;
        }

        return out * current_amp_;
    }

  private:
    static inline float polyBlep(float t, float dt)
    {
        if (t < dt)
        {
            t /= dt;
            return t + t - t * t - 1.f;
        }
        else if (t > 1.f - dt)
        {
            t = (t - 1.f) / dt;
            return t * t + t + t + 1.f;
        }
        return 0.f;
    }

    static inline float polyBlepSquare(float phase, float dt)
    {
        float val = phase < 0.5f ? 1.f : -1.f;
        val += polyBlep(phase, dt);
        val -= polyBlep(fmodf(phase + 0.5f, 1.f), dt);
        return val;
    }

    static inline float polyBlepSaw(float phase, float dt)
    {
        float val = 2.f * phase - 1.f;
        val -= polyBlep(phase, dt);
        return val;
    }

    float   sr_, sr_recip_;
    float   phase_;
    float   freq_, target_freq_;
    float   target_amp_, current_amp_;
    uint8_t waveform_;
    float   porta_coeff_;
    float   amp_attack_c_, amp_release_c_;
    float   tri_integrator_;
};
