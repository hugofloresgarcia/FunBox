#pragma once
#ifndef BOCA_ENVELOPE_FILTER_H
#define BOCA_ENVELOPE_FILTER_H

#include "daisysp.h"
#include "mathutils.h"
#include <cmath>
#include <algorithm>

// ────────────────────────────────────────────────────────────────────────────
// EnvelopeFilterEngine
//
// Envelope follower + LFO → additive mod signal → drives a DaisySP Svf.
// Supports LP/BP/HP output selection, up/down sweep direction, and
// switchable envelope response presets (fast/medium/slow).
// ────────────────────────────────────────────────────────────────────────────

class EnvelopeFilterEngine
{
  public:
    enum class FilterType { LP, BP, HP };
    enum class Direction  { UP, DOWN };
    enum class Response   { FAST, MEDIUM, SLOW };
    enum class LfoWave    { SINE, TRIANGLE };

    void Init(float sample_rate)
    {
        sr_ = sample_rate;
        inv_sr_ = 1.f / sr_;

        // Envelope follower
        env_ = 0.f;
        sensitivity_ = 1.f;
        SetResponse(Response::MEDIUM);

        // LFO
        lfo_phase_ = 0.f;
        lfo_rate_ = 0.f;
        lfo_depth_ = 0.f;
        lfo_wave_ = LfoWave::SINE;

        // Filter
        svf_.Init(sr_);
        svf_.SetRes(0.5f);
        svf_.SetDrive(0.f);
        filter_type_ = FilterType::BP;
        direction_ = Direction::UP;

        // Range
        base_freq_ = 200.f;
        sweep_range_ = 0.5f;

        // Smoothing
        cutoff_smooth_ = base_freq_;

        // Freeze
        frozen_ = false;
        frozen_mod_ = 0.f;

        // Drive
        drive_enabled_ = false;

        // Bass recovery
        bass_recovery_ = false;
        bass_z1_ = 0.f;
        bass_coeff_ = 1.f - expf(-2.f * M_PI * 200.f * inv_sr_);
    }

    // ~~~~~ parameter setters (call per-block) ~~~~~

    void SetSensitivity(float s) { sensitivity_ = s * 4.f; }

    void SetRange(float r) { sweep_range_ = r; }

    void SetResonance(float r) { svf_.SetRes(r); }

    void SetLfoRate(float hz) { lfo_rate_ = hz; }

    void SetLfoDepth(float d) { lfo_depth_ = d; }

    void SetLfoWave(LfoWave w) { lfo_wave_ = w; }

    void SetFilterType(FilterType t) { filter_type_ = t; }

    void SetDirection(Direction d) { direction_ = d; }

    void SetResponse(Response r)
    {
        switch (r) {
            case Response::FAST:
                attack_coeff_  = 1.f - expf(-1.f / (0.001f  * sr_));
                release_coeff_ = 1.f - expf(-1.f / (0.030f  * sr_));
                break;
            case Response::MEDIUM:
                attack_coeff_  = 1.f - expf(-1.f / (0.005f  * sr_));
                release_coeff_ = 1.f - expf(-1.f / (0.100f  * sr_));
                break;
            case Response::SLOW:
                attack_coeff_  = 1.f - expf(-1.f / (0.020f  * sr_));
                release_coeff_ = 1.f - expf(-1.f / (0.300f  * sr_));
                break;
        }
    }

    void SetFreeze(bool f)
    {
        if (f && !frozen_) {
            frozen_mod_ = last_mod_;
        }
        frozen_ = f;
    }

    void SetDrive(bool d) { drive_enabled_ = d; }

    void SetBassRecovery(bool b) { bass_recovery_ = b; }

    // ~~~~~ per-sample processing ~~~~~

    // Update envelope + LFO and return the raw mod value (0-1).
    // Use this in formant mode to drive the formant filter externally.
    float ProcessMod(float in)
    {
        // Envelope follower: rectify the input scaled by sensitivity
        float rect = fabsf(in * sensitivity_);
        float coeff = (rect > env_) ? attack_coeff_ : release_coeff_;
        env_ += coeff * (rect - env_);

        // LFO: unipolar 0-1 output
        float lfo_out = 0.f;
        if (lfo_rate_ > 0.001f) {
            if (!frozen_) {
                lfo_phase_ += lfo_rate_ * inv_sr_;
                if (lfo_phase_ >= 1.f) lfo_phase_ -= 1.f;
            }

            if (lfo_wave_ == LfoWave::SINE) {
                lfo_out = 0.5f * (1.f + sinf(lfo_phase_ * 2.f * (float)M_PI));
            } else {
                lfo_out = (lfo_phase_ < 0.5f)
                    ? lfo_phase_ * 2.f
                    : 2.f - lfo_phase_ * 2.f;
            }
        }

        // Additive mod signal
        if (frozen_) {
            return frozen_mod_;
        }

        float mod = env_ + lfo_out * lfo_depth_;
        mod = std::min(mod, 1.f);
        mod = std::max(mod, 0.f);
        last_mod_ = mod;
        return mod;
    }

    // Full processing: envelope + LFO → mod → SVF filter.
    // Returns the filtered audio sample.
    float Process(float in)
    {
        float mod = ProcessMod(in);

        // Direction: DOWN inverts the mod
        if (direction_ == Direction::DOWN) {
            mod = 1.f - mod;
        }

        // Map mod 0-1 to frequency range (exponential)
        static constexpr float kMinFreq = 80.f;
        static constexpr float kMaxFreq = 8000.f;
        float lo = linexp(sweep_range_, 0.f, 1.f, kMinFreq, 800.f);
        float hi = linexp(sweep_range_, 0.f, 1.f, 400.f, kMaxFreq);
        float target_freq = linexp(mod, 0.f, 1.f, lo, hi);

        // Smooth cutoff to avoid zipper noise
        daisysp::fonepole(cutoff_smooth_, target_freq, 0.002f);
        svf_.SetFreq(cutoff_smooth_);

        // Optional drive: add harmonics before the filter
        float sig = in;
        if (drive_enabled_) {
            sig = tanhf(sig * 2.f);
        }

        // Run the SVF
        svf_.Process(sig);

        float filtered;
        switch (filter_type_) {
            case FilterType::LP:  filtered = svf_.Low();  break;
            case FilterType::HP:  filtered = svf_.High(); break;
            case FilterType::BP:
            default:              filtered = svf_.Band(); break;
        }

        // Bass recovery: mix in lowpassed dry signal
        if (bass_recovery_) {
            bass_z1_ += bass_coeff_ * (in - bass_z1_);
            filtered += bass_z1_ * 0.3f;
        }

        return filtered;
    }

    float GetModValue() const { return last_mod_; }

  private:
    float sr_;
    float inv_sr_;

    // Envelope follower
    float env_;
    float sensitivity_;
    float attack_coeff_;
    float release_coeff_;

    // LFO
    float lfo_phase_;
    float lfo_rate_;
    float lfo_depth_;
    LfoWave lfo_wave_;

    // Filter
    daisysp::Svf svf_;
    FilterType filter_type_;
    Direction direction_;

    // Range
    float base_freq_;
    float sweep_range_;
    float cutoff_smooth_;

    // Freeze
    bool frozen_;
    float frozen_mod_;
    float last_mod_ = 0.f;

    // Drive
    bool drive_enabled_;

    // Bass recovery
    bool bass_recovery_;
    float bass_z1_;
    float bass_coeff_;
};

#endif // BOCA_ENVELOPE_FILTER_H
