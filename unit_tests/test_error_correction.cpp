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
#include <random>
#include <gtest/gtest.h>
#include "inc/error_correction.h"
#include "test_error_correction.h"

namespace qpsk::test::error_correction
{

const int kTestLengths[] = { 1, 2, 3, 4, 10, 16, 50, 100, 256 };

class ErrorCorrectionTest : public ::testing::TestWithParam<int>
{
protected:
    int length_;
    std::vector<uint8_t> expected_;
    uint32_t parity_;
    HammingEncoder encoder_;
    HammingDecoder decoder_;

    void SetUp() override
    {
        length_ = GetParam();

        std::minstd_rand rng;
        std::uniform_int_distribution<uint8_t> dist(0);
        rng.seed(0);

        // Generate random bytes to act as the data
        for (int i = 0; i < length_; i++)
        {
            expected_.push_back(dist(rng));
        }

        // Calculate the parity bits
        parity_ = encoder_.Encode(expected_);
    }
};

TEST_P(ErrorCorrectionTest, DecodeNoError)
{
    auto data = expected_;
    decoder_.Init(parity_);
    decoder_.Process(data.data(), data.size());
    ASSERT_EQ(expected_, data);
}

TEST_P(ErrorCorrectionTest, DecodeWithDataError)
{
    auto bad_data = expected_;

    for (uint32_t i = 0; i < bad_data.size() * 8; i++)
    {
        bad_data[i / 8] ^= 1 << (i % 8);

        ASSERT_NE(expected_, bad_data);
        decoder_.Init(parity_);
        decoder_.Process(bad_data.data(), bad_data.size());
        ASSERT_EQ(expected_, bad_data);
    }
}

TEST_P(ErrorCorrectionTest, DecodeWithParityError)
{
    auto data = expected_;

    for (uint32_t i = 0; i < 32; i++)
    {
        uint32_t bad_parity = parity_ ^ (1 << i);
        decoder_.Init(bad_parity);
        decoder_.Process(data.data(), data.size());
        ASSERT_EQ(data, expected_);
    }
}

INSTANTIATE_TEST_CASE_P(Length, ErrorCorrectionTest,
    ::testing::ValuesIn(kTestLengths));

}
