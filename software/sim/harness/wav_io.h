#pragma once
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>
#include <string>
#include <stdexcept>

namespace wav
{

struct WavData
{
    uint32_t sample_rate  = 48000;
    uint16_t num_channels = 1;
    uint16_t bits_per_sample = 16;
    std::vector<std::vector<float>> channels; // channels[ch][sample]
};

inline WavData read_wav(const std::string &path)
{
    FILE *f = fopen(path.c_str(), "rb");
    if (!f) throw std::runtime_error("Cannot open: " + path);

    char riff_id[4];
    uint32_t file_size;
    char wave_id[4];
    fread(riff_id, 1, 4, f);
    fread(&file_size, 4, 1, f);
    fread(wave_id, 1, 4, f);

    if (memcmp(riff_id, "RIFF", 4) != 0 || memcmp(wave_id, "WAVE", 4) != 0)
    {
        fclose(f);
        throw std::runtime_error("Not a WAV file: " + path);
    }

    WavData wav;
    bool found_fmt = false, found_data = false;
    uint16_t audio_format = 0;

    while (!feof(f) && !found_data)
    {
        char chunk_id[4];
        uint32_t chunk_size;
        if (fread(chunk_id, 1, 4, f) != 4) break;
        if (fread(&chunk_size, 4, 1, f) != 1) break;

        if (memcmp(chunk_id, "fmt ", 4) == 0)
        {
            fread(&audio_format, 2, 1, f);
            fread(&wav.num_channels, 2, 1, f);
            fread(&wav.sample_rate, 4, 1, f);
            uint32_t byte_rate;
            uint16_t block_align;
            fread(&byte_rate, 4, 1, f);
            fread(&block_align, 2, 1, f);
            fread(&wav.bits_per_sample, 2, 1, f);
            long remaining = (long)chunk_size - 16;
            if (remaining > 0) fseek(f, remaining, SEEK_CUR);
            found_fmt = true;
        }
        else if (memcmp(chunk_id, "data", 4) == 0)
        {
            if (!found_fmt)
            {
                fclose(f);
                throw std::runtime_error("WAV data before fmt chunk");
            }

            uint32_t num_samples = chunk_size / (wav.num_channels * (wav.bits_per_sample / 8));
            wav.channels.resize(wav.num_channels);
            for (auto &ch : wav.channels) ch.resize(num_samples);

            if (audio_format == 1 && wav.bits_per_sample == 16)
            {
                for (uint32_t i = 0; i < num_samples; i++)
                {
                    for (uint16_t ch = 0; ch < wav.num_channels; ch++)
                    {
                        int16_t s;
                        fread(&s, 2, 1, f);
                        wav.channels[ch][i] = static_cast<float>(s) / 32768.f;
                    }
                }
            }
            else if (audio_format == 1 && wav.bits_per_sample == 24)
            {
                for (uint32_t i = 0; i < num_samples; i++)
                {
                    for (uint16_t ch = 0; ch < wav.num_channels; ch++)
                    {
                        uint8_t b[3];
                        fread(b, 1, 3, f);
                        int32_t s = (b[2] << 24) | (b[1] << 16) | (b[0] << 8);
                        s >>= 8;
                        wav.channels[ch][i] = static_cast<float>(s) / 8388608.f;
                    }
                }
            }
            else if (audio_format == 3 && wav.bits_per_sample == 32)
            {
                for (uint32_t i = 0; i < num_samples; i++)
                {
                    for (uint16_t ch = 0; ch < wav.num_channels; ch++)
                    {
                        float s;
                        fread(&s, 4, 1, f);
                        wav.channels[ch][i] = s;
                    }
                }
            }
            else
            {
                fclose(f);
                throw std::runtime_error("Unsupported WAV format: fmt=" + std::to_string(audio_format)
                                         + " bits=" + std::to_string(wav.bits_per_sample));
            }
            found_data = true;
        }
        else
        {
            fseek(f, chunk_size, SEEK_CUR);
        }
    }

    fclose(f);
    if (!found_data) throw std::runtime_error("No data chunk found in: " + path);
    return wav;
}

inline void write_wav(const std::string &path, const WavData &wav)
{
    if (wav.channels.empty() || wav.channels[0].empty())
        throw std::runtime_error("Empty WAV data");

    FILE *f = fopen(path.c_str(), "wb");
    if (!f) throw std::runtime_error("Cannot create: " + path);

    uint16_t num_ch  = static_cast<uint16_t>(wav.channels.size());
    uint32_t num_smp = static_cast<uint32_t>(wav.channels[0].size());
    uint16_t bps     = 16;
    uint16_t fmt     = 1;
    uint32_t byte_rate   = wav.sample_rate * num_ch * (bps / 8);
    uint16_t block_align = num_ch * (bps / 8);
    uint32_t data_size   = num_smp * num_ch * (bps / 8);
    uint32_t file_size   = 36 + data_size;

    fwrite("RIFF", 1, 4, f);
    fwrite(&file_size, 4, 1, f);
    fwrite("WAVE", 1, 4, f);

    fwrite("fmt ", 1, 4, f);
    uint32_t fmt_size = 16;
    fwrite(&fmt_size, 4, 1, f);
    fwrite(&fmt, 2, 1, f);
    fwrite(&num_ch, 2, 1, f);
    fwrite(&wav.sample_rate, 4, 1, f);
    fwrite(&byte_rate, 4, 1, f);
    fwrite(&block_align, 2, 1, f);
    fwrite(&bps, 2, 1, f);

    fwrite("data", 1, 4, f);
    fwrite(&data_size, 4, 1, f);

    for (uint32_t i = 0; i < num_smp; i++)
    {
        for (uint16_t ch = 0; ch < num_ch; ch++)
        {
            float s = wav.channels[ch][i];
            if (s > 1.f)  s = 1.f;
            if (s < -1.f) s = -1.f;
            int16_t sample = static_cast<int16_t>(s * 32767.f);
            fwrite(&sample, 2, 1, f);
        }
    }

    fclose(f);
}

inline void write_wav_stereo(const std::string &path,
                              const std::vector<float> &left,
                              const std::vector<float> &right,
                              uint32_t sample_rate = 48000)
{
    WavData wav;
    wav.sample_rate = sample_rate;
    wav.channels.resize(2);
    wav.channels[0] = left;
    wav.channels[1] = right;
    write_wav(path, wav);
}

} // namespace wav
