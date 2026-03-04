#pragma once
#include "daisy_core.h"
#include "per/gpio.h"
#include "sys/system.h"

namespace daisy
{

class Switch
{
  public:
    enum Type { TYPE_TOGGLE, TYPE_MOMENTARY };
    enum Polarity { POLARITY_NORMAL, POLARITY_INVERTED };
    enum Pull { PULL_UP, PULL_DOWN, PULL_NONE };

    Switch() {}
    ~Switch() {}

    void Init(dsy_gpio_pin pin, float update_rate, Type t, Polarity pol, Pull pu);
    void Init(dsy_gpio_pin pin, float update_rate = 0.f);
    void Debounce();

    inline bool RisingEdge() const { return rising_; }
    inline bool FallingEdge() const { return falling_; }
    inline bool Pressed() const { return pressed_; }
    inline float TimeHeldMs() const { return pressed_ ? time_held_ms_ : 0.f; }

    void SetState(bool pressed);

    bool  pressed_      = false;
    bool  rising_       = false;
    bool  falling_      = false;
    float time_held_ms_ = 0.f;
};

} // namespace daisy
