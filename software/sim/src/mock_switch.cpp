#include "hid/switch.h"

namespace daisy
{

void Switch::Init(dsy_gpio_pin pin, float update_rate, Type t, Polarity pol, Pull pu)
{
    (void)pin; (void)update_rate; (void)t; (void)pol; (void)pu;
    pressed_ = false;
    rising_  = false;
    falling_ = false;
    time_held_ms_ = 0.f;
}

void Switch::Init(dsy_gpio_pin pin, float update_rate)
{
    Init(pin, update_rate, TYPE_MOMENTARY, POLARITY_INVERTED, PULL_UP);
}

void Switch::Debounce()
{
    // In the sim, state is set directly via SetState()
}

void Switch::SetState(bool pressed)
{
    bool was_pressed = pressed_;
    pressed_ = pressed;
    rising_  = pressed && !was_pressed;
    falling_ = !pressed && was_pressed;
    if (pressed && was_pressed)
        time_held_ms_ += 1.f;
    else if (pressed && !was_pressed)
        time_held_ms_ = 0.f;
    else
        time_held_ms_ = 0.f;
}

} // namespace daisy
