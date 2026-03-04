#pragma once
#ifndef DSY_CORE_HW_H
#define DSY_CORE_HW_H
#include <stdint.h>
#include <stdlib.h>

#if defined(_MSC_VER)
#define FORCE_INLINE __forceinline
#elif defined(__clang__) || defined(__GNUC__)
#define FORCE_INLINE inline __attribute__((always_inline))
#else
#define FORCE_INLINE inline
#endif

#define DMA_BUFFER_MEM_SECTION
#define DTCM_MEM_SECTION

#define FBIPMAX 0.999985f
#define FBIPMIN (-FBIPMAX)
#define S162F_SCALE 3.0517578125e-05f
#define F2S16_SCALE 32767.0f
#define F2S24_SCALE 8388608.0f
#define S242F_SCALE 1.192092896e-07f
#define S24SIGN 0x800000
#define S322F_SCALE 4.6566129e-10f
#define F2S32_SCALE 2147483647.f

#define OUT_L out[0]
#define OUT_R out[1]
#define IN_L in[0]
#define IN_R in[1]

FORCE_INLINE float cube(float x) { return (x * x) * x; }

typedef enum
{
    DSY_GPIOA, DSY_GPIOB, DSY_GPIOC, DSY_GPIOD,
    DSY_GPIOE, DSY_GPIOF, DSY_GPIOG, DSY_GPIOH,
    DSY_GPIOI, DSY_GPIOJ, DSY_GPIOK, DSY_GPIOX,
    DSY_GPIO_LAST,
} dsy_gpio_port;

typedef struct
{
    dsy_gpio_port port;
    uint8_t       pin;
} dsy_gpio_pin;

FORCE_INLINE dsy_gpio_pin dsy_pin(dsy_gpio_port port, uint8_t pin)
{
    dsy_gpio_pin p;
    p.port = port;
    p.pin  = pin;
    return p;
}

#ifdef __cplusplus

namespace daisy
{

enum GPIOPort
{
    PORTA, PORTB, PORTC, PORTD, PORTE, PORTF,
    PORTG, PORTH, PORTI, PORTJ, PORTK, PORTX,
};

struct Pin
{
    GPIOPort port;
    uint8_t  pin;

    constexpr Pin(const GPIOPort pt, const uint8_t pn) : port(pt), pin(pn) {}
    constexpr Pin() : port(PORTX), pin(255) {}
    constexpr bool IsValid() const { return port != PORTX && pin < 16; }
    constexpr bool operator==(const Pin &rhs) const { return rhs.port == port && rhs.pin == pin; }
    constexpr bool operator!=(const Pin &rhs) const { return !operator==(rhs); }
    constexpr operator dsy_gpio_pin() const
    {
        return dsy_gpio_pin{.port = static_cast<dsy_gpio_port>(port), .pin = pin};
    }
};

} // namespace daisy

#endif
#endif
