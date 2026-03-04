#include "sim_runner.h"
#include <cstring>
#include <cstdio>
#include <algorithm>

namespace sim
{

daisy::DaisyPetal *sim_hw_ptr = nullptr;

void sim_set_knob(daisy::DaisyPetal &hw, int knob_idx, float value_0_1)
{
    if (knob_idx < 0 || knob_idx >= daisy::DaisyPetal::KNOB_LAST) return;
    value_0_1 = std::max(0.f, std::min(1.f, value_0_1));
    uint16_t raw = static_cast<uint16_t>(value_0_1 * 65535.f);
    if (hw.knob[knob_idx].raw_)
        *hw.knob[knob_idx].raw_ = raw;
}

void sim_set_switch(daisy::DaisyPetal &hw, int sw_idx, bool pressed)
{
    if (sw_idx < 0 || sw_idx >= daisy::DaisyPetal::SW_LAST) return;
    hw.switches[sw_idx].SetState(pressed);
}

void sim_apply_config(daisy::DaisyPetal &hw, const SimConfig &cfg)
{
    for (int i = 0; i < 6; i++)
        sim_set_knob(hw, i, cfg.knobs[i]);

    // 3-way switches: pos 0=left, 1=center, 2=right
    // SW_3/SW_4 = switch1 left/right, SW_5/SW_6 = switch2 left/right, SW_7/SW_8 = switch3 left/right
    for (int s = 0; s < 3; s++)
    {
        int left_idx  = 2 + s * 2;
        int right_idx = 3 + s * 2;
        switch (cfg.switch_pos[s])
        {
            case 0: // left
                sim_set_switch(hw, left_idx, true);
                sim_set_switch(hw, right_idx, false);
                break;
            case 2: // right
                sim_set_switch(hw, left_idx, false);
                sim_set_switch(hw, right_idx, true);
                break;
            default: // center
                sim_set_switch(hw, left_idx, false);
                sim_set_switch(hw, right_idx, false);
                break;
        }
    }

    sim_set_switch(hw, 0, cfg.fsw1);
    sim_set_switch(hw, 1, cfg.fsw2);

    // DIP switches: SW_9..SW_12
    for (int i = 0; i < 4; i++)
        sim_set_switch(hw, 8 + i, cfg.dip[i]);

    // Let the analog controls settle
    for (int i = 0; i < 500; i++)
        hw.ProcessAnalogControls();
}

int sim_process_audio(daisy::DaisyPetal &hw,
                      const std::vector<float> &input,
                      std::vector<float> &output_l,
                      std::vector<float> &output_r,
                      size_t block_size)
{
    if (!hw.audio_callback_)
    {
        fprintf(stderr, "ERROR: No audio callback registered.\n");
        return -1;
    }

    size_t total_samples = input.size();
    output_l.resize(total_samples, 0.f);
    output_r.resize(total_samples, 0.f);

    std::vector<float> in_buf(block_size, 0.f);
    std::vector<float> out_l_buf(block_size, 0.f);
    std::vector<float> out_r_buf(block_size, 0.f);

    for (size_t pos = 0; pos < total_samples; pos += block_size)
    {
        size_t chunk = std::min(block_size, total_samples - pos);

        std::memcpy(in_buf.data(), input.data() + pos, chunk * sizeof(float));
        if (chunk < block_size)
            std::memset(in_buf.data() + chunk, 0, (block_size - chunk) * sizeof(float));

        std::memset(out_l_buf.data(), 0, block_size * sizeof(float));
        std::memset(out_r_buf.data(), 0, block_size * sizeof(float));

        const float *in_ptrs[2]  = {in_buf.data(), in_buf.data()};
        float       *out_ptrs[2] = {out_l_buf.data(), out_r_buf.data()};

        hw.audio_callback_(
            reinterpret_cast<daisy::AudioHandle::InputBuffer>(in_ptrs),
            reinterpret_cast<daisy::AudioHandle::OutputBuffer>(out_ptrs),
            block_size);

        std::memcpy(output_l.data() + pos, out_l_buf.data(), chunk * sizeof(float));
        std::memcpy(output_r.data() + pos, out_r_buf.data(), chunk * sizeof(float));
    }

    return 0;
}

} // namespace sim
