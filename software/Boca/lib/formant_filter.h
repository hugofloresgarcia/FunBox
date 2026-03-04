#pragma once
#ifndef BOCA_FORMANT_FILTER_H
#define BOCA_FORMANT_FILTER_H

#include "daisysp.h"
#include "mathutils.h"
#include <cmath>
#include <algorithm>

// ────────────────────────────────────────────────────────────────────────────
// FormantFilter
//
// Three parallel SVFs in bandpass mode, tuned to F1/F2/F3 vowel formant
// frequencies. A mod signal (0-1) morphs between two vowel targets,
// interpolating all three formant frequencies simultaneously.
//
// Vowel pairs are selected by SetVowelPair(). The resonance knob controls
// the Q of the formant peaks — low Q gives subtle vocal color, high Q
// makes it a pronounced vowel.
// ────────────────────────────────────────────────────────────────────────────

class FormantFilter
{
  public:
    enum class VowelPair {
        AH_EE,   // /a/ → /i/ — open to bright, classic "yah"
        OH_EE,   // /o/ → /i/ — round to bright, "woy"
        OO_AH    // /u/ → /a/ — dark to open, "wah"
    };

    void Init(float sample_rate)
    {
        sr_ = sample_rate;
        inv_sr_ = 1.f / sr_;

        for (int i = 0; i < 3; i++) {
            svf_[i].Init(sr_);
            svf_[i].SetRes(0.7f);
            svf_[i].SetDrive(0.f);
            freq_smooth_[i] = 500.f;
        }

        formant_gain_[0] = 1.0f;
        formant_gain_[1] = 0.7f;
        formant_gain_[2] = 0.4f;

        vowel_pair_ = VowelPair::OO_AH;
        drive_enabled_ = false;
        bass_recovery_ = false;
        bass_z1_ = 0.f;
        bass_coeff_ = 1.f - expf(-2.f * (float)M_PI * 200.f * inv_sr_);
    }

    void SetVowelPair(VowelPair vp) { vowel_pair_ = vp; }

    void SetResonance(float r)
    {
        float q = linlin(r, 0.f, 1.f, 0.3f, 0.95f);
        for (int i = 0; i < 3; i++) {
            svf_[i].SetRes(q);
        }
    }

    void SetDrive(bool d) { drive_enabled_ = d; }

    void SetBassRecovery(bool b) { bass_recovery_ = b; }

    float Process(float in, float mod)
    {
        // Look up vowel pair formant frequencies
        float f1_start, f2_start, f3_start;
        float f1_end, f2_end, f3_end;
        getVowelFreqs(vowel_pair_, f1_start, f2_start, f3_start,
                                    f1_end, f2_end, f3_end);

        // Interpolate formant frequencies based on mod value
        float f1 = f1_start + mod * (f1_end - f1_start);
        float f2 = f2_start + mod * (f2_end - f2_start);
        float f3 = f3_start + mod * (f3_end - f3_start);

        float freqs[3] = { f1, f2, f3 };

        // Smooth formant frequencies
        for (int i = 0; i < 3; i++) {
            daisysp::fonepole(freq_smooth_[i], freqs[i], 0.001f);
            svf_[i].SetFreq(freq_smooth_[i]);
        }

        float sig = in;
        if (drive_enabled_) {
            sig = tanhf(sig * 2.f);
        }

        // Run all three formant filters in parallel, sum with decreasing gain
        float out = 0.f;
        for (int i = 0; i < 3; i++) {
            svf_[i].Process(sig);
            out += svf_[i].Band() * formant_gain_[i];
        }

        // Bass recovery
        if (bass_recovery_) {
            bass_z1_ += bass_coeff_ * (in - bass_z1_);
            out += bass_z1_ * 0.3f;
        }

        return out;
    }

  private:
    // Vowel formant frequencies (Hz)
    //   /a/ (ah): F1=800,  F2=1200, F3=2800
    //   /i/ (ee): F1=270,  F2=2300, F3=3000
    //   /o/ (oh): F1=500,  F2=800,  F3=2800
    //   /u/ (oo): F1=300,  F2=700,  F3=2200
    static void getVowelFreqs(VowelPair vp,
                              float& f1s, float& f2s, float& f3s,
                              float& f1e, float& f2e, float& f3e)
    {
        switch (vp) {
            case VowelPair::AH_EE:
                f1s = 800.f;  f2s = 1200.f; f3s = 2800.f;
                f1e = 270.f;  f2e = 2300.f; f3e = 3000.f;
                break;
            case VowelPair::OH_EE:
                f1s = 500.f;  f2s = 800.f;  f3s = 2800.f;
                f1e = 270.f;  f2e = 2300.f; f3e = 3000.f;
                break;
            case VowelPair::OO_AH:
            default:
                f1s = 300.f;  f2s = 700.f;  f3s = 2200.f;
                f1e = 800.f;  f2e = 1200.f; f3e = 2800.f;
                break;
        }
    }

    float sr_;
    float inv_sr_;

    daisysp::Svf svf_[3];
    float freq_smooth_[3];
    float formant_gain_[3];

    VowelPair vowel_pair_;

    bool drive_enabled_;

    bool bass_recovery_;
    float bass_z1_;
    float bass_coeff_;
};

#endif // BOCA_FORMANT_FILTER_H
