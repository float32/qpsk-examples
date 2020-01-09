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
#include "inc/window.h"

namespace qpsk::test
{

template <typename T>
class WindowTest : public ::testing::Test
{
protected:
    T window_;

    void SetUp() override
    {
        window_.Init();
    }
};

using WindowTypes = ::testing::Types<
    Window<double, 1>,
    Window<double, 7>,
    Window<double, 8>,
    Window<double, 9>>;
TYPED_TEST_CASE(WindowTest, WindowTypes);

TYPED_TEST(WindowTest, All)
{
    int32_t window_length = this->window_.length();

    ASSERT_EQ(this->window_.Sum(), 0);
    ASSERT_EQ(this->window_.Average(), 0);

    double sum = 0;

    for (int32_t i = 0; i < 1000; i++)
    {
        this->window_.Write(i);
        sum += i;
        sum -= (i >= window_length) ? (i - window_length) : 0;

        for (int32_t j = 0; j < window_length; j++)
        {
            if (i < j)
            {
                ASSERT_EQ(this->window_[j], 0)
                    << "i = " << i << ", j = " << j;
            }
            else
            {
                ASSERT_EQ(this->window_[j], i - j)
                    << "i = " << i << ", j = " << j;
            }
        }

        ASSERT_DOUBLE_EQ(this->window_.Sum(), sum)
            << "i = " << i;
        ASSERT_DOUBLE_EQ(this->window_.Average(), sum / window_length)
            << "i = " << i;
    }
}

template <typename T>
class BayTest : public ::testing::Test
{
protected:
    T bay_;

    void SetUp() override
    {
        bay_.Init();
    }
};

using BayTypes = ::testing::Types<
    Bay<double, 1, 1>,
    Bay<double, 1, 4>,
    Bay<double, 7, 1>,
    Bay<double, 9, 1>,
    Bay<double, 8, 1>,
    Bay<double, 8, 4>>;
TYPED_TEST_CASE(BayTest, BayTypes);

TYPED_TEST(BayTest, All)
{
    int32_t bay_width = this->bay_.width();
    int32_t window_length = this->bay_.length();

    ASSERT_EQ(this->bay_.Sum(), 0);
    ASSERT_EQ(this->bay_.Average(), 0);

    double sum = 0;
    int32_t total_length = bay_width * window_length;

    for (int32_t i = 0; i < 1000; i++)
    {
        this->bay_.Write(i);
        sum += i;
        sum -= (i >= total_length) ? (i - total_length) : 0;

        for (int32_t k = 0; k < bay_width; k++)
        {
            for (int32_t j = 0; j < window_length; j++)
            {
                int32_t m = k * window_length + j;

                if (i < m)
                {
                    ASSERT_EQ(this->bay_[k][j], 0)
                        << "i = " << i << ", j = " << j;
                }
                else
                {
                    ASSERT_EQ(this->bay_[k][j], i - m)
                        << "i = " << i << ", j = " << j;
                }
            }
        }

        ASSERT_DOUBLE_EQ(this->bay_.Sum(), sum)
            << "i = " << i;
        ASSERT_DOUBLE_EQ(this->bay_.Average(), sum / total_length)
            << "i = " << i;
    }
}

}
