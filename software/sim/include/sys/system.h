#pragma once
#include <cstdint>

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
