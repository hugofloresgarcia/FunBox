#pragma once
#include <cmath>
#include <cstring>
#include <cstdint>

#ifndef M_PI_F
#define M_PI_F 3.14159265358979323846f
#endif

static constexpr int   kFftSize       = 2048;
static constexpr int   kHopSize       = 256;
static constexpr int   kNumBins       = kFftSize / 2;
static constexpr int   kMaxVoices     = 4;
static constexpr int   kNumHarmonics  = 8;
static constexpr float kHarmonicDecay = 0.8f;
static constexpr float kMinF0Hz       = 75.f;
static constexpr float kMaxF0Hz       = 1400.f;
static constexpr float kSalienceFloor = 0.001f;
static constexpr float kVoiceMatchSemitones = 3.f;

struct DetectedVoice
{
    float freq;
    float amp;
    bool  active;
};

// In-place radix-2 DIT complex FFT. N must be a power of 2.
// re[] and im[] are arrays of length N, modified in place.
static void fft_radix2(float* re, float* im, int N)
{
    // Bit-reversal permutation
    for (int i = 1, j = 0; i < N; i++)
    {
        int bit = N >> 1;
        for (; j & bit; bit >>= 1)
            j ^= bit;
        j ^= bit;

        if (i < j)
        {
            float tmp;
            tmp = re[i]; re[i] = re[j]; re[j] = tmp;
            tmp = im[i]; im[i] = im[j]; im[j] = tmp;
        }
    }

    // Butterfly stages
    for (int len = 2; len <= N; len <<= 1)
    {
        float angle = -2.f * M_PI_F / len;
        float wRe = cosf(angle);
        float wIm = sinf(angle);

        for (int i = 0; i < N; i += len)
        {
            float curRe = 1.f, curIm = 0.f;
            for (int j = 0; j < len / 2; j++)
            {
                int u = i + j;
                int v = i + j + len / 2;
                float tRe = curRe * re[v] - curIm * im[v];
                float tIm = curRe * im[v] + curIm * re[v];
                re[v] = re[u] - tRe;
                im[v] = im[u] - tIm;
                re[u] += tRe;
                im[u] += tIm;
                float newRe = curRe * wRe - curIm * wIm;
                curIm = curRe * wIm + curIm * wRe;
                curRe = newRe;
            }
        }
    }
}

class PolyPitchDetector
{
  public:
    PolyPitchDetector() {}
    ~PolyPitchDetector() {}

    void Init(float sample_rate)
    {
        sr_       = sample_rate;
        bin_hz_   = sr_ / static_cast<float>(kFftSize);
        write_pos_ = 0;
        hop_count_ = 0;
        tonality_  = 0.f;

        memset(ring_buf_, 0, sizeof(ring_buf_));
        memset(fft_re_, 0, sizeof(fft_re_));
        memset(fft_im_, 0, sizeof(fft_im_));
        memset(mag_, 0, sizeof(mag_));
        memset(mag_work_, 0, sizeof(mag_work_));

        for (int i = 0; i < kMaxVoices; i++)
        {
            voices_[i].freq   = 0.f;
            voices_[i].amp    = 0.f;
            voices_[i].active = false;
        }

        for (int i = 0; i < kFftSize; i++)
            window_[i] = 0.5f * (1.f - cosf(2.f * M_PI_F * i / (kFftSize - 1)));
    }

    // Called per-sample from audio ISR. Only writes the ring buffer.
    bool Process(float in)
    {
        ring_buf_[write_pos_] = in;
        write_pos_ = (write_pos_ + 1) & (kFftSize - 1);
        hop_count_++;

        if (hop_count_ >= kHopSize)
        {
            hop_count_ = 0;
            return true;
        }
        return false;
    }

    // Called from main loop when Process() returned true.
    void Analyze()
    {
        analyzeFrame();
    }

    const DetectedVoice* GetVoices() const { return voices_; }
    float GetTonality() const { return tonality_; }

