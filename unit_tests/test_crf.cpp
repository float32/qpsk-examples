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
#include <vector>
#include <gtest/gtest.h>
#include "qpsk/inc/carrier_rejection_filter.h"

namespace qpsk::test::crf
{

using SymbolDurations = ::testing::Types<
    std::integral_constant<uint32_t, 6>,
    std::integral_constant<uint32_t, 8>,
    std::integral_constant<uint32_t, 12>,
    std::integral_constant<uint32_t, 16>>;

constexpr float kTestDuration = 10.f;
constexpr float kSampleRate = 48e3f;

template <typename T>
class CRFTest : public ::testing::Test
{
protected:
    static constexpr uint32_t kSymbolDuration = T::value;
    CarrierRejectionFilter<kSymbolDuration> crf_;

    void SetUp() override
    {
        crf_.Init();
    }
};

TYPED_TEST_CASE(CRFTest, SymbolDurations);

float MeasureSineLevel(std::vector<float>& signal)
{
    double sum = 0.f;

    for (uint32_t i = 0; i < signal.size(); i++)
    {
        sum += std::abs(signal[i]);
    }

    return sum * M_SQRT2 / signal.size();
}

TYPED_TEST(CRFTest, Attenuation)
{
    constexpr float kSymbolRate = kSampleRate / this->kSymbolDuration;
    // std::cout << "Symbol rate: " << kSymbolRate << std::endl;

    this->crf_.Init();
    std::vector<float> output;

    for (float t = 0.f; t < kTestDuration; t += 1.f / kSampleRate)
    {
        float input = std::sin(2.f * M_PI * t * kSymbolRate);
        output.push_back(this->crf_.Process(input));
    }

    float passband_level = MeasureSineLevel(output);
    float passband_gain = 20.f * std::log10(passband_level);
    // std::cout << "  Passband: " << passband_gain << std::endl;

    this->crf_.Init();
    output.clear();

    for (float t = 0.f; t < kTestDuration; t += 1.f / kSampleRate)
    {
        float input = std::sin(2.f * M_PI * t * 2.f * kSymbolRate);
        output.push_back(this->crf_.Process(input));
    }

    float stopband_level = MeasureSineLevel(output);
    float stopband_gain = 20.f * std::log10(stopband_level);
    // std::cout << "  Stopband: " << stopband_gain << std::endl;

    ASSERT_GE(passband_gain, -3.f);
    ASSERT_GE(passband_gain - stopband_gain, 12.f);
}

}
