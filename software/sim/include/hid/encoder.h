#pragma once
#include "daisy_core.h"

namespace daisy
{

class Encoder
{
  public:
    Encoder() {}
    ~Encoder() {}
    void Init(dsy_gpio_pin a, dsy_gpio_pin b, dsy_gpio_pin click, float update_rate = 0.f)
    {
        (void)a; (void)b; (void)click; (void)update_rate;
    }
    void Debounce() {}
    int32_t Increment() { return 0; }
    bool RisingEdge() { return false; }
    bool FallingEdge() { return false; }
    bool Pressed() { return false; }
};

} // namespace daisy
