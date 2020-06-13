// MIT License
//
// Copyright 2020 Tyler Coy
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include <cmath>
#include <random>
#include <string>
#include <vector>
#include <cassert>
#include <fstream>
#include <sstream>

namespace qpsk::test::util
{

template <typename T>
inline T Resample(T signal, double ratio)
{
    if (ratio != 1.0)
    {
        T resampled;
        uint32_t length = floor(signal.size() * ratio);

        for (uint32_t i = 0; i < length; i++)
        {
            double position = i / ratio;
            uint32_t x0 = floor(position);
            float y0 = signal[x0];
            float y1 = signal[x0 + 1];
            position = fmod(position, 1.0);
            resampled.push_back(y0 + position * (y1 - y0));
        }

        return resampled;
    }
    else
    {
        return signal;
    }
}

template <typename T>
inline T Scale(T signal, float level)
{
    if (level != 1.f)
    {
        for (auto it = signal.begin(); it != signal.end(); it++)
        {
            *it *= level;
        }
    }

    return signal;
}

template <typename T>
inline T AddNoise(T signal, float noise_level)
{
    if (noise_level != 0.f)
    {
        std::minstd_rand rng;
        std::uniform_real_distribution<float> dist(-1, 1);

        for (auto it = signal.begin(); it != signal.end(); it++)
        {
            float sample = *it;
            sample += noise_level * dist(rng);
            sample = (sample > 1) ? 1 : (sample < -1) ? -1 : sample;
            *it = sample;
        }
    }

    return signal;
}

template <typename T>
inline T LoadAudio(std::string file_path)
{
    std::ifstream wav_file;
    wav_file.open(file_path, std::ios::in | std::ios::binary);
    assert(wav_file.good());
    wav_file.seekg(44);
    T signal;
    for (;;)
    {
        int16_t sample = (wav_file.get() & 0xFF);
        sample |= (wav_file.get() << 8);
        if (wav_file.eof())
        {
            break;
        }
        signal.push_back(sample / 32767.f);
    }
    wav_file.close();
    return signal;
}

template <typename T>
inline T LoadAudio(std::string bin_file_path,
    int symbol_rate, int packet_size, int page_size,
    float write_time = 0.05f)
{
    T signal;
    std::stringstream ss;
    ss << "python3 encoder.py -s 48000 -t bin -o -"
        << " -i " << bin_file_path
        << " -y " << symbol_rate
        << " -p " << packet_size
        << " -b " << page_size
        << " -w " << (write_time / 2 * 1000)
        << " -f " << page_size << ":" << (write_time / 2 * 1000)
        << " -a 0";
    std::string cmd = ss.str();
    auto wav_file = popen(cmd.c_str(), "r");
    fseek(wav_file, 44, SEEK_SET);
    for (;;)
    {
        int16_t sample = (fgetc(wav_file) & 0xFF);
        sample |= (fgetc(wav_file) << 8);
        if (feof(wav_file))
        {
            break;
        }
        signal.push_back(sample / 32767.f);
    }
    pclose(wav_file);
    return signal;
}

inline std::vector<uint8_t> LoadBinary(std::string file_path)
{
    std::ifstream bin_file;
    bin_file.open(file_path, std::ios::in | std::ios::binary);
    assert(bin_file.good());
    std::vector<uint8_t> bin_data;
    for (;;)
    {
        uint32_t byte = bin_file.get();
        if (bin_file.eof())
        {
            break;
        }
        bin_data.push_back(byte);
    }
    bin_file.close();
    return bin_data;
}

}
