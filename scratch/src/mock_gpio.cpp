#include "per/gpio.h"

extern "C"
{
    void dsy_gpio_init(const dsy_gpio *p) { (void)p; }
    void dsy_gpio_deinit(const dsy_gpio *p) { (void)p; }
    uint8_t dsy_gpio_read(const dsy_gpio *p) { (void)p; return 0; }
    void dsy_gpio_write(const dsy_gpio *p, uint8_t state) { (void)p; (void)state; }
    void dsy_gpio_toggle(const dsy_gpio *p) { (void)p; }
}
