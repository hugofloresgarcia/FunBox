#include "sys/system.h"
#include <chrono>
#include <thread>

namespace daisy
{

static auto start_time = std::chrono::steady_clock::now();

uint32_t System::GetNow()
{
    auto now = std::chrono::steady_clock::now();
    return static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count());
}

uint32_t System::GetUs()
{
    auto now = std::chrono::steady_clock::now();
    return static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(now - start_time).count());
}

void System::Delay(uint32_t delay_ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
}

void System::DelayUs(uint32_t delay_us)
{
    std::this_thread::sleep_for(std::chrono::microseconds(delay_us));
}

} // namespace daisy
