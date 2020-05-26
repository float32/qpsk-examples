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
#include "inc/one_pole.h"

namespace qpsk::test::one_pole
{

TEST(OnePoleTest, LowpassDecayTime)
{
    constexpr float sample_rate = 48000.;
    constexpr float freq = 50.;

    OnePoleLowpass lpf;
    lpf.Init(freq / sample_rate);
    float tau = 1 / (2 * M_PI * freq);

    float worst = 0;

    for (uint32_t i = 1; i < sample_rate; i++)
    {
        lpf.Process(1.f);
        float time = i / sample_rate;
        float expected = 1 - std::exp(-time / tau);
        ASSERT_NEAR(expected, lpf.Output(), 1e-5) << "at t = " << time;

        float error = abs(expected - lpf.Output());
        worst = error > worst ? error : worst;
    }

    // printf("Worst error: %g\n", worst);
}

}
