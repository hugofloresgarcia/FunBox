#include "hid/switch.h"

namespace daisy
{

void Switch::Init(dsy_gpio_pin pin, float update_rate, Type t, Polarity pol, Pull pu)
{
    (void)pin; (void)update_rate; (void)t; (void)pol; (void)pu;
    pressed_        = false;
    rising_         = false;
    falling_        = false;
    time_held_ms_   = 0.f;
    prev_debounced_ = false;
}

void Switch::Init(dsy_gpio_pin pin, float update_rate)
{
    Init(pin, update_rate, TYPE_MOMENTARY, POLARITY_INVERTED, PULL_UP);
}

void Switch::Debounce()
{
    rising_  = pressed_ && !prev_debounced_;
    falling_ = !pressed_ && prev_debounced_;
    if (pressed_)
        time_held_ms_ += 1.f;
    else
        time_held_ms_ = 0.f;
    prev_debounced_ = pressed_;
}

void Switch::SetState(bool pressed)
{
    pressed_ = pressed;
}

} // namespace daisy
