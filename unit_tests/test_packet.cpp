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

#include <cstdint>
#include <cmath>
#include <gtest/gtest.h>
#include <zlib.h>
#include "inc/packet.h"

namespace qpsk::test::packet
{

constexpr uint32_t kPacketSize = 256;
constexpr uint32_t kPacketsPerBlock = 4;
constexpr uint32_t kCRCSeed = 0xDEADBEEF;

template <typename T>
void PushByte(T& packet, uint8_t byte)
{
    packet.WriteSymbol((byte >> 6) & 3);
    packet.WriteSymbol((byte >> 4) & 3);
    packet.WriteSymbol((byte >> 2) & 3);
    packet.WriteSymbol((byte >> 0) & 3);
}

TEST(PacketTest, Valid)
{
    uint8_t data[kPacketSize];
    for (uint32_t i = 0; i < kPacketSize; i++)
    {
        data[i] = i * i;
    }
    uint32_t expected_crc = crc32(kCRCSeed, data, kPacketSize);

    Packet<kPacketSize> packet;
    packet.Init(kCRCSeed);

    for (uint32_t i = 0; i < kPacketSize; i++)
    {
        ASSERT_FALSE(packet.Complete());
        PushByte(packet, data[i]);
    }

    ASSERT_FALSE(packet.Complete());
    PushByte(packet, expected_crc >> 24);
    PushByte(packet, expected_crc >> 16);
    PushByte(packet, expected_crc >> 8);
    PushByte(packet, expected_crc);

    ASSERT_TRUE(packet.Complete());
    ASSERT_TRUE(packet.Valid());
    ASSERT_EQ(expected_crc, packet.CalculatedCRC());

    packet.Reset();
    ASSERT_FALSE(packet.Complete());
}

TEST(PacketTest, Invalid)
{
    uint8_t data[kPacketSize];
    for (uint32_t i = 0; i < kPacketSize; i++)
    {
        data[i] = i * i;
    }
    uint32_t expected_crc = crc32(kCRCSeed, data, kPacketSize);

    Packet<kPacketSize> packet;
    packet.Init(kCRCSeed);

    // Tamper with one byte
    data[kPacketSize / 2] ^= 0xFF;

    for (uint32_t i = 0; i < kPacketSize; i++)
    {
        ASSERT_FALSE(packet.Complete());
        PushByte(packet, data[i]);
    }

    ASSERT_FALSE(packet.Complete());
    PushByte(packet, expected_crc >> 24);
    PushByte(packet, expected_crc >> 16);
    PushByte(packet, expected_crc >> 8);
    PushByte(packet, expected_crc);

    ASSERT_TRUE(packet.Complete());
    ASSERT_FALSE(packet.Valid());
    ASSERT_NE(expected_crc, packet.CalculatedCRC());
}

TEST(PacketTest, BlockFill)
{
    // The Block class does no validity checking so we can just fill it with
    // empty packets.
    Block<kPacketSize * kPacketsPerBlock> block;
    Packet<kPacketSize> packet;
    block.Init();
    packet.Init(kCRCSeed);

    for (uint32_t i = 0; i < kPacketsPerBlock; i++)
    {
        ASSERT_FALSE(block.Complete());
        block.AppendPacket(packet);
    }

    ASSERT_TRUE(block.Complete());

    block.Clear();
    ASSERT_FALSE(block.Complete());
}

}
