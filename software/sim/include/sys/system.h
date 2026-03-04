#pragma once
#include <cstdint>
#include <stdexcept>

namespace sim
{
    struct InitDone : std::exception { const char* what() const noexcept override { return "sim init done"; } };
    extern bool break_on_delay;
}

namespace daisy
{

class System
{
  public:
    static uint32_t GetNow();
    static uint32_t GetUs();
    static void     Delay(uint32_t delay_ms);
    static void     DelayUs(uint32_t delay_us);
};

} // namespace daisy
