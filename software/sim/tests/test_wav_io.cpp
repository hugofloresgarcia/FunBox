#include "wav_io.h"
#include <cstdio>
#include <cmath>
#include <cassert>

static void test_round_trip()
{
    const uint32_t sr = 48000;
    const uint32_t len = 48000;

    wav::WavData original;
    original.sample_rate = sr;
    original.channels.resize(2);
    original.channels[0].resize(len);
    original.channels[1].resize(len);

    for (uint32_t i = 0; i < len; i++)
    {
        original.channels[0][i] = sinf(2.f * M_PI * 440.f * i / sr) * 0.5f;
        original.channels[1][i] = sinf(2.f * M_PI * 880.f * i / sr) * 0.3f;
    }

    const char *path = "/tmp/funbox_sim_test.wav";
    wav::write_wav(path, original);
    printf("  Wrote %s (%u samples, stereo)\n", path, len);

    wav::WavData loaded = wav::read_wav(path);
    assert(loaded.sample_rate == sr);
    assert(loaded.channels.size() == 2);
    assert(loaded.channels[0].size() == len);
    assert(loaded.channels[1].size() == len);

    float max_err_l = 0.f, max_err_r = 0.f;
    for (uint32_t i = 0; i < len; i++)
    {
        float el = fabsf(loaded.channels[0][i] - original.channels[0][i]);
        float er = fabsf(loaded.channels[1][i] - original.channels[1][i]);
        if (el > max_err_l) max_err_l = el;
        if (er > max_err_r) max_err_r = er;
    }

    printf("  Round-trip max error L=%.6f R=%.6f\n", max_err_l, max_err_r);
    // 16-bit quantization: max error should be < 1/32768 ≈ 3e-5
    assert(max_err_l < 0.001f);
    assert(max_err_r < 0.001f);
    printf("  WAV round-trip: PASS\n");
}

static void test_mono_write_read()
{
    wav::WavData mono;
    mono.sample_rate = 44100;
    mono.channels.resize(1);
    mono.channels[0].resize(1000);
    for (int i = 0; i < 1000; i++)
        mono.channels[0][i] = sinf(2.f * M_PI * 1000.f * i / 44100.f) * 0.8f;

    const char *path = "/tmp/funbox_sim_test_mono.wav";
    wav::write_wav(path, mono);
    wav::WavData loaded = wav::read_wav(path);
    assert(loaded.channels.size() == 1);
    assert(loaded.channels[0].size() == 1000);
    assert(loaded.sample_rate == 44100);
    printf("  Mono WAV: PASS\n");
}

int main()
{
    printf("Milestone 5: WAV I/O tests\n");
    test_round_trip();
    test_mono_write_read();
    printf("ALL PASS\n");
    return 0;
}
