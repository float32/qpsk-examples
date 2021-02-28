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
#include <vector>

namespace qpsk::test::error_correction
{

class HammingEncoder
{
protected:
    uint32_t parity_;
    uint32_t bit_num_;

public:
    HammingEncoder()
    {
        Init();
    }

    void Init(void)
    {
        parity_ = 0;
        bit_num_ = 1;
    }

    uint32_t parity(void)
    {
        return parity_;
    }

    uint32_t Encode(uint8_t* bytes, size_t length)
    {
        for (uint32_t i = 0; i < length * 8; i++)
        {
            while ((bit_num_ & (bit_num_ - 1)) == 0)
            {
                bit_num_++;
            }

            if (bytes[i / 8] & (1 << (i % 8)))
            {
                parity_ ^= bit_num_;
            }

            bit_num_++;
        }

        return parity_;
    }

    uint32_t Encode(uint8_t byte)
    {
        return Encode(&byte, 1);
    }

    uint32_t Encode(std::vector<uint8_t> bytes)
    {
        return Encode(bytes.data(), bytes.size());
    }
};

}
