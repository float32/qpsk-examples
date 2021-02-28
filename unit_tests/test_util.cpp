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
#include <gtest/gtest.h>
#include "inc/util.h"

namespace qpsk::test::util
{

TEST(UtilTest, Sine)
{
    // Our sine function has a period of 1.0, so its maximum absolute slope is
    // 2*pi. The quadrant table has effectively 64 elements (the value 1.0 in
    // the 65th position is merely an alias of the 0.0 in the first position),
    // providing an effective lookup table length of 256, and maximum phase
    // error of 1/256. Multiplying by the max slope gives us the max expected
    // error in the sine function. We bump it up a little to allow for floating
    // point roundoff error.

    constexpr double tolerance = 2 * M_PI / 256 * 1.001;
    double worst = 0;

    for (int32_t i = -100000; i < 100000; i++)
    {
        double x = 3.0 * i / 100000.0;

        double sin_expected = std::sin(x * 2 * M_PI);
        double sin_actual = Sine(x);
        double cos_expected = std::cos(x * 2 * M_PI);
        double cos_actual = Cosine(x);

        ASSERT_NEAR(sin_expected, sin_actual, tolerance)
            << "With x = " << x;

        ASSERT_NEAR(cos_expected, cos_actual, tolerance)
            << "With x = " << x;

        double error = std::abs(sin_actual - sin_expected);
        if (error > worst)
        {
            worst = error;
        }
    }

    // printf("Worst sine error: % .8e\n", worst);
}

TEST(UtilTest, ArcCot)
{
    for (int32_t i = -100000; i <= 100000; i++)
    {
        double x = i / 100000.0;
        double expected = M_PI_2 - std::atan(x);
        float actual = RestrictedArcCot(x);

        ASSERT_NEAR(expected, actual, 0.01)
            << "With x = " << x;
    }
}

TEST(UtilTest, ArcTan)
{
    for (int32_t i = -100000; i <= 100000; i++)
    {
        double x = i / 100000.0;
        double expected = std::atan(x);
        float actual = RestrictedArcTan(x);

        ASSERT_NEAR(expected, actual, 0.01)
            << "With x = " << x;
    }
}

TEST(UtilTest, VectorToAngle)
{
    for (int32_t i = -100000; i <= 100000; i++)
    {
        double expected = M_PI * (i / 100000.0);
        double x = std::cos(expected);
        double y = std::sin(expected);
        float actual = VectorToAngle(x, y);

        if (expected - actual > M_PI)
        {
            expected -= 2 * M_PI;
        }
        else if (actual - expected > M_PI)
        {
            expected += 2 * M_PI;
        }

        ASSERT_GE(actual, -M_PI);
        ASSERT_LE(actual, 2 * M_PI);
        ASSERT_NEAR(expected, actual, 0.01)
            << "With x = " << x << ", y = " << y;
    }
}

TEST(UtilTest, VectorToPhase)
{
    for (int32_t i = -100000; i <= 100000; i++)
    {
        double angle = M_PI * (i / 100000.0);
        double x = std::cos(angle);
        double y = std::sin(angle);
        double expected = std::atan2(y, x) / (2 * M_PI);
        float actual = VectorToPhase(x, y);

        if (expected - actual > 0.5)
        {
            expected -= 1.0;
        }
        else if (actual - expected > 0.5)
        {
            expected += 1.0;
        }

        ASSERT_GE(actual, 0.0);
        ASSERT_LT(actual, 1.0);
        ASSERT_NEAR(expected, actual, 0.01 / (2 * M_PI))
            << "With x = " << x << ", y = " << y;
    }
}

}
