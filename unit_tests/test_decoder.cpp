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

#include <gtest/gtest.h>
#include <cmath>
#include <deque>
#include <vector>
#include <random>
#include <string>
#include <fstream>
#include "decoder.h"

namespace qpsk::test::decoder
{

constexpr uint32_t kDecoderSampleRate = 48077;
constexpr uint32_t kEncoderSampleRate = 48000;
constexpr double kResamplingRatio =
    kDecoderSampleRate * 1.0 / kEncoderSampleRate;

constexpr uint32_t kSymbolRate = 6000;
constexpr uint32_t kPageSize = 1024;
constexpr uint32_t kPacketSize = 256;
constexpr uint32_t kCRCSeed = 0;
constexpr uint32_t kSamplesPerSymbol = kEncoderSampleRate / kSymbolRate;
constexpr uint8_t kFillByte = 0xFF;
constexpr float kFlashWriteTime = 0.025f;

static Decoder<kSamplesPerSymbol, kPacketSize, kPageSize, 256> qpsk_;
using Signal = std::deque<float>;

void DebugError(Error error)
{
    printf("  PLL freq         : %li\n"
           "  Decision phase   : %li\n"
           "  Signal power     : %li\n",
           std::lround(1000 * qpsk_.PllPhaseIncrement()),
           std::lround(1000 * qpsk_.DecisionPhase()),
           std::lround(1000 * qpsk_.SignalPower()));

    if (error == ERROR_SYNC)
    {
        printf("  Recent symbols   :");

        while (qpsk_.SymbolsAvailable() > 16)
        {
            qpsk_.PopSymbol();
        }

        while (qpsk_.SymbolsAvailable())
        {
            printf(" %u", qpsk_.PopSymbol());
        }

        printf("\n");
        printf("  Expected symbols : ");

        auto mask = qpsk_.ExpectedSymbolMask();

        for (uint32_t i = 0; i < 5; i++)
        {
            if (mask & 1)
            {
                printf("%u", i);

                if (mask >> 1)
                {
                    printf(", ");
                }
            }

            mask >>= 1;
        }

        printf("\n");
    }
    else if (error == ERROR_CRC)
    {
        printf("  Packet data      :\n");

        for (uint32_t row = 0; row < kPacketSize; row += 16)
        {
            printf("    ");

            for (uint32_t col = 0; col < 16; col++)
            {
                printf("%02X ", qpsk_.GetPacket()[row + col]);
            }

            printf("\n");
        }

        printf("  Calculated CRC   : 0x%08X\n", qpsk_.CalculatedCRC());
        printf("  Expected CRC     : 0x%08X\n", qpsk_.ExpectedCRC());
    }
}

void ReceiveError(Error error)
{
    switch (error)
    {
    case ERROR_SYNC:
        printf("Error : sync\n");
        DebugError(error);
        break;

    case ERROR_CRC:
        printf("Error : CRC\n");
        DebugError(error);
        break;

    case ERROR_OVERFLOW:
        printf("Error : overflow\n");
        break;

    case ERROR_ABORT:
        printf("Error : abort\n");
        break;

    case ERROR_TIMEOUT:
        printf("Error : timeout\n");
        break;

    case ERROR_NONE:
        break;
    }
}

Signal Resample(Signal signal, double ratio)
{
    Signal resampled;
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

Signal Scale(Signal signal, float level)
{
    for (auto it = signal.begin(); it != signal.end(); it++)
    {
        *it *= level;
    }

    return signal;
}

Signal AddNoise(Signal signal, float noise_level)
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

    return signal;
}

Signal LoadAudio(std::string file_path)
{
    std::ifstream wav_file;
    wav_file.open(file_path, std::ios::in | std::ios::binary);
    wav_file.seekg(44);
    Signal signal;
    while (!wav_file.eof())
    {
        int16_t sample = (wav_file.get() & 0xFF);
        sample |= (wav_file.get() << 8);
        signal.push_back(sample / 32767.f);
    }
    wav_file.close();
    return signal;
}

std::vector<uint8_t> LoadBinary(std::string file_path)
{
    std::ifstream bin_file;
    bin_file.open(file_path, std::ios::in | std::ios::binary);
    std::vector<uint8_t> bin_data;
    while (!bin_file.eof())
    {
        bin_data.push_back(bin_file.get());
    }
    bin_file.close();
    return bin_data;
}

TEST(DecoderTest, Decode)
{
    qpsk_.Init(kCRCSeed);

    // Load audio data from wav file
    auto signal = LoadAudio("unit_tests/data.wav");

    // Resample, attenuate, and add noise
    signal = Resample(signal, kResamplingRatio);
    signal = Scale(signal, 0.1f);
    signal = AddNoise(signal, 0.025);

    // Begin decoding
    std::vector<uint8_t> data;
    while (signal.size())
    {
        float sample = signal.front();
        signal.pop_front();
        qpsk_.Push(sample);
        auto result = qpsk_.Receive();

        if (result == RESULT_ERROR)
        {
            ReceiveError(qpsk_.GetError());
            FAIL();
        }
        else if (result == RESULT_PAGE_COMPLETE)
        {
            uint32_t* page = qpsk_.GetPage();
            for (uint32_t i = 0; i < kPageSize / 4; i++)
            {
                data.push_back(page[i] >>  0);
                data.push_back(page[i] >>  8);
                data.push_back(page[i] >> 16);
                data.push_back(page[i] >> 24);
            }

            // Simulate stall caused by flash write
            for (int i = 0; i < kFlashWriteTime * kDecoderSampleRate; i++)
            {
                if (!signal.size())
                {
                    break;
                }
                float sample = signal.front();
                signal.pop_front();
                qpsk_.Push(sample);
            }
        }
    }

    // Compare the received data to the bin file
    auto bin_data = LoadBinary("unit_tests/data.bin");

    ASSERT_GE(data.size(), bin_data.size());

    for (uint32_t i = 0; i < data.size(); i++)
    {
        uint8_t expected = (i < bin_data.size()) ? bin_data[i] : kFillByte;
        ASSERT_EQ(data[i], expected) << "at i = " << i;
    }
}

}
