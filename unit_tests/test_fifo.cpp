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
#include <type_traits>
#include <gtest/gtest.h>
#include "inc/fifo.h"

namespace qpsk::test
{

constexpr uint32_t kPaint = 0xDEADBEEF;

using FifoSizes = ::testing::Types<
    std::integral_constant<uint32_t, 128>,
    std::integral_constant<uint32_t, 37>,
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
    EXPECT_EQ(this->fifo_.Empty(), true);
    EXPECT_EQ(this->fifo_.Full(), false);
    EXPECT_EQ(this->fifo_.Available(), 0);
}

TYPED_TEST(FifoTest, FillAndFlush)
{
    for (uint32_t i = 0; i < this->kSize; i++)
    {
        ASSERT_FALSE(this->fifo_.Full());
        this->fifo_.Push(kPaint);
    }

    EXPECT_FALSE(this->fifo_.Empty());
    EXPECT_TRUE(this->fifo_.Full());
    EXPECT_EQ(this->fifo_.Available(), this->kSize);

    this->fifo_.Flush();

    EXPECT_TRUE(this->fifo_.Empty());
    EXPECT_FALSE(this->fifo_.Full());
    EXPECT_EQ(this->fifo_.Available(), 0);
}

TYPED_TEST(FifoTest, PushPeekPop)
{
    for (uint32_t i = 0; i < this->kSize; i++)
    {
        this->fifo_.Push(kPaint + i);
        ASSERT_EQ(this->fifo_.Available(), i + 1);
    }

    for (uint32_t i = 0; i < this->kSize; i++)
    {
        EXPECT_EQ(this->fifo_.Peek(), kPaint + i);
        EXPECT_EQ(this->fifo_.Pop(), kPaint + i);
        ASSERT_EQ(this->fifo_.Available(), this->kSize - i - 1);
    }

    EXPECT_TRUE(this->fifo_.Empty());
}

}
