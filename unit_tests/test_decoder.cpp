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
#include <tuple>
#include <string>
#include <vector>
#include <cstdint>
#include <type_traits>
#include <gtest/gtest.h>
#include "decoder.h"
#include "unit_tests/util.h"

namespace qpsk::test::decoder
{

constexpr uint32_t kSampleRate = 48000;
constexpr uint32_t kCRCSeed = 0;
constexpr uint8_t kFillByte = 0xFF;
constexpr float kFlashWriteTime = 0.025f;

using Signal = std::vector<float>;

template <int A, int B, int C>
using ParamType = std::tuple<
    std::integral_constant<int, A>,
    std::integral_constant<int, B>,
    std::integral_constant<int, C>>;
using ParamTypeList = ::testing::Types<
    /*[[[cog
    import itertools
    lines = []
    symbol_duration = (6, 8, 12, 16)
    packet_size = (52, 256)
    num_packets = (1, 4, 7)
    encodings = itertools.product(symbol_duration, packet_size, num_packets)
    for (symbol_duration, packet_size, num_packets) in encodings:
        block_size = packet_size * num_packets
        lines.append('ParamType<{:2}, {:4}, {:5}>'
            .format(symbol_duration, packet_size, block_size))
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
    static inline Signal test_audio_;
    static inline std::vector<uint8_t> test_data_;

    static constexpr int kSymbolDuration = std::tuple_element_t<0, T>::value;
    static constexpr int kPacketSize     = std::tuple_element_t<1, T>::value;
    static constexpr int kBlockSize      = std::tuple_element_t<2, T>::value;
    static constexpr int kSymbolRate     = kSampleRate / kSymbolDuration;
    Decoder<kSampleRate, kSymbolRate, kPacketSize, kBlockSize, 256> qpsk_;

    void DebugError(Error error)
    {
        printf("  PLL freq         : %li\n"
               "  Decision phase   : %li\n"
               "  Signal power     : %li\n",
               std::lround(1000 * qpsk_.pll_step()),
               std::lround(1000 * qpsk_.decision_phase()),
               std::lround(1000 * qpsk_.signal_power()));

        if (error == ERROR_CRC)
        {
            printf("  Packet data      :\n");

            for (uint32_t row = 0; row < kPacketSize; row += 16)
            {
                printf("    ");

                for (uint32_t col = 0; col < 16; col++)
                {
                    if (row + col >= kPacketSize)
                    {
                        break;
                    }

                    printf("%02X ", qpsk_.packet_data()[row + col]);
                }

                printf("\n");
            }
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

        case ERROR_NONE:
            break;
        }
    }

    static void SetUpTestCase()
    {
        std::string bin_file = "unit_tests/data/data.bin";
        test_data_ = util::LoadBinary(bin_file);
        test_audio_ = util::LoadAudio<Signal>(bin_file,
            kSymbolRate, kPacketSize, kBlockSize);
    }

    void SetUp() override
    {
        qpsk_.Init(kCRCSeed);
    }

    void Decode(float resampling_ratio, float signal_level, float noise_level)
    {
        Signal signal = test_audio_;
        // Resample, attenuate, and add noise
        signal = util::Resample(signal, resampling_ratio);
        signal = util::Scale(signal, signal_level);
        signal = util::AddNoise(signal, noise_level);

        int flash_write_delay = 0;

        // Begin decoding
        Result result;
        std::vector<uint8_t> data;
        for (auto sample : signal)
        {
            qpsk_.Push(sample);

            if (flash_write_delay == 0)
            {
                result = qpsk_.Process();

                if (result == RESULT_ERROR)
                {
                    ReceiveError(qpsk_.error());
                    FAIL();
                }
                else if (result == RESULT_BLOCK_COMPLETE)
                {
                    const uint32_t* block = qpsk_.block_data();
                    for (uint32_t i = 0; i < kBlockSize / 4; i++)
                    {
                        data.push_back(block[i] >>  0);
                        data.push_back(block[i] >>  8);
                        data.push_back(block[i] >> 16);
                        data.push_back(block[i] >> 24);
                    }
                    flash_write_delay = kSampleRate * kFlashWriteTime;
                }
            }
            else
            {
                flash_write_delay--;
            }
        }

        ASSERT_EQ(result, RESULT_END);

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
    this->Decode(1.f, 0.1f, 0.025f);
}

TYPED_TEST(DecoderTest, Inverted)
{
    this->Decode(1.f, -1.f, 0.f);
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



class HangTest : public ::testing::Test
{
public:
    Decoder<kSampleRate, 8000, 256, 1024, 256> qpsk_;

    void SetUp() override
    {
        qpsk_.Init(kCRCSeed);
    }

    Result Run(std::string command)
    {
        auto signal = util::LoadAudioFromCommand<Signal>(command);

        int flash_write_delay = 0;
        Result result;

        for (auto sample : signal)
        {
            qpsk_.Push(sample);

            if (flash_write_delay == 0)
            {
                result = qpsk_.Process();
                if (result == RESULT_BLOCK_COMPLETE)
                {
                    flash_write_delay = kSampleRate * kFlashWriteTime;
                }
            }
            else
            {
                flash_write_delay--;
            }
        }

        return result;
    }
};

TEST_F(HangTest, Sync)
{
    // Make sure that the decoder errors out instead of hanging when the
    // carrier sync is interrupted by silence.
    auto result = Run("PYTHONPATH=. python3 unit_tests/hang.py sync");
    ASSERT_EQ(result, RESULT_ERROR);
}

TEST_F(HangTest, Prealignment)
{
    // Make sure that the decoder errors out instead of hanging when the
    // alignment sequence is interrupted by silence.
    auto result = Run("PYTHONPATH=. python3 unit_tests/hang.py prealign");
    ASSERT_EQ(result, RESULT_ERROR);
}

TEST_F(HangTest, Alignment)
{
    // Make sure that the decoder errors out instead of hanging when the
    // alignment sequence is interrupted by silence.
    auto result = Run("PYTHONPATH=. python3 unit_tests/hang.py align");
    ASSERT_EQ(result, RESULT_ERROR);
}

TEST_F(HangTest, Write)
{
    // Make sure that the decoder errors out instead of hanging when a
    // block is followed by silence.
    auto result = Run("PYTHONPATH=. python3 unit_tests/hang.py write");
    ASSERT_EQ(result, RESULT_ERROR);
}

}
