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
#include "inc/delay_line.h"

namespace qpsk::test::delay_line
{

constexpr uint32_t kPaint = 0xDEADBEEF;

using DelayLineSizes = ::testing::Types<
    std::integral_constant<uint32_t, 32>,
    std::integral_constant<uint32_t, 29>,
    std::integral_constant<uint32_t, 2>,
    std::integral_constant<uint32_t, 1>>;

template <typename T>
class DelayLineTest : public ::testing::Test
{
protected:
    static constexpr uint32_t kSize = T::value;
    DelayLine<uint32_t, kSize> delay_;

    void SetUp() override
    {
        delay_.Init();
    }
};

TYPED_TEST_CASE(DelayLineTest, DelayLineSizes);

TYPED_TEST(DelayLineTest, Init)
{
    this->delay_.Init(0);

    for (uint32_t i = 0; i < this->kSize; i++)
    {
        ASSERT_EQ(this->delay_.Tap(i), 0);
    }

    this->delay_.Init(kPaint);

    for (uint32_t i = 0; i < this->kSize; i++)
    {
        ASSERT_EQ(this->delay_.Tap(i), kPaint);
    }
}

TYPED_TEST(DelayLineTest, Process)
{
    this->delay_.Init(kPaint);

    for (uint32_t i = 0; i < this->kSize * 1000; i++)
    {
        ASSERT_EQ(this->delay_.Process(i),
            (i < this->kSize) ? kPaint : (i - this->kSize));

        for (uint32_t j = 0; j < this->kSize; j++)
        {
            ASSERT_EQ(this->delay_.Tap(j), (i < j) ? kPaint : (i - j));
        }
    }
}

}
