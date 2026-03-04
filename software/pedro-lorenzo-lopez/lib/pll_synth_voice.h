#pragma once
#include <cmath>
#include <cstdint>

#ifndef M_PI_F
#define M_PI_F 3.14159265358979323846f
#endif

#ifndef M_TWOPI_F
#define M_TWOPI_F (2.f * M_PI_F)
#endif

namespace daisysp
{

class PllSynthVoice
{
  public:
    enum Waveform
    {
        WAVE_SQUARE = 0,
        WAVE_SAW,
        WAVE_TRI,
        WAVE_SIN,
        WAVE_LAST
    };

    enum ModMode
    {
        MOD_OFF = 0,
        MOD_GLIDE,
        MOD_VIBRATO,
    };

    PllSynthVoice() {}
    ~PllSynthVoice() {}

    void Init(float sample_rate)
    {
        sr_         = sample_rate;
        sr_recip_   = 1.f / sample_rate;
        phase_      = 0.f;
        freq_       = 0.f;
        target_freq_= 0.f;
        smoothed_freq_ = 0.f;
        amp_        = 1.f;
        waveform_   = WAVE_SQUARE;
        mod_mode_   = MOD_OFF;
        mod_rate_   = 1.f;
        lfo_phase_  = 0.f;
        glide_coeff_= 0.f;
    }

    void SetTargetFreq(float hz)
    {
        target_freq_ = hz;
    }

    void SetAmp(float a) { amp_ = a; }

    void SetWaveform(uint8_t wf)
    {
        waveform_ = wf < WAVE_LAST ? wf : WAVE_SQUARE;
    }

    void SetModMode(ModMode m) { mod_mode_ = m; }

    /// 0..1 controls glide speed or vibrato rate
    void SetModRate(float rate)
    {
        mod_rate_ = rate;

        // Glide: rate maps to portamento coefficient
        // 0 = instant, 1 = very slow glide (~2s)
        float glide_time = rate * 2.f;
        if (glide_time < 0.001f)
            glide_coeff_ = 1.f;
        else
            glide_coeff_ = 1.f - expf(-1.f / (glide_time * sr_));
    }

    float Process()
    {
        if (target_freq_ <= 0.f)
            return 0.f;

        // Apply portamento / vibrato depending on mode
        float effective_freq = target_freq_;

        switch (mod_mode_)
        {
        case MOD_GLIDE:
            smoothed_freq_ += glide_coeff_ * (target_freq_ - smoothed_freq_);
            effective_freq = smoothed_freq_;
            break;

        case MOD_VIBRATO:
        {
            smoothed_freq_ = target_freq_;
            float lfo_hz = 0.5f + mod_rate_ * 14.5f; // 0.5 - 15 Hz
            lfo_phase_ += lfo_hz * sr_recip_;
            if (lfo_phase_ >= 1.f) lfo_phase_ -= 1.f;
            float lfo_val = sinf(lfo_phase_ * M_TWOPI_F);
            // Vibrato depth: up to +/- ~3% pitch (quarter-tone-ish)
            effective_freq = target_freq_ * (1.f + lfo_val * 0.03f);
            break;
        }

        default: // MOD_OFF
            smoothed_freq_ = target_freq_;
            effective_freq = target_freq_;
            break;
        }

        freq_ = effective_freq;

        float phase_inc = freq_ * sr_recip_;
        phase_ += phase_inc;
        if (phase_ >= 1.f) phase_ -= 1.f;
        if (phase_ < 0.f)  phase_ += 1.f;

        float out = 0.f;
        switch (waveform_)
        {
        case WAVE_SIN:
            out = sinf(phase_ * M_TWOPI_F);
            break;

        case WAVE_SAW:
            out = polyBlepSaw(phase_, phase_inc);
            break;

        case WAVE_TRI:
            out = polyBlepTri(phase_, phase_inc);
            break;

        case WAVE_SQUARE:
        default:
            out = polyBlepSquare(phase_, phase_inc);
            break;
        }

        return out * amp_;
    }

  private:
    // PolyBLEP residual
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

    // Integrated polyBLEP square -> triangle via leaky integrator
    float polyBlepTri(float phase, float dt)
    {
        float sq = polyBlepSquare(phase, dt);
        // Leaky integration coefficient tuned for triangle shape
        float c = 4.f * dt;
        tri_integrator_ = tri_integrator_ + c * (sq - tri_integrator_);
        return tri_integrator_;
    }

    float    sr_, sr_recip_;
    float    phase_;
    float    freq_, target_freq_, smoothed_freq_;
    float    amp_;
    uint8_t  waveform_;
    ModMode  mod_mode_;
    float    mod_rate_;
    float    lfo_phase_;
    float    glide_coeff_;
    float    tri_integrator_ = 0.f;
};

} // namespace daisysp
