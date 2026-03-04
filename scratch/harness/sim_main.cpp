#include "daisy_petal.h"
#include "sim_runner.h"
#include "wav_io.h"
#include <cstdio>
#include <cmath>
#include <vector>
#include <string>
#include <cstring>

extern daisy::DaisyPetal hw;
int pedal_main(void);

static void print_usage(const char *name)
{
    fprintf(stderr, "Usage: %s [options]\n", name);
    fprintf(stderr, "  --input  <file.wav>  Input WAV file (or 'sine' for generated tone)\n");
    fprintf(stderr, "  --output <file.wav>  Output WAV file (default: output.wav)\n");
    fprintf(stderr, "  --knob1..6 <0-1>     Knob positions (default: 0.5)\n");
    fprintf(stderr, "  --switch1..3 <left|center|right>  3-way switch positions\n");
    fprintf(stderr, "  --fsw1, --fsw2       Enable footswitches\n");
    fprintf(stderr, "  --dip1..4            Enable DIP switches\n");
    fprintf(stderr, "  --duration <sec>     Duration for generated input (default: 1.0)\n");
    fprintf(stderr, "  --freq <hz>          Frequency for generated sine (default: 440)\n");
}

int main(int argc, char **argv)
{
    sim::SimConfig cfg;
    cfg.output_file = "output.wav";
    std::string input_mode = "sine";
    float duration = 1.0f;
    float freq = 440.f;

    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];
        auto next = [&]() -> const char* { return (i + 1 < argc) ? argv[++i] : ""; };

        if (arg == "--input")       { cfg.input_file = next(); input_mode = "file"; }
        else if (arg == "--output") cfg.output_file = next();
        else if (arg == "--knob1")  cfg.knobs[0] = atof(next());
        else if (arg == "--knob2")  cfg.knobs[1] = atof(next());
        else if (arg == "--knob3")  cfg.knobs[2] = atof(next());
        else if (arg == "--knob4")  cfg.knobs[3] = atof(next());
        else if (arg == "--knob5")  cfg.knobs[4] = atof(next());
        else if (arg == "--knob6")  cfg.knobs[5] = atof(next());
        else if (arg.substr(0, 9) == "--switch1" || arg.substr(0, 9) == "--switch2" || arg.substr(0, 9) == "--switch3")
        {
            int idx = arg[8] - '1';
            std::string val = next();
            if (val == "left")        cfg.switch_pos[idx] = 0;
            else if (val == "right")  cfg.switch_pos[idx] = 2;
            else                      cfg.switch_pos[idx] = 1;
        }
        else if (arg == "--fsw1") cfg.fsw1 = true;
        else if (arg == "--fsw2") cfg.fsw2 = true;
        else if (arg == "--dip1") cfg.dip[0] = true;
        else if (arg == "--dip2") cfg.dip[1] = true;
        else if (arg == "--dip3") cfg.dip[2] = true;
        else if (arg == "--dip4") cfg.dip[3] = true;
        else if (arg == "--duration") duration = atof(next());
        else if (arg == "--freq") freq = atof(next());
        else if (arg == "--help" || arg == "-h") { print_usage(argv[0]); return 0; }
        else { fprintf(stderr, "Unknown option: %s\n", arg.c_str()); print_usage(argv[0]); return 1; }
    }

    // Initialize pedal (calls hw.Init(), sets up DSP, registers callback)
    sim::break_on_delay = true;
    try {
        pedal_main();
    } catch (const sim::InitDone &) {
        // Expected: pedal init complete, callback registered
    }
    sim::break_on_delay = false;

    if (!hw.audio_callback_)
    {
        fprintf(stderr, "ERROR: Pedal did not register an audio callback.\n");
        return 1;
    }

    size_t block_size = hw.AudioBlockSize();
    float sr = hw.AudioSampleRate();

    fprintf(stderr, "Pedal initialized: sr=%.0f block_size=%zu\n", sr, block_size);

    // Prepare input
    std::vector<float> input;
    uint32_t input_sr = static_cast<uint32_t>(sr);

    if (input_mode == "file" && !cfg.input_file.empty())
    {
        wav::WavData wav_in = wav::read_wav(cfg.input_file);
        input = wav_in.channels[0];
        input_sr = wav_in.sample_rate;
        fprintf(stderr, "Loaded %s: %zu samples @ %uHz\n",
                cfg.input_file.c_str(), input.size(), input_sr);
    }
    else
    {
        size_t num_samples = static_cast<size_t>(duration * sr);
        input.resize(num_samples);
        for (size_t i = 0; i < num_samples; i++)
            input[i] = sinf(2.f * M_PI * freq * i / sr) * 0.5f;
        fprintf(stderr, "Generated %.1fHz sine, %.1fs (%zu samples)\n", freq, duration, num_samples);
    }

    // Apply knob/switch config
    sim::sim_apply_config(hw, cfg);

    // Process audio
    std::vector<float> out_l, out_r;
    int ret = sim::sim_process_audio(hw, input, out_l, out_r, block_size);
    if (ret != 0)
    {
        fprintf(stderr, "ERROR: Audio processing failed.\n");
        return 1;
    }

    // Compute stats
    float peak_l = 0.f, energy_l = 0.f;
    for (size_t i = 0; i < out_l.size(); i++)
    {
        float a = fabsf(out_l[i]);
        if (a > peak_l) peak_l = a;
        energy_l += out_l[i] * out_l[i];
    }
    float rms_l = sqrtf(energy_l / out_l.size());

    fprintf(stderr, "Output: %zu samples, peak=%.4f rms=%.4f\n", out_l.size(), peak_l, rms_l);

    // Write output
    wav::write_wav_stereo(cfg.output_file, out_l, out_r, input_sr);
    fprintf(stderr, "Wrote %s\n", cfg.output_file.c_str());

    return 0;
}