  private:
    void analyzeFrame()
    {
        int wp = write_pos_;
        for (int i = 0; i < kFftSize; i++)
        {
            int idx = (wp + i) & (kFftSize - 1);
            fft_re_[i] = ring_buf_[idx] * window_[i];
            fft_im_[i] = 0.f;
        }

        fft_radix2(fft_re_, fft_im_, kFftSize);

        for (int i = 0; i < kNumBins; i++)
            mag_[i] = sqrtf(fft_re_[i] * fft_re_[i] + fft_im_[i] * fft_im_[i]);

        // Spectral flatness: exp(mean(log(x))) / mean(x)
        // Near 0 for harmonic (pitched), near 1 for noise (unpitched).
        // tonality_ inverts this: 1 = pitched, 0 = noise.
        {
            float log_sum = 0.f, lin_sum = 0.f;
            int count = 0;
            int lo_bin = static_cast<int>(kMinF0Hz / bin_hz_);
            int hi_bin = static_cast<int>(kMaxF0Hz * 4.f / bin_hz_);
            if (hi_bin > kNumBins) hi_bin = kNumBins;
            for (int i = lo_bin; i < hi_bin; i++)
            {
                float m = mag_[i] + 1e-12f;
                log_sum += logf(m);
                lin_sum += m;
                count++;
            }
            float geo_mean  = expf(log_sum / count);
            float arith_mean = lin_sum / count;
            tonality_ = 1.f - (geo_mean / (arith_mean + 1e-12f));
        }

        memcpy(mag_work_, mag_, sizeof(mag_work_));

        DetectedVoice candidates[kMaxVoices];
        for (int v = 0; v < kMaxVoices; v++)
        {
            candidates[v].freq   = 0.f;
            candidates[v].amp    = 0.f;
            candidates[v].active = false;
        }

        float strongest_salience = 0.f;

        for (int v = 0; v < kMaxVoices; v++)
        {
            float best_salience = 0.f;
            float best_f0       = 0.f;

            float f0 = kMinF0Hz;
            while (f0 <= kMaxF0Hz)
            {
                float salience = computeSalience(f0);
                if (salience > best_salience)
                {
                    best_salience = salience;
                    best_f0       = f0;
                }
                f0 *= 1.012f;
            }

            if (best_salience < kSalienceFloor)
                break;

            // Secondary voices must be at least 15% as strong as the primary
            if (v == 0)
                strongest_salience = best_salience;
            else if (best_salience < strongest_salience * 0.15f)
                break;

            // Refine around best_f0
            float lo = best_f0 * 0.99f;
            float hi = best_f0 * 1.01f;
            float refined_f0 = best_f0;
            float refined_sal = best_salience;
            for (float f = lo; f <= hi; f *= 1.001f)
            {
                float s = computeSalience(f);
                if (s > refined_sal)
                {
                    refined_sal = s;
                    refined_f0 = f;
                }
            }

            candidates[v].freq   = refined_f0;
            candidates[v].amp    = refined_sal;
            candidates[v].active = true;

            cancelHarmonics(refined_f0);
        }

        // Reject candidates that lack a real harmonic series (spectral
        // leakage artifacts only have energy at 1-2 bins, not a full series).
        for (int v = 0; v < kMaxVoices; v++)
        {
            if (!candidates[v].active) continue;

            float peak_h = 0.f;
            for (int h = 1; h <= 6; h++)
            {
                float bin_f = (candidates[v].freq * h) / bin_hz_;
                if (bin_f >= kNumBins - 1) break;
                int bi = static_cast<int>(bin_f);
                float fr = bin_f - bi;
                float m = mag_[bi] + fr * ((bi+1 < kNumBins ? mag_[bi+1] : 0.f) - mag_[bi]);
                if (m > peak_h) peak_h = m;
            }

            if (peak_h < 0.001f) { candidates[v].active = false; continue; }

            int present = 0;
            float thresh = peak_h * 0.08f;
            for (int h = 1; h <= 6; h++)
            {
                float bin_f = (candidates[v].freq * h) / bin_hz_;
                if (bin_f >= kNumBins - 1) break;
                int bi = static_cast<int>(bin_f);
                float fr = bin_f - bi;
                float m = mag_[bi] + fr * ((bi+1 < kNumBins ? mag_[bi+1] : 0.f) - mag_[bi]);
                if (m > thresh) present++;
            }

            if (present < 3) candidates[v].active = false;
        }

        // Re-measure each voice's amplitude from the original (uncancelled)
        // spectrum so iterative cancellation doesn't deflate later voices.
        float max_amp = 0.f;
        for (int v = 0; v < kMaxVoices; v++)
        {
            if (candidates[v].active)
            {
                candidates[v].amp = measureAmplitude(candidates[v].freq);
                if (candidates[v].amp > max_amp)
                    max_amp = candidates[v].amp;
            }
        }

        // Normalize to [0,1] with gentle compression for chord balance
        if (max_amp > 0.f)
        {
            float inv = 1.f / max_amp;
            for (int v = 0; v < kMaxVoices; v++)
            {
                if (candidates[v].active)
                    candidates[v].amp = powf(candidates[v].amp * inv, 0.7f);
            }
        }

        trackVoices(candidates);
    }

