// MIT License
//
// Copyright 2013 Émilie Gillet
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

#pragma once

#include <cstdint>
#include "window.h"

namespace qpsk
{

class Correlator
{
protected:
    static inline const uint8_t kAlignmentSequence[] = {2, 1};
    static constexpr uint32_t kLength = sizeof(kAlignmentSequence);

    Window<float, 3> history_;
    uint32_t age_;
    float maximum_;
    float correlation_;
    float tilt_;

public:
    void Init(void)
    {
        Reset();
    }

    void Reset(void)
    {
        history_.Init();
        age_ = 0;
        maximum_ = 0.f;
        correlation_ = 0.f;
        tilt_ = 0.5f;
    }

    template <class bay>
    bool Process(bay& i_history, bay& q_history)
    {
        constexpr uint32_t kRipeAge = bay::length() * bay::width() / 2.f;
        constexpr float kPeakThreshold = bay::length() * bay::width() / 2.f;

        correlation_ = 0.f;

        if (++age_ >= kRipeAge)
        {
            for (uint32_t i = 0; i < kLength; i++)
            {
                uint8_t symbol = kAlignmentSequence[kLength - 1 - i];
                uint8_t expected_i = (symbol & 2);
                uint8_t expected_q = (symbol & 1);

                float i_sum = i_history[i].Sum();
                float q_sum = q_history[i].Sum();

                correlation_ += expected_i ? i_sum : -i_sum;
                correlation_ += expected_q ? q_sum : -q_sum;
            }
        }

        if (correlation_ < 0.f)
        {
            // Reset the peak detector at each valley in the detection function,
            // so that we can detect several consecutive peaks.
            maximum_ = 0.f;
        }
        else if (correlation_ > maximum_)
        {
            maximum_ = correlation_;
        }

        // Detect a local maximum in the output of the correlator.
        history_.Write(correlation_);

        bool peak = (history_[1] == maximum_) && history_[0] < maximum_ &&
            (maximum_ >= kPeakThreshold);

        if (peak)
        {
            // We can approximate the sub-sample position of the peak by
            // comparing the relative correlation of the samples before and
            // after the raw peak.
            float left = history_[1] - history_[2];
            float right = history_[1] - history_[0];
            tilt_ = 0.5f * (left - right) / (left + right);
        }

        uint32_t center = bay::length() / 2;
        uint8_t symbol = kAlignmentSequence[kLength - 1];

        bool i_correlated = (symbol & 2) ? (i_history[0][center] > 0) :
                                           (i_history[0][center] < 0);

        bool q_correlated = (symbol & 1) ? (q_history[0][center] > 0) :
                                           (q_history[0][center] < 0);

        return peak && i_correlated && q_correlated;
    }

    static constexpr uint32_t length(void)
    {
        return kLength;
    }

    float output(void)
    {
        return correlation_;
    }

    float tilt(void)
    {
        return tilt_;
    }
};

}
