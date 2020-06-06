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

#include <cmath>
#include <deque>
#include <tuple>
#include <cstdio>
#include <vector>
#include <random>
#include <string>
#include <fstream>
#include <sstream>
#include <type_traits>
#include <gtest/gtest.h>
#include "decoder.h"

namespace qpsk::test::decoder
{

constexpr uint32_t kSampleRate = 48000;
constexpr uint32_t kCRCSeed = 0;
constexpr uint8_t kFillByte = 0xFF;
constexpr float kFlashWriteTime = 0.025f;

template <int A, int B, int C>
using ParamType = std::tuple<
    std::integral_constant<int, A>,
    std::integral_constant<int, B>,
    std::integral_constant<int, C>>;
using ParamTypeList = ::testing::Types<
    /*[[[cog
    from data.encodings import GenerateEncodings
    lines = []
    for e in GenerateEncodings():
        lines.append('ParamType<{:2}, {:4}, {:5}>'
            .format(e.symbol_duration, e.packet_size, e.page_size))
    cog.outl(',\n'.join(lines))
    ]]]*/
    ParamType< 6,   52,    52>,
    ParamType< 6,   52,   208>,
    ParamType< 6,   52,   364>,
    ParamType< 6,  256,   256>,
    ParamType< 6,  256,  1024>,
    ParamType< 6,  256,  1792>,
    ParamType< 8,   52,    52>,
    ParamType< 8,   52,   208>,
    ParamType< 8,   52,   364>,
    ParamType< 8,  256,   256>,
    ParamType< 8,  256,  1024>,
    ParamType< 8,  256,  1792>,
    ParamType<12,   52,    52>,
    ParamType<12,   52,   208>,
    ParamType<12,   52,   364>,
    ParamType<12,  256,   256>,
    ParamType<12,  256,  1024>,
    ParamType<12,  256,  1792>,
    ParamType<16,   52,    52>,
    ParamType<16,   52,   208>,
    ParamType<16,   52,   364>,
    ParamType<16,  256,   256>,
    ParamType<16,  256,  1024>,
    ParamType<16,  256,  1792>
    //[[[end]]]
    >;

template <typename T>
class DecoderTest : public ::testing::Test
{
public:
    using Signal = std::deque<float>;
    static inline Signal test_audio_;
    static inline std::vector<uint8_t> test_data_;

    static constexpr int kSymbolDuration = std::tuple_element_t<0, T>::value;
    static constexpr int kPacketSize     = std::tuple_element_t<1, T>::value;
    static constexpr int kPageSize       = std::tuple_element_t<2, T>::value;
    Decoder<kSymbolDuration, kPacketSize, kPageSize, 256> qpsk_;

    static constexpr int kCarrierRate = kSampleRate / kSymbolDuration;

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
        if (ratio != 1.0)
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
        else
        {
            return signal;
        }
    }

    Signal Scale(Signal signal, float level)
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

    Signal AddNoise(Signal signal, float noise_level)
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

    static Signal LoadAudio(std::string file_path)
    {
        std::ifstream wav_file;
        wav_file.open(file_path, std::ios::in | std::ios::binary);
        assert(wav_file.good());
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

    static Signal LoadAudio(int carrier_rate, int packet_size, int page_size)
    {
        Signal signal;
        std::stringstream ss;
        ss << "python3 encoder.py -s 48000 -w 0.05 -t bin"
            << " -i unit_tests/data/data.bin -o -"
            << " -c " << carrier_rate
            << " -p " << packet_size
            << " -f " << page_size;
        std::string cmd = ss.str();
        auto wav_file = popen(cmd.c_str(), "r");
        fseek(wav_file, 44, SEEK_SET);
        while (!feof(wav_file))
        {
            int16_t sample = (fgetc(wav_file) & 0xFF);
            sample |= (fgetc(wav_file) << 8);
            signal.push_back(sample / 32767.f);
        }
        pclose(wav_file);
        return signal;
    }

    static std::vector<uint8_t> LoadBinary(std::string file_path)
    {
        std::ifstream bin_file;
        bin_file.open(file_path, std::ios::in | std::ios::binary);
        assert(bin_file.good());
        std::vector<uint8_t> bin_data;
        while (!bin_file.eof())
        {
            bin_data.push_back(bin_file.get());
        }
        bin_file.close();
        return bin_data;
    }

    static void SetUpTestCase()
    {
        test_data_ = LoadBinary("unit_tests/data/data.bin");
        test_audio_ = LoadAudio(kCarrierRate, kPacketSize, kPageSize);
    }

    void SetUp() override
    {
        qpsk_.Init(kCRCSeed);
    }

    void Decode(float resampling_ratio, float signal_level, float noise_level)
    {
        Signal signal = test_audio_;
        // Resample, attenuate, and add noise
        signal = Resample(signal, resampling_ratio);
        signal = Scale(signal, signal_level);
        signal = AddNoise(signal, noise_level);

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
                for (int i = 0; i < kFlashWriteTime * kSampleRate; i++)
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
        ASSERT_GE(data.size(), test_data_.size());

        for (uint32_t i = 0; i < data.size(); i++)
        {
            uint8_t expected = (i < test_data_.size()) ?
                test_data_[i] : kFillByte;
            ASSERT_EQ(data[i], expected) << "at i = " << i;
        }
    }
};

TYPED_TEST_CASE(DecoderTest, ParamTypeList);

TYPED_TEST(DecoderTest, Noisy)
{
    this->Decode(1.f, 0.1f, 0.01f);
}

TYPED_TEST(DecoderTest, Upsampled)
{
    this->Decode(1.05f, 1.f, 0.f);
}

TYPED_TEST(DecoderTest, Downsampled)
{
    this->Decode(0.95f, 1.f, 0.f);
}

TYPED_TEST(DecoderTest, Imperfect)
{
    this->Decode(0.98f, 0.5f, 0.01f);
}

}
