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
#include <type_traits>
#include <thread>
#include <chrono>
#include <random>
#include <algorithm>
#include <gtest/gtest.h>
#include "inc/fifo.h"

namespace qpsk::test::fifo
{

constexpr uint32_t kPaint = 0xDEADBEEF;

using FifoSizes = ::testing::Types<
    std::integral_constant<uint32_t, 4096>,
    std::integral_constant<uint32_t, 16>,
    std::integral_constant<uint32_t, 2>,
    std::integral_constant<uint32_t, 1>>;

template <typename T>
class FifoTest : public ::testing::Test
{
protected:
    static constexpr uint32_t kSize = T::value;
    Fifo<uint32_t, kSize> fifo_;

    void SetUp() override
    {
        fifo_.Init();
    }
};

TYPED_TEST_CASE(FifoTest, FifoSizes);

TYPED_TEST(FifoTest, Init)
{
    EXPECT_EQ(this->fifo_.empty(), true);
    EXPECT_EQ(this->fifo_.full(), false);
    EXPECT_EQ(this->fifo_.available(), 0);
}

TYPED_TEST(FifoTest, EmptyPeekPop)
{
    uint32_t item;
    EXPECT_FALSE(this->fifo_.Peek(item));
    EXPECT_FALSE(this->fifo_.Pop(item));
}

TYPED_TEST(FifoTest, FullPush)
{
    for (uint32_t i = 0; i < this->kSize; i++)
    {
        ASSERT_FALSE(this->fifo_.full());
        ASSERT_TRUE(this->fifo_.Push(kPaint));
    }

    EXPECT_TRUE(this->fifo_.full());
    EXPECT_FALSE(this->fifo_.Push(kPaint));

    for (uint32_t i = 0; i < this->kSize; i++)
    {
        this->fifo_.Pop();
        ASSERT_FALSE(this->fifo_.full());
        ASSERT_TRUE(this->fifo_.Push(kPaint));
        ASSERT_TRUE(this->fifo_.full());
        ASSERT_FALSE(this->fifo_.Push(kPaint));
    }
}

TYPED_TEST(FifoTest, FillAndFlush)
{
    for (uint32_t i = 0; i < this->kSize; i++)
    {
        ASSERT_FALSE(this->fifo_.full());
        ASSERT_TRUE(this->fifo_.Push(kPaint));
    }

    EXPECT_FALSE(this->fifo_.empty());
    EXPECT_TRUE(this->fifo_.full());
    EXPECT_EQ(this->fifo_.available(), this->kSize);

    this->fifo_.Flush();

    EXPECT_TRUE(this->fifo_.empty());
    EXPECT_FALSE(this->fifo_.full());
    EXPECT_EQ(this->fifo_.available(), 0);
}

TYPED_TEST(FifoTest, PushPeekPop)
{
    for (uint32_t i = 0; i < this->kSize; i++)
    {
        ASSERT_TRUE(this->fifo_.Push(kPaint + i));
        ASSERT_EQ(this->fifo_.available(), i + 1);
    }

    for (uint32_t i = 0; i < this->kSize; i++)
    {
        uint32_t peeked;
        uint32_t popped;
        ASSERT_TRUE(this->fifo_.Peek(peeked));
        ASSERT_EQ(peeked, kPaint + i);
        ASSERT_TRUE(this->fifo_.Pop(popped));
        ASSERT_EQ(popped, kPaint + i);
        ASSERT_EQ(this->fifo_.available(), this->kSize - i - 1);
    }

    EXPECT_TRUE(this->fifo_.empty());
}

TEST(FifoTest, PushBuffer)
{
    constexpr uint32_t kSize = 1024;
    Fifo<uint32_t, kSize> fifo;
    fifo.Init();
    uint32_t buffer[16];
    std::minstd_rand rng;
    std::uniform_int_distribution<uint32_t> dist(1, 16);

    for (uint32_t i = 0; i < kSize; )
    {
        uint32_t buffer_length = std::min(dist(rng), kSize - i);

        for (uint32_t j = 0; j < buffer_length; j++)
        {
            buffer[j] = i++;
        }

        ASSERT_TRUE(fifo.Push(buffer, buffer_length));
    }

    EXPECT_TRUE(fifo.full());
    ASSERT_EQ(fifo.available(), kSize);

    for (uint32_t i = 0; i < kSize; i++)
    {
        uint32_t item;
        ASSERT_TRUE(fifo.Pop(item));
        ASSERT_EQ(item, i);
    }

    ASSERT_TRUE(fifo.empty());
}

template <typename T>
class FifoThreadTest : public FifoTest<T>
{
protected:
    static constexpr uint32_t kTestLength = 100000;
    std::thread producer_;

    void TearDown() override
    {
        producer_.join();
    }
};

TYPED_TEST_CASE(FifoThreadTest, FifoSizes);

TYPED_TEST(FifoThreadTest, ThreadSafeProducerBound)
{
    using namespace std::literals::chrono_literals;

    this->producer_ = std::thread([&](void)
    {
        std::this_thread::sleep_for(10ms);

        for (uint32_t i = 0; i < this->kTestLength; i++)
        {
            while (this->fifo_.Push(i) == false);
            std::this_thread::yield();
        }
    });

    for (uint32_t i = 0; i < this->kTestLength; i++)
    {
        uint32_t item;
        while (this->fifo_.Pop(item) == false);
        ASSERT_EQ(item, i);
    }

    ASSERT_TRUE(this->fifo_.empty());
}

TYPED_TEST(FifoThreadTest, ThreadSafeConsumerBound)
{
    using namespace std::literals::chrono_literals;

    this->producer_ = std::thread([&](void)
    {
        for (uint32_t i = 0; i < this->kTestLength; i++)
        {
            while (this->fifo_.Push(i) == false);
        }
    });

    std::this_thread::sleep_for(10ms);

    for (uint32_t i = 0; i < this->kTestLength; i++)
    {
        uint32_t item;
        while (this->fifo_.Pop(item) == false);
        ASSERT_EQ(item, i);
        std::this_thread::yield();
    }

    ASSERT_TRUE(this->fifo_.empty());
}

}