    // Amplitude from the original (uncancelled) magnitude spectrum.
    float measureAmplitude(float f0) const
    {
        float salience = 0.f;
        float weight   = 1.f;

        for (int h = 1; h <= kNumHarmonics; h++)
        {
            float bin_f = (f0 * h) / bin_hz_;
            if (bin_f >= kNumBins - 1)
                break;

            int   bin_i = static_cast<int>(bin_f);
            float frac  = bin_f - bin_i;
            float m0 = mag_[bin_i];
            float m1 = (bin_i + 1 < kNumBins) ? mag_[bin_i + 1] : 0.f;

            salience += (m0 + frac * (m1 - m0)) * weight;
            weight *= kHarmonicDecay;
        }
        return salience;
    }

    // Salience from the working (cancelled) spectrum for iterative detection.
    float computeSalience(float f0) const
    {
        float salience = 0.f;
        float weight   = 1.f;

        for (int h = 1; h <= kNumHarmonics; h++)
        {
            float bin_f = (f0 * h) / bin_hz_;
            if (bin_f >= kNumBins - 1)
                break;

            int   bin_i = static_cast<int>(bin_f);
            float frac  = bin_f - bin_i;
            float m0 = mag_work_[bin_i];
            float m1 = (bin_i + 1 < kNumBins) ? mag_work_[bin_i + 1] : 0.f;

            salience += (m0 + frac * (m1 - m0)) * weight;
            weight *= kHarmonicDecay;
        }
        return salience;
    }

    void cancelHarmonics(float f0)
    {
        for (int h = 1; h <= kNumHarmonics; h++)
        {
            int center = static_cast<int>((f0 * h) / bin_hz_ + 0.5f);
            for (int b = center - 3; b <= center + 3; b++)
            {
                if (b >= 0 && b < kNumBins)
                    mag_work_[b] = 0.f;
            }
        }
    }

    void trackVoices(const DetectedVoice* candidates)
    {
        bool candidate_used[kMaxVoices] = {};
        bool voice_matched[kMaxVoices]  = {};

        for (int v = 0; v < kMaxVoices; v++)
        {
            if (!voices_[v].active)
                continue;

            int   best_c    = -1;
            float best_dist = 1e9f;

            for (int c = 0; c < kMaxVoices; c++)
            {
                if (!candidates[c].active || candidate_used[c])
                    continue;

                float ratio = candidates[c].freq / voices_[v].freq;
                float semitone_dist = 12.f * fabsf(logf(ratio)) * 1.4427f; // 1/ln(2)
                if (semitone_dist < best_dist)
                {
                    best_dist = semitone_dist;
                    best_c    = c;
                }
            }

            if (best_c >= 0 && best_dist < kVoiceMatchSemitones)
            {
                voices_[v].freq   = candidates[best_c].freq;
                voices_[v].amp    = candidates[best_c].amp;
                voices_[v].active = true;
                candidate_used[best_c] = true;
                voice_matched[v]       = true;
            }
        }

        for (int v = 0; v < kMaxVoices; v++)
        {
            if (voices_[v].active && !voice_matched[v])
            {
                voices_[v].amp *= 0.96f;
                if (voices_[v].amp < 0.0003f)
                {
                    voices_[v].active = false;
                    voices_[v].freq   = 0.f;
                    voices_[v].amp    = 0.f;
                }
            }
        }

        for (int c = 0; c < kMaxVoices; c++)
        {
            if (!candidates[c].active || candidate_used[c])
                continue;

            for (int v = 0; v < kMaxVoices; v++)
            {
                if (!voices_[v].active)
                {
                    voices_[v] = candidates[c];
                    break;
                }
            }
        }
    }

    float sr_;
    float bin_hz_;

    float        ring_buf_[kFftSize];
    volatile int write_pos_;
    int          hop_count_;

    float window_[kFftSize];
    float fft_re_[kFftSize];
    float fft_im_[kFftSize];
    float mag_[kNumBins];
    float mag_work_[kNumBins];

    DetectedVoice voices_[kMaxVoices];
    float         tonality_;
};
