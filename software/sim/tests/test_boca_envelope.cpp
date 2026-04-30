#include "daisysp.h"
#include "envelope_filter.h"
#include <cstdio>
#include <cmath>
#include <cassert>

static constexpr float kSampleRate = 48000.f;
static constexpr float kPi = 3.14159265358979f;

static float gen_sine(float freq, int sample, float amp = 0.3f)
{
    return amp * sinf(2.f * kPi * freq * sample / kSampleRate);
}

static int g_pass = 0;
static int g_fail = 0;

static void check(const char *name, bool cond, const char *detail)
{
    if (cond) {
        printf("  PASS: %s  (%s)\n", name, detail);
        g_pass++;
    } else {
        printf("  FAIL: %s  (%s)\n", name, detail);
        g_fail++;
    }
}

// ────────────────────────────────────────────────────────────────────────────
// Test 1: Envelope mod tracks a tone burst (max sensitivity, FAST response)
// ────────────────────────────────────────────────────────────────────────────
static void test_envelope_mod_tracks_burst()
{
    printf("\n--- Test 1: Envelope mod tracks tone burst ---\n");

    EnvelopeFilterEngine eng;
    eng.Init(kSampleRate);
    eng.SetSensitivity(1.0f);   // 8x
    eng.SetLfoRate(0.f);
    eng.SetLfoDepth(0.f);
    eng.SetResponse(EnvelopeFilterEngine::Response::FAST);

    // Feed 100ms of 220 Hz sine at amplitude 0.3
    const int burst_samples = (int)(0.1f * kSampleRate);
    float peak_mod = 0.f;
    for (int i = 0; i < burst_samples; i++) {
        float in = gen_sine(220.f, i);
        float mod = eng.ProcessMod(in);
        if (mod > peak_mod) peak_mod = mod;
    }

    char buf[128];
    snprintf(buf, sizeof(buf), "peak_mod=%.4f, expected > 0.5", peak_mod);
    check("burst peak mod", peak_mod > 0.5f, buf);

    // Feed 500ms of silence, check decay
    const int silence_samples = (int)(0.5f * kSampleRate);
    float mod_after = 0.f;
    for (int i = 0; i < silence_samples; i++) {
        mod_after = eng.ProcessMod(0.f);
    }

    snprintf(buf, sizeof(buf), "mod_after_silence=%.6f, expected < 0.01", mod_after);
    check("silence decay", mod_after < 0.01f, buf);
}

// ────────────────────────────────────────────────────────────────────────────
// Test 2: Envelope mod at moderate sensitivity
// ────────────────────────────────────────────────────────────────────────────
static void test_envelope_mod_moderate()
{
    printf("\n--- Test 2: Envelope mod at moderate sensitivity ---\n");

    EnvelopeFilterEngine eng;
    eng.Init(kSampleRate);
    eng.SetSensitivity(0.25f);  // 2x
    eng.SetLfoRate(0.f);
    eng.SetLfoDepth(0.f);
    eng.SetResponse(EnvelopeFilterEngine::Response::MEDIUM);

    const int burst_samples = (int)(0.1f * kSampleRate);
    float peak_mod = 0.f;
    for (int i = 0; i < burst_samples; i++) {
        float in = gen_sine(220.f, i, 0.15f);
        float mod = eng.ProcessMod(in);
        if (mod > peak_mod) peak_mod = mod;
    }

    char buf[128];
    snprintf(buf, sizeof(buf), "peak_mod=%.4f, expected > 0.15", peak_mod);
    check("moderate sensitivity peak", peak_mod > 0.15f, buf);
}

// ────────────────────────────────────────────────────────────────────────────
// Test 3: LFO modulates independently of input
// ────────────────────────────────────────────────────────────────────────────
static void test_lfo_independent()
{
    printf("\n--- Test 3: LFO modulates independently ---\n");

    EnvelopeFilterEngine eng;
    eng.Init(kSampleRate);
    eng.SetSensitivity(0.f);    // envelope off
    eng.SetLfoRate(5.f);
    eng.SetLfoDepth(1.0f);
    eng.SetResponse(EnvelopeFilterEngine::Response::MEDIUM);

    const int samples = (int)(1.0f * kSampleRate);
    float min_mod = 1.f, max_mod = 0.f;
    for (int i = 0; i < samples; i++) {
        float mod = eng.ProcessMod(0.f);
        if (mod < min_mod) min_mod = mod;
        if (mod > max_mod) max_mod = mod;
    }

    char buf[128];
    snprintf(buf, sizeof(buf), "max_mod=%.4f, expected > 0.9", max_mod);
    check("LFO max swing", max_mod > 0.9f, buf);

    snprintf(buf, sizeof(buf), "min_mod=%.4f, expected < 0.1", min_mod);
    check("LFO min swing", min_mod < 0.1f, buf);
}

