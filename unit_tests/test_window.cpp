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

#include <gtest/gtest.h>
#include <cmath>
#include <random>
#include "qpsk/inc/window.h"

namespace qpsk::test::window
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
    Window<float, 1>,
    Window<float, 7>,
    Window<float, 8>,
    Window<float, 9>>;
TYPED_TEST_CASE(WindowTest, WindowTypes);

TYPED_TEST(WindowTest, All)
{
    int32_t window_length = this->window_.length();

    ASSERT_EQ(this->window_.sum(), 0);
    ASSERT_EQ(this->window_.average(), 0);

    float sum = 0;

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

        ASSERT_FLOAT_EQ(this->window_.sum(), sum)
            << "i = " << i;
        ASSERT_FLOAT_EQ(this->window_.average(), sum / window_length)
            << "i = " << i;
    }
}

TYPED_TEST(WindowTest, Drift)
{
    std::minstd_rand rng;
    rng.seed(1);
    std::uniform_real_distribution<float> dist(0, 1);

    ASSERT_EQ(this->window_.sum(), 0);

    for (int i = 0; i < 1000000; i++)
    {
        this->window_.Write(dist(rng));
    }

    float sum = 0;
    for (unsigned int i = 0; i < this->window_.length(); i++)
    {
        sum += this->window_[i];
    }

    ASSERT_FLOAT_EQ(this->window_.sum(), sum);
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
    Bay<float, 1, 1>,
    Bay<float, 1, 4>,
    Bay<float, 7, 1>,
    Bay<float, 9, 1>,
    Bay<float, 8, 1>,
    Bay<float, 8, 4>>;
TYPED_TEST_CASE(BayTest, BayTypes);

TYPED_TEST(BayTest, All)
{
    int32_t bay_width = this->bay_.width();
    int32_t window_length = this->bay_.length();

    ASSERT_EQ(this->bay_.sum(), 0);
    ASSERT_EQ(this->bay_.average(), 0);

    float sum = 0;
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

        ASSERT_FLOAT_EQ(this->bay_.sum(), sum)
            << "i = " << i;
        ASSERT_FLOAT_EQ(this->bay_.average(), sum / total_length)
            << "i = " << i;
    }
}

TYPED_TEST(BayTest, Drift)
{
    std::minstd_rand rng;
    rng.seed(1);
    std::uniform_real_distribution<float> dist(0, 1);

    ASSERT_EQ(this->bay_.sum(), 0);

    for (int i = 0; i < 1000000; i++)
    {
        this->bay_.Write(dist(rng));
    }

    float sum = 0;
    for (unsigned int k = 0; k < this->bay_.width(); k++)
    {
        for (unsigned int j = 0; j < this->bay_.length(); j++)
        {
            sum += this->bay_[k][j];
        }
    }

    ASSERT_FLOAT_EQ(this->bay_.sum(), sum);
}

}
