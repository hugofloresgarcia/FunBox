#include "Filters/svf.h"
#include "Synthesis/oscillator.h"
#include "Utility/dsp.h"
#include <cstdio>
#include <cmath>
#include <cassert>

using namespace daisysp;

static void test_svf()
{
    Svf filt;
    filt.Init(48000.f);
    filt.SetFreq(1000.f);
    filt.SetRes(0.5f);
    filt.SetDrive(0.002f);

    float energy = 0.f;
    for (int i = 0; i < 480; i++)
    {
        float sample = sinf(2.f * M_PI * 440.f * i / 48000.f) * 0.5f;
        filt.Process(sample);
        float out = filt.Low();
        energy += out * out;
        assert(std::isfinite(out));
    }

    assert(energy > 0.f);
    printf("  Svf LP energy (440Hz into 1kHz LP): %.4f PASS\n", energy);
}

static void test_oscillator()
{
    Oscillator osc;
    osc.Init(48000.f);
    osc.SetFreq(440.f);
    osc.SetAmp(0.8f);
    osc.SetWaveform(Oscillator::WAVE_SIN);

    float energy = 0.f;
    for (int i = 0; i < 480; i++)
    {
        float out = osc.Process();
        energy += out * out;
        assert(std::isfinite(out));
    }

    assert(energy > 0.f);
    printf("  Oscillator sine energy: %.4f PASS\n", energy);
}

static void test_softclip()
{
    float x = SoftClip(0.5f);
    assert(std::isfinite(x));
    assert(fabsf(x - 0.5f) < 0.1f);

    float y = SoftClip(5.0f);
    assert(y <= 1.0f);

    float z = SoftClip(-5.0f);
    assert(z >= -1.0f);

    printf("  SoftClip: PASS\n");
}

int main()
{
    printf("Milestone 3: DaisySP native compilation tests\n");
    test_svf();
    test_oscillator();
    test_softclip();
    printf("ALL PASS\n");
    return 0;
}
