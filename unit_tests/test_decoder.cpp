#include <gtest/gtest.h>
#include <cmath>
#include <vector>
#include <deque>
#include <random>
#include <zlib.h>
#include "decoder.h"

namespace qpsk::test
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
constexpr uint32_t kPacketsPerPage = kPageSize / kPacketSize;
constexpr uint32_t kDataLength = kPageSize * 10 + 1;

const uint8_t kPacketPreamble[] =
{
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x99, 0x99, 0x99, 0x99,
    0xCC, 0xCC, 0xCC, 0xCC,
};

constexpr double kFlashWriteBlankDuration = 0.05;

constexpr double kSymbolsPerPacket =
    (sizeof(kPacketPreamble) + kPacketSize + 4) * 4;
constexpr double kSamplesPerPacket = kSymbolsPerPacket * kSamplesPerSymbol;
constexpr double kPacketDuration = kSamplesPerPacket / kEncoderSampleRate;
constexpr double kPageDuration =
    kFlashWriteBlankDuration + kPacketsPerPage * kPacketDuration;
constexpr double kNumPages = ceil(kDataLength * 1.0 / kPageSize);
constexpr double kApproxSignalDuration = 3 + kNumPages * kPageDuration;
constexpr uint32_t kFifoCapacity =
    lround(kApproxSignalDuration * kEncoderSampleRate * kResamplingRatio);

static Decoder <kSamplesPerSymbol, kPacketSize, kPageSize, kFifoCapacity> qpsk_;
static std::vector<uint8_t> data_;
static std::deque<int16_t> signal_;

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

    case ERROR_PAGE_WRITE:
        printf("Error : page write\n");
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

void EncodeSymbol(std::deque<int16_t>& signal, uint8_t symbol)
{
    // assumes the symbol rate is equal to the carrier frequency
    int16_t msb = (symbol & 2) - 1;
    int16_t lsb = (symbol & 1) * 2 - 1;

    for (uint32_t i = 0; i < kSamplesPerSymbol; i++)
    {
        double phase = 2 * M_PI * i / kSamplesPerSymbol;
        double sample = (msb * cos(phase) - lsb * sin(phase)) / M_SQRT2;
        assert(sample >= -1 && sample <= 1);
        signal.push_back(lround(32767.0 * sample));
    }
}

void CodeBlank(std::deque<int16_t>& signal, double duration)
{
    uint32_t num_zeros = lround(duration * kSymbolRate);

    for (uint32_t i = 0; i < num_zeros; i++)
    {
        EncodeSymbol(signal, 0);
    }
}

void CodeIntro(std::deque<int16_t>& signal)
{
    uint32_t num_zeros = kEncoderSampleRate;

    for (uint32_t i = 0; i < num_zeros; i++)
    {
        signal.push_back(0);
    }

    CodeBlank(signal, 1.0);
}

void CodeOutro(std::deque<int16_t>& signal)
{
    CodeBlank(signal, 1.0);
}

void EncodeByte(std::deque<int16_t>& signal, uint8_t byte)
{
    EncodeSymbol(signal, (byte >> 6) & 3);
    EncodeSymbol(signal, (byte >> 4) & 3);
    EncodeSymbol(signal, (byte >> 2) & 3);
    EncodeSymbol(signal, (byte >> 0) & 3);
}

void CodePacket(std::deque<int16_t>& signal, uint8_t *data)
{
    uint32_t crc = crc32(kCRCSeed, data, kPacketSize);

    for (uint32_t i = 0; i < sizeof(kPacketPreamble); i++)
    {
        EncodeByte(signal, kPacketPreamble[i]);
    }

    for (uint32_t i = 0; i < kPacketSize; i++)
    {
        EncodeByte(signal, data[i]);
    }

    EncodeByte(signal, crc >> 24);
    EncodeByte(signal, crc >> 16);
    EncodeByte(signal, crc >> 8);
    EncodeByte(signal, crc);
}

void CodeData(std::deque<int16_t>& signal, std::vector<uint8_t>& data)
{
    while (data.size() % kPageSize)
    {
        data.push_back(0xFF);
    }

    uint32_t num_pages = data.size() / kPageSize;

    for (uint32_t page = 0; page < num_pages; page++)
    {
        for (uint32_t packet = 0; packet < kPacketsPerPage; packet++)
        {
            uint32_t offset = page * kPageSize + packet * kPacketSize;
            CodePacket(signal, &data[offset]);
        }

        CodeBlank(signal, kFlashWriteBlankDuration);
    }
}

void Resample(std::deque<int16_t>& signal, double ratio)
{
    std::deque<int16_t> resampled;
    uint32_t length = floor(signal.size() * ratio);

    for (uint32_t i = 0; i < length; i++)
    {
        double position = i / ratio;
        uint32_t x0 = floor(position);
        int16_t y0 = signal[x0];
        int16_t y1 = signal[x0 + 1];
        position = fmod(position, 1.0);
        resampled.push_back(y0 + position * (y1 - y0));
    }

    signal = resampled;
}

void AddNoise(std::deque<int16_t>& signal, double noise_level)
{
    std::minstd_rand rng;
    std::uniform_real_distribution<double> dist(-1, 1);

    for (auto it = signal.begin(); it != signal.end(); it++)
    {
        double sample = *it / 32767.0;
        sample += noise_level * dist(rng);
        sample = (sample > 1) ? 1 : (sample < -1) ? -1 : sample;
        *it = sample * 32767.0;
    }
}

bool ReceivePage(uint32_t* data)
{
    for (uint32_t i = 0; i < kPageSize / 4; i++)
    {
        data_.push_back(data[i] >>  0);
        data_.push_back(data[i] >>  8);
        data_.push_back(data[i] >> 16);
        data_.push_back(data[i] >> 24);
    }

    return true;
}

TEST(DecoderTest, Decode)
{
    ASSERT_EQ(kPageSize % kPacketSize, 0);
    qpsk_.Init(kCRCSeed);
    data_.clear();

    // generate random data to encode
    std::minstd_rand rng;
    std::uniform_int_distribution<uint8_t> dist(0, 0xFF);
    rng.seed(0);
    std::vector<uint8_t> input_data;
    for (uint32_t i = 0; i < kDataLength; i++)
    {
        input_data.push_back(dist(rng));
    }

    // encode the qpsk signal
    CodeIntro(signal_);
    CodeData(signal_, input_data);
    CodeOutro(signal_);
    Resample(signal_, kResamplingRatio);
    AddNoise(signal_, 0.0625);

    uint32_t signal_length = signal_.size();
    ASSERT_LE(signal_length, kFifoCapacity);

    // push the signal into the qpsk module's fifo
    while (!signal_.empty())
    {
        qpsk_.Push(signal_.front() / 32768.f * 0.1);
        signal_.pop_front();
    }

    // begin decoding
    Error error = qpsk_.Receive(ReceivePage, signal_length);

    if (error != ERROR_NONE)
    {
        ReceiveError(error);
        FAIL();
    }

    ASSERT_GE(data_.size(), kDataLength);

    // reseed the rng and check that we got back identical data
    rng.seed(0);

    for (uint32_t i = 0; i < data_.size(); i++)
    {
        uint8_t expected = (i < kDataLength) ? dist(rng) : 0xFF;
        ASSERT_EQ(data_[i], expected) << "at i = " << i;
    }
}

}
