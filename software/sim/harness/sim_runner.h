#pragma once
#include "daisy_petal.h"
#include <vector>
#include <string>

namespace sim
{

struct SimConfig
{
    std::string input_file;
    std::string output_file;
    float knobs[6]    = {0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f};
    bool  switches[3] = {false, false, false}; // true = left, false = right
    int   switch_pos[3] = {1, 1, 1}; // 0=left, 1=center, 2=right
    bool  fsw1 = false;
    bool  fsw2 = false;
    bool  dip[4] = {false, false, false, false};
    size_t block_size = 48;
};

extern daisy::DaisyPetal *sim_hw_ptr;

void sim_set_knob(daisy::DaisyPetal &hw, int knob_idx, float value_0_1);
void sim_set_switch(daisy::DaisyPetal &hw, int sw_idx, bool pressed);

int sim_process_audio(daisy::DaisyPetal &hw,
                      const std::vector<float> &input,
                      std::vector<float> &output_l,
                      std::vector<float> &output_r,
                      size_t block_size);

void sim_apply_config(daisy::DaisyPetal &hw, const SimConfig &cfg);

} // namespace sim
