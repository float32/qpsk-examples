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

#pragma once

#include <cmath>
#include <cstring>
#include <string>
#include <sstream>
#include <algorithm>
#include "sim/vcd-writer/vcd_writer.h"

namespace qpsk::sim
{

using namespace vcd;

template <int width>
class VCDIntegerVar
{
protected:
    VCDWriter* vcd_;
    VarPtr var_;
    static constexpr int kNumBytes = std::ceil(width / 8.0);
    char str_[kNumBytes * 8 + 1];

    const char kByteString[256][9] =
    {
        /* [[[cog
        for row in range(0, 256, 8):
            for byte in range(row, row + 8):
                cog.out('"{:08b}",'.format(byte))
            cog.outl()
        ]]] */
        "00000000","00000001","00000010","00000011","00000100","00000101","00000110","00000111",
        "00001000","00001001","00001010","00001011","00001100","00001101","00001110","00001111",
        "00010000","00010001","00010010","00010011","00010100","00010101","00010110","00010111",
        "00011000","00011001","00011010","00011011","00011100","00011101","00011110","00011111",
        "00100000","00100001","00100010","00100011","00100100","00100101","00100110","00100111",
        "00101000","00101001","00101010","00101011","00101100","00101101","00101110","00101111",
        "00110000","00110001","00110010","00110011","00110100","00110101","00110110","00110111",
        "00111000","00111001","00111010","00111011","00111100","00111101","00111110","00111111",
        "01000000","01000001","01000010","01000011","01000100","01000101","01000110","01000111",
        "01001000","01001001","01001010","01001011","01001100","01001101","01001110","01001111",
        "01010000","01010001","01010010","01010011","01010100","01010101","01010110","01010111",
        "01011000","01011001","01011010","01011011","01011100","01011101","01011110","01011111",
        "01100000","01100001","01100010","01100011","01100100","01100101","01100110","01100111",
        "01101000","01101001","01101010","01101011","01101100","01101101","01101110","01101111",
        "01110000","01110001","01110010","01110011","01110100","01110101","01110110","01110111",
        "01111000","01111001","01111010","01111011","01111100","01111101","01111110","01111111",
        "10000000","10000001","10000010","10000011","10000100","10000101","10000110","10000111",
        "10001000","10001001","10001010","10001011","10001100","10001101","10001110","10001111",
        "10010000","10010001","10010010","10010011","10010100","10010101","10010110","10010111",
        "10011000","10011001","10011010","10011011","10011100","10011101","10011110","10011111",
        "10100000","10100001","10100010","10100011","10100100","10100101","10100110","10100111",
        "10101000","10101001","10101010","10101011","10101100","10101101","10101110","10101111",
        "10110000","10110001","10110010","10110011","10110100","10110101","10110110","10110111",
        "10111000","10111001","10111010","10111011","10111100","10111101","10111110","10111111",
        "11000000","11000001","11000010","11000011","11000100","11000101","11000110","11000111",
        "11001000","11001001","11001010","11001011","11001100","11001101","11001110","11001111",
        "11010000","11010001","11010010","11010011","11010100","11010101","11010110","11010111",
        "11011000","11011001","11011010","11011011","11011100","11011101","11011110","11011111",
        "11100000","11100001","11100010","11100011","11100100","11100101","11100110","11100111",
        "11101000","11101001","11101010","11101011","11101100","11101101","11101110","11101111",
        "11110000","11110001","11110010","11110011","11110100","11110101","11110110","11110111",
        "11111000","11111001","11111010","11111011","11111100","11111101","11111110","11111111",
        // [[[end]]]
    };

public:
    VCDIntegerVar(VCDWriter& vcd, std::string scope, std::string name)
    {
        vcd_ = &vcd;
        var_ = vcd.register_var(scope, name, VariableType::integer, width);
        str_[kNumBytes * 8] = '\0';
    }

    template <typename T>
    void change(TimeStamp t, T value)
    {
        for (int i = 0; i < kNumBytes; i++)
        {
            int pos = (kNumBytes - i - 1) * 8;
            unsigned int byte = value & 0xFF;
            std::memcpy(&str_[pos], kByteString[byte], 8);
            value >>= 8;
        }

        vcd_->change(var_, t, &str_[kNumBytes * 8 - width]);
    }
};

class VCDRealVar
{
protected:
    VCDWriter* vcd_;
    VarPtr var_;

public:
    VCDRealVar(VCDWriter& vcd, std::string scope, std::string name)
    {
        vcd_ = &vcd;
        var_ = vcd.register_var(scope, name, VariableType::real);
    }

    template <typename T>
    void change(TimeStamp t, T value)
    {
        std::stringstream ss;
        ss << value;
        vcd_->change(var_, t, ss.str());
    }
};

template <int i_width, int q_width>
class VCDFixedPointVar : public VCDIntegerVar<i_width + q_width>
{
protected:
    using super = VCDIntegerVar<i_width + q_width>;
    using super::VCDIntegerVar;

public:
    template <typename T>
    void change(TimeStamp t, T value)
    {
        value = std::clamp<T>(value, INT_MIN >> q_width, INT_MAX >> q_width);
        int fix = std::round(value * (1 << q_width));
        super::change(t, fix);
    }
};

}
