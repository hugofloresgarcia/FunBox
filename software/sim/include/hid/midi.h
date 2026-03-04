#pragma once
#include <cstdint>
#include <cstddef>
#include "daisy_core.h"

namespace daisy
{

class MidiUartTransport
{
  public:
    struct Config
    {
        int periph = 0;
        dsy_gpio_pin rx = {};
        dsy_gpio_pin tx = {};
    };
    void Init(Config config) { (void)config; }
    void StartRx(void (*cb)(uint8_t*, size_t, void*), void* ctx) { (void)cb; (void)ctx; }
};

template <typename Transport>
class MidiHandler
{
  public:
    MidiHandler() {}
    ~MidiHandler() {}
    struct Config { typename Transport::Config transport_config; };
    void Init(Config config) { (void)config; }
    void StartReceive() {}
    void Listen() {}
    bool HasEvents() { return false; }
};

using MidiUartHandler = MidiHandler<MidiUartTransport>;

} // namespace daisy
