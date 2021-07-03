// MIT License
//
// Copyright 2021 Tyler Coy
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

#include <cstdint>
#include <cmath>
#include <random>
#include <vector>
#include <gtest/gtest.h>
#include <zlib.h>
#include "qpsk/inc/packet.h"
#include "test_error_correction.h"

namespace qpsk::test::packet
{

using PacketSizes = ::testing::Types<
    std::integral_constant<uint32_t, 4>,
    std::integral_constant<uint32_t, 8>,
    std::integral_constant<uint32_t, 16>,
    std::integral_constant<uint32_t, 32>,
    std::integral_constant<uint32_t, 64>,
    std::integral_constant<uint32_t, 100>,
    std::integral_constant<uint32_t, 128>,
    std::integral_constant<uint32_t, 252>,
    std::integral_constant<uint32_t, 256>,
    std::integral_constant<uint32_t, 260>,
    std::integral_constant<uint32_t, 1000>,
    std::integral_constant<uint32_t, 4096>>;

constexpr uint32_t kPacketsPerBlock = 4;
constexpr uint32_t kCRCSeed = 420;

template <typename T>
class PacketTest : public ::testing::Test
{
protected:
    static constexpr uint32_t kPacketSize = T::value;
    static constexpr uint32_t kBlockSize = kPacketSize * kPacketsPerBlock;
    uint8_t data_[kPacketSize];
    uint32_t expected_crc_;
    error_correction::HammingEncoder hamming_;
    Packet<kPacketSize> packet_;
    Block<kBlockSize> block_;
    std::minstd_rand rng_;

    void SetUp() override
    {
        rng_.seed(0);
        RandomizeData();
        packet_.Init(kCRCSeed);
        block_.Init();
    }

    void RandomizeData(void)
    {
        std::uniform_int_distribution<uint8_t> dist(0);

        for (uint32_t i = 0; i < kPacketSize; i++)
        {
            data_[i] = dist(rng_);
        }

        expected_crc_ = crc32(kCRCSeed, data_, kPacketSize);

        hamming_.Encode(data_, kPacketSize);
        hamming_.Encode((expected_crc_ >>  0) & 0xFF);
        hamming_.Encode((expected_crc_ >>  8) & 0xFF);
        hamming_.Encode((expected_crc_ >> 16) & 0xFF);
        hamming_.Encode((expected_crc_ >> 24) & 0xFF);
    }

    void PushByte(uint8_t byte)
    {
        packet_.WriteSymbol((byte >> 6) & 3);
        packet_.WriteSymbol((byte >> 4) & 3);
        packet_.WriteSymbol((byte >> 2) & 3);
        packet_.WriteSymbol((byte >> 0) & 3);
    }
};

TYPED_TEST_CASE(PacketTest, PacketSizes);

TYPED_TEST(PacketTest, valid)
{
    for (auto byte : this->data_)
    {
        ASSERT_FALSE(this->packet_.full());
        ASSERT_FALSE(this->packet_.valid());
        this->PushByte(byte);
    }

    for (uint32_t i = 0; i < 32; i += 8)
    {
        ASSERT_FALSE(this->packet_.full());
        ASSERT_FALSE(this->packet_.valid());
        this->PushByte(this->expected_crc_ >> i);
    }

    ASSERT_FALSE(this->packet_.full());
    ASSERT_FALSE(this->packet_.valid());
    this->PushByte(this->hamming_.parity());
    ASSERT_FALSE(this->packet_.full());
    ASSERT_FALSE(this->packet_.valid());
    this->PushByte(this->hamming_.parity() >> 8);

    ASSERT_TRUE(this->packet_.full());
    ASSERT_TRUE(this->packet_.valid());
    ASSERT_EQ(this->expected_crc_, this->packet_.calculated_crc());

    this->packet_.Reset();
    ASSERT_FALSE(this->packet_.full());
    ASSERT_FALSE(this->packet_.valid());
}

TYPED_TEST(PacketTest, Invalid)
{
    // Tamper with one byte
    this->data_[this->kPacketSize / 2] ^= 0xFF;

    for (uint32_t i = 0; i < this->kPacketSize; i++)
    {
        ASSERT_FALSE(this->packet_.full());
        ASSERT_FALSE(this->packet_.valid());
        this->PushByte(this->data_[i]);
    }

    for (uint32_t i = 0; i < 32; i += 8)
    {
        ASSERT_FALSE(this->packet_.full());
        ASSERT_FALSE(this->packet_.valid());
        this->PushByte(this->expected_crc_ >> i);
    }

    ASSERT_FALSE(this->packet_.full());
    ASSERT_FALSE(this->packet_.valid());
    this->PushByte(this->hamming_.parity());
    ASSERT_FALSE(this->packet_.full());
    ASSERT_FALSE(this->packet_.valid());
    this->PushByte(this->hamming_.parity() >> 8);

    ASSERT_TRUE(this->packet_.full());
    ASSERT_FALSE(this->packet_.valid());
    ASSERT_NE(this->expected_crc_, this->packet_.calculated_crc());

    this->packet_.Reset();
    ASSERT_FALSE(this->packet_.full());
    ASSERT_FALSE(this->packet_.valid());
}

TYPED_TEST(PacketTest, BlockFill)
{
    std::vector<uint8_t> bytes;

    for (uint32_t i = 0; i < kPacketsPerBlock; i++)
    {
        ASSERT_FALSE(this->block_.full());
        this->RandomizeData();
        this->packet_.Reset();

        for (auto byte : this->data_)
        {
            this->PushByte(byte);
            bytes.push_back(byte);
        }

        this->block_.AppendPacket(this->packet_);
    }

    ASSERT_TRUE(this->block_.full());

    for (uint32_t i = 0; i < this->kBlockSize / 4; i++)
    {
        uint32_t word = this->block_.data()[i];

        #if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
            word = __builtin_bswap32(word);
        #endif

        ASSERT_EQ(bytes[i * 4 + 0], (word >>  0) & 0xFF);
        ASSERT_EQ(bytes[i * 4 + 1], (word >>  8) & 0xFF);
        ASSERT_EQ(bytes[i * 4 + 2], (word >> 16) & 0xFF);
        ASSERT_EQ(bytes[i * 4 + 3], (word >> 24) & 0xFF);
    }

    this->block_.Clear();
    ASSERT_FALSE(this->block_.full());
}

}
