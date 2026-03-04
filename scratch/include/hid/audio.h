#pragma once
#include <cstddef>

namespace daisy
{

class SaiHandle
{
  public:
    struct Config
    {
        enum class SampleRate { SAI_8KHZ, SAI_16KHZ, SAI_32KHZ, SAI_48KHZ, SAI_96KHZ };
    };
};

class AudioHandle
{
  public:
    typedef const float *const *InputBuffer;
    typedef float **OutputBuffer;
    typedef void (*AudioCallback)(InputBuffer in, OutputBuffer out, size_t size);

    typedef const float *InterleavingInputBuffer;
    typedef float *InterleavingOutputBuffer;
    typedef void (*InterleavingAudioCallback)(InterleavingInputBuffer in,
                                              InterleavingOutputBuffer out,
                                              size_t size);
};

} // namespace daisy
