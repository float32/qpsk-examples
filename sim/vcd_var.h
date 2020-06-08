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

public:
    VCDIntegerVar(VCDWriter& vcd, std::string scope, std::string name)
    {
        vcd_ = &vcd;
        var_ = vcd.register_var(scope, name, VariableType::integer, width);
    }

    template <typename T>
    void change(TimeStamp t, T value)
    {
        std::stringstream ss;

        for (uint32_t mask = 1 << (width - 1); mask > 0; mask >>= 1)
        {
            ss << ((value & mask) ? '1' : '0');
        }

        vcd_->change(var_, t, ss.str());
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