// ────────────────────────────────────────────────────────────────────────────
// Test 4: Filter cutoff actually moves with envelope (LP mode)
// ────────────────────────────────────────────────────────────────────────────
static void test_filter_tracks_envelope()
{
    printf("\n--- Test 4: LP filter cutoff tracks envelope ---\n");

    EnvelopeFilterEngine eng;
    eng.Init(kSampleRate);
    eng.SetSensitivity(1.0f);   // 8x
    eng.SetLfoRate(0.f);
    eng.SetLfoDepth(0.f);
    eng.SetRange(0.5f);
    eng.SetResonance(0.3f);
    eng.SetFilterType(EnvelopeFilterEngine::FilterType::LP);
    eng.SetResponse(EnvelopeFilterEngine::Response::FAST);

    // Settle the filter at base cutoff with 200ms of silence
    for (int i = 0; i < (int)(0.2f * kSampleRate); i++) {
        eng.Process(0.f);
    }

    // Start feeding 3 kHz tone at 0.3 amplitude.
    // Measure RMS in first 2ms (filter closed) vs 40-60ms (filter open).
    const int early_end   = (int)(0.002f * kSampleRate);    // 96 samples
    const int open_start  = (int)(0.040f * kSampleRate);    // 1920 samples
    const int open_end    = (int)(0.060f * kSampleRate);    // 2880 samples

    double sum_sq_closed = 0.0;
    double sum_sq_open   = 0.0;
    int n_closed = 0, n_open = 0;

    for (int i = 0; i < open_end; i++) {
        float in = gen_sine(3000.f, i);
        float out = eng.Process(in);

        if (i < early_end) {
            sum_sq_closed += (double)out * out;
            n_closed++;
        }
        if (i >= open_start && i < open_end) {
            sum_sq_open += (double)out * out;
            n_open++;
        }
    }

    float rms_closed = sqrtf((float)(sum_sq_closed / n_closed));
    float rms_open   = sqrtf((float)(sum_sq_open / n_open));
    float ratio = (rms_closed > 1e-10f) ? rms_open / rms_closed : 999.f;

    char buf[128];
    snprintf(buf, sizeof(buf),
             "rms_closed=%.6f, rms_open=%.6f, ratio=%.2f, expected > 3.0",
             rms_closed, rms_open, ratio);
    check("filter opens with envelope", ratio > 3.f, buf);
}

// ────────────────────────────────────────────────────────────────────────────
// Test 5: Cutoff smoothing doesn't kill the envelope attack
// ────────────────────────────────────────────────────────────────────────────
static void test_attack_not_killed()
{
    printf("\n--- Test 5: Cutoff smoothing preserves attack ---\n");

    EnvelopeFilterEngine eng;
    eng.Init(kSampleRate);
    eng.SetSensitivity(1.0f);   // 8x
    eng.SetLfoRate(0.f);
    eng.SetLfoDepth(0.f);
    eng.SetRange(0.5f);
    eng.SetResonance(0.3f);
    eng.SetFilterType(EnvelopeFilterEngine::FilterType::LP);
    eng.SetResponse(EnvelopeFilterEngine::Response::FAST);

    // Settle at base
    for (int i = 0; i < (int)(0.2f * kSampleRate); i++) {
        eng.Process(0.f);
    }

    // Feed burst and sample mod at 5ms and 10ms
    const int at_5ms  = (int)(0.005f * kSampleRate);   // 240
    const int at_10ms = (int)(0.010f * kSampleRate);    // 480

    float mod_5ms = 0.f, mod_10ms = 0.f;

    for (int i = 0; i <= at_10ms; i++) {
        float in = gen_sine(220.f, i);
        eng.Process(in);

        if (i == at_5ms)  mod_5ms  = eng.GetModValue();
        if (i == at_10ms) mod_10ms = eng.GetModValue();
    }

    char buf[128];
    snprintf(buf, sizeof(buf), "mod_at_5ms=%.4f, expected > 0.3", mod_5ms);
    check("mod at 5ms", mod_5ms > 0.3f, buf);

    snprintf(buf, sizeof(buf), "mod_at_10ms=%.4f, expected > 0.6", mod_10ms);
    check("mod at 10ms", mod_10ms > 0.6f, buf);
}

// ────────────────────────────────────────────────────────────────────────────

int main()
{
    printf("=== Boca Envelope Filter Tests ===\n");

    test_envelope_mod_tracks_burst();
    test_envelope_mod_moderate();
    test_lfo_independent();
    test_filter_tracks_envelope();
    test_attack_not_killed();

    printf("\n=== Results: %d passed, %d failed ===\n", g_pass, g_fail);
    return g_fail > 0 ? 1 : 0;
}
