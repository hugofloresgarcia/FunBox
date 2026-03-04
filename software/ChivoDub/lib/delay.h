#pragma once

#ifndef DSY_HUGODELAY_H
#define DSY_HUGODELAY_H

#include <cmath>
#include <cstdint>
#include "daisy_petal.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

namespace daisysp
{
class DelayEngine
{
  public:
    DelayEngine() {}
    ~DelayEngine() {}

    void Init(float sample_rate, float fade_time_ms = 20.0f)
    {
        sample_rate_ = sample_rate;

        del_.Init();

        feedback_ = 0.2f;
        SetDelayMs(1000.f);

        wet_        = 1.0f;
        wet_target_ = 1.0f;
        bypass_     = false;

        const float fade_seconds = fmax(0.001f, fade_time_ms * 0.001f);
        wet_coeff_ = std::exp(-1.0f / (fade_seconds * sample_rate_));
    }


    float Process(float in)
    {
        // smooth the wet sign
        fonepole(wet_, wet_target_, wet_coeff_);

        // smooth delay time
        fonepole(delay_, delay_target_, 0.00007f);
        del_.SetDelay(delay_);

        // read delayed sample
        float delayed = del_.Read();

        // write new sample to delay line
        float line_in = in + delayed * feedback_;
        line_in = SoftClip(line_in);
        del_.Write(line_in);

        // dry/wet
        const float out = in + delayed * wet_;
        return out;
    }

    void SetDelayMs(float ms)
    {
        ms = fmax(0.1f, ms);
        delay_target_ = ms * 0.001f * sample_rate_;
    }

    void SetFeedback(float feedback)
    {
        feedback_ = fclamp(feedback, 0.0f, 1.0f);
    }

    void SetBypass(bool should_bypass)
    {
        bypass_ = should_bypass;
        wet_target_ = bypass_ ? 0.0f : 1.0f;
    }

    void SetFadeTimeMs(float fade_time_ms)
    {
        const float fade_seconds = fmax(0.001f, fade_time_ms * 0.001f);
        wet_coeff_ = std::exp(-1.0f / (fade_seconds * sample_rate_));
    }

    float GetMaxDelayMs() const
    {
        return (kDelayLength / sample_rate_) * 1000.0f; // Convert samples to milliseconds
    }

  private:
    float sample_rate_;
    static constexpr int32_t kDelayLength = 2 * 48000; // enough for ~2 s @48kHz (adjust if needed)

    float feedback_;
    float delay_;         // smoothed current delay length (in samples)
    float delay_target_;  // target delay length (in samples)

    DelayLine<float, kDelayLength> del_;

    // bypass 
    bool  bypass_;
    float wet_;          
    float wet_target_;  
    float wet_coeff_;  

};
} // namespace daisysp


class OscillatorBase {
public:
    virtual ~OscillatorBase() {}

    virtual void SetFreq(float freq) = 0;
    virtual void SetAmp(float amp) = 0;
    virtual float Process() = 0;
};

class LFO : public OscillatorBase {
public:
    LFO() : osc(), last_value(0.0f) {}

    void Init(float sample_rate) {
        osc.Init(sample_rate);
        osc.SetWaveform(osc.WAVE_SIN);
    }

    void SetFreq(float freq) override {
        osc.SetFreq(freq);
    }

    void SetAmp(float amp) override {
        osc.SetAmp(amp);
    }
    
    void SetWaveform(uint8_t waveform) {
        osc.SetWaveform(waveform);
    }

    void Reset(float phase = 0.0f) {
        osc.Reset(phase);
    }

    float Process() override {
        last_value = osc.Process();
        return last_value;
    }

private:
    Oscillator osc;
    float last_value;
};

class Noise : public OscillatorBase {
public:
    Noise() : last_value(0.0f), amp(1.0f), hold_rate(1.0f), last_sample_time(0.0f) {}

    void Init(float sample_rate) {
        // Set the sample rate and initial parameters for hold functionality
        sample_rate_ = sample_rate;
    }

    void SetFreq(float freq) override {
        // Set frequency to control the sample rate of the sample and hold
        hold_rate = freq;
    }

    void SetAmp(float amp) override {
        this->amp = amp;
    }

    float Process() override {
        // Generate white noise
        float noise_value = GenerateNoise();

        // Sample-and-hold logic (simple time-based thresholding)
        if (System::GetNow() - last_sample_time >= (1.0f / hold_rate)) {
            last_value = noise_value;
            last_sample_time = System::GetNow();
        }

        // Return the processed value multiplied by the amplitude
        return last_value * amp;
    }

    void Reset(float phase = 0.0f) {
        // Reset the last value and sample time
        last_value = phase;
        last_sample_time = System::GetNow();
    }

private:
    // Simple noise generator using random values (you could use a more sophisticated generator here)
    float GenerateNoise() {
        // This is a simple noise function, using the system's time for randomness
        // You could replace this with a better noise function (e.g., Perlin noise, white noise, etc.)
        return static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 2.0f - 1.0f; // Generates a value between -1 and 1
    }

    float last_value;
    float amp;
    float hold_rate;
    float last_sample_time;
    float sample_rate_;  // Used to track sample rate, could be useful for interpolation or smoothing
};


class WaveGenerator {
public:
    WaveGenerator() {}

    enum
    {
        WAVE_SIN,
        WAVE_TRI,
        WAVE_SAW,
        WAVE_RAMP,
        WAVE_SQUARE,
        WAVE_POLYBLEP_TRI,
        WAVE_NOISE,
        WAVE_LAST,
    };


    void Init(float sr) {
        lfo_.Init(sr);
        noise_.Init(sr);
    }

    void SetFreq(float freq) {
        lfo_.SetFreq(freq);
        noise_.SetFreq(freq/30000.0f);
    }

    void SetAmp(float amp) {
        lfo_.SetAmp(amp);
        noise_.SetAmp(amp);
    }

    void SetWaveform(uint8_t waveform) {
        if (waveform > 6) {
            // we're using noise here.
            is_noise_ = true;
        }
        else {
            lfo_.SetWaveform(waveform);
            is_noise_ = false;
        }
    }

    void Reset(float phase = 0.0f) {
        lfo_.Reset(phase);
        noise_.Reset();
    }

    float Process() {
        if (is_noise_) {
            return noise_.Process();
        } else {
            return lfo_.Process();
        }
    }

private:
    LFO lfo_;      // Pointer to the LFO oscillator
    Noise noise_;  // Pointer to the Noise oscillator
    
    uint8_t waveform_;  // Current waveform type
    bool is_noise_ = false;  // Flag to indicate if noise is being used
};



#endif // DSY_HUGODELAY_H
