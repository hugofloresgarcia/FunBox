#include "daisy_petal.h"
#include "sim_runner.h"
#include <cstdio>
#include <cmath>
#include <cassert>
#include <vector>

using namespace daisy;

DaisyPetal hw;

static void PassthroughCallback(AudioHandle::InputBuffer  in,
                                 AudioHandle::OutputBuffer out,
                                 size_t                    size)
{
    hw.ProcessAnalogControls();
    hw.ProcessDigitalControls();

    for (size_t i = 0; i < size; i++)
    {
        out[0][i] = in[0][i];
        out[1][i] = in[0][i];
    }
}

int main()
{
    printf("Milestone 4: Audio callback harness test\n");

    hw.Init();
    hw.SetAudioBlockSize(48);
    hw.StartAudio(PassthroughCallback);

    // Generate a 440 Hz sine, 1 second at 48 kHz
    const size_t num_samples = 48000;
    std::vector<float> input(num_samples);
    for (size_t i = 0; i < num_samples; i++)
        input[i] = sinf(2.f * M_PI * 440.f * i / 48000.f) * 0.5f;

    std::vector<float> out_l, out_r;
    int ret = sim::sim_process_audio(hw, input, out_l, out_r, 48);
    assert(ret == 0);
    assert(out_l.size() == num_samples);
    assert(out_r.size() == num_samples);

    // Verify passthrough: output should match input
    float max_err = 0.f;
    for (size_t i = 0; i < num_samples; i++)
    {
        float err = fabsf(out_l[i] - input[i]);
        if (err > max_err) max_err = err;
    }

    printf("  Passthrough max error: %.10f\n", max_err);
    assert(max_err < 1e-6f);

    // Verify L and R are the same
    for (size_t i = 0; i < num_samples; i++)
        assert(fabsf(out_l[i] - out_r[i]) < 1e-6f);

    printf("  Passthrough: PASS\n");
    printf("ALL PASS\n");
    return 0;
}
