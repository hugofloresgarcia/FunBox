#include "daisy_seed.h"
#include <thread>
#include <chrono>

namespace daisy
{

void DaisySeed::DelayMs(size_t del)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(del));
}

dsy_gpio_pin DaisySeed::GetPin(uint8_t pin_idx)
{
    return dsy_pin(DSY_GPIOA, pin_idx);
}

void DaisySeed::StartAudio(AudioHandle::InterleavingAudioCallback cb) { (void)cb; }
void DaisySeed::StartAudio(AudioHandle::AudioCallback cb) { (void)cb; }
void DaisySeed::ChangeAudioCallback(AudioHandle::InterleavingAudioCallback cb) { (void)cb; }
void DaisySeed::ChangeAudioCallback(AudioHandle::AudioCallback cb) { (void)cb; }

float DaisySeed::AudioSampleRate() { return 48000.f; }
void  DaisySeed::SetAudioBlockSize(size_t blocksize) { (void)blocksize; }
size_t DaisySeed::AudioBlockSize() { return 48; }
float  DaisySeed::AudioCallbackRate() const { return 48000.f / 48.f; }

} // namespace daisy
