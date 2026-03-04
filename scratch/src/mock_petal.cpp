#include "daisy_petal.h"
#include <thread>
#include <chrono>

namespace daisy
{

void DaisyPetal::Init(bool boost)
{
    (void)boost;
    sample_rate_ = 48000.f;
    block_size_  = 48;

    for (int i = 0; i < KNOB_LAST; i++)
    {
        adc_values_[i] = 0;
        knob[i].Init(&adc_values_[i], sample_rate_);
    }
    adc_values_[KNOB_LAST] = 0;
    expression.Init(&adc_values_[KNOB_LAST], sample_rate_);

    for (int i = 0; i < SW_LAST; i++)
    {
        dsy_gpio_pin dummy = {DSY_GPIOA, static_cast<uint8_t>(i)};
        switches[i].Init(dummy, 0.f);
    }
}

void DaisyPetal::DelayMs(size_t del)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(del));
}

void DaisyPetal::StartAudio(AudioHandle::AudioCallback cb)
{
    audio_callback_ = cb;
}

void DaisyPetal::StartAudio(AudioHandle::InterleavingAudioCallback cb)
{
    interleaving_callback_ = cb;
}

void DaisyPetal::ChangeAudioCallback(AudioHandle::AudioCallback cb)
{
    audio_callback_ = cb;
    interleaving_callback_ = nullptr;
}

void DaisyPetal::ChangeAudioCallback(AudioHandle::InterleavingAudioCallback cb)
{
    interleaving_callback_ = cb;
    audio_callback_ = nullptr;
}

void DaisyPetal::StopAudio()
{
    audio_callback_ = nullptr;
    interleaving_callback_ = nullptr;
}

float DaisyPetal::AudioSampleRate() { return sample_rate_; }

void DaisyPetal::SetAudioBlockSize(size_t size) { block_size_ = size; }

size_t DaisyPetal::AudioBlockSize() { return block_size_; }

float DaisyPetal::AudioCallbackRate() { return sample_rate_ / static_cast<float>(block_size_); }

void DaisyPetal::ProcessAnalogControls()
{
    for (int i = 0; i < KNOB_LAST; i++)
        knob[i].Process();
    expression.Process();
}

void DaisyPetal::ProcessDigitalControls()
{
    for (int i = 0; i < SW_LAST; i++)
        switches[i].Debounce();
}

float DaisyPetal::GetKnobValue(Knob k) { return knob[k].Value(); }
float DaisyPetal::GetExpression() { return expression.Value(); }

} // namespace daisy
