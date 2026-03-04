#include "daisy_petal.h"
#include <cstdio>

using namespace daisy;

int main()
{
    DaisyPetal hw;
    hw.Init();

    float sr = hw.AudioSampleRate();
    hw.SetAudioBlockSize(48);

    Led led1;
    led1.Init(hw.seed.GetPin(22), false);
    led1.Set(1.0f);
    led1.Update();

    Parameter knob1;
    knob1.Init(hw.knob[DaisyPetal::KNOB_1], 0.f, 1.f, Parameter::LINEAR);

    hw.ProcessAnalogControls();
    hw.ProcessDigitalControls();

    float val = knob1.Process();

    printf("PASS: DaisyPetal compiled and ran. sr=%.0f knob=%.3f\n", sr, val);
    return 0;
}
