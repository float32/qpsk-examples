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

#include <stdexcept>
#include <iostream>
#include <climits>
#include <cstdint>
#include <vector>
#include <cmath>

#include "sim/vcd-writer/vcd_writer.h"
#include "sim/vcd_var.h"
#include "unit_tests/util.h"
#include "decoder.h"

namespace qpsk::sim
{

using namespace vcd;

constexpr uint32_t kSampleRate = 48000;
constexpr uint32_t kSymbolRate = 6000;
constexpr uint32_t kPageSize = 1024;
constexpr uint32_t kPacketSize = 256;
constexpr uint32_t kCRCSeed = 0;
constexpr uint32_t kSamplesPerSymbol = kSampleRate / kSymbolRate;
constexpr uint8_t kFillByte = 0xFF;
constexpr float kFlashWriteTime = 0.025f;

using Signal = std::vector<float>;

inline void SimQPSK(std::string vcd_file, std::string bin_file)
{
    auto expected_data = test::util::LoadBinary(bin_file);
    decltype(expected_data) decoded_data;

    auto signal = test::util::LoadAudio<Signal>(bin_file,
        kSymbolRate, kPacketSize, kPageSize, kFlashWriteTime * 2);

    // Resample, attenuate, and add noise
    signal = test::util::Resample(signal, 1.02f);
    signal = test::util::Scale(signal, 0.1f);
    signal = test::util::AddNoise(signal, 0.01f);

    VCDWriter vcd{vcd_file,
        makeVCDHeader(TimeScale::ONE, TimeScaleUnit::us, utils::now())};

    // Decoder vars
    VCDIntegerVar<4> v_q_state(vcd, "top", "q.state");
    VCDFixedPointVar<4, 16> v_q_in(vcd, "top", "q.in");

    // Packet vars
    VCDIntegerVar<8> v_pkt_byte(vcd, "top.q", "pkt.byte");

    // Demodulator vars
    VCDIntegerVar<4> v_dm_state(vcd, "top.q", "dm.state");
    VCDIntegerVar<3> v_dm_symbol(vcd, "top.q", "dm.symbol");
    VCDIntegerVar<1> v_dm_early(vcd, "top.q", "dm.early");
    VCDIntegerVar<1> v_dm_late(vcd, "top.q", "dm.late");
    VCDIntegerVar<1> v_dm_decide(vcd, "top.q", "dm.decide");
    VCDFixedPointVar<4, 16> v_dm_power(vcd, "top.q", "dm.power");
    VCDFixedPointVar<2, 16> v_dm_dec_ph(vcd, "top.q", "dm.dec_ph");

    // PLL vars
    VCDFixedPointVar<2, 16> v_pll_phase(vcd, "top.q.dm", "pll.phase");
    VCDFixedPointVar<2, 16> v_pll_phase_error(vcd, "top.q.dm", "pll.ph_err");
    VCDFixedPointVar<2, 16> v_pll_phase_inc(vcd, "top.q.dm", "pll.ph_inc");
    VCDFixedPointVar<4, 16> v_pll_crfi_out(vcd, "top.q.dm", "crfi.out");
    VCDFixedPointVar<4, 16> v_pll_crfq_out(vcd, "top.q.dm", "crfq.out");

    // Correlator vars
    VCDFixedPointVar<8, 16> v_corr_out(vcd, "top.q.dm", "corr.out");

    Decoder<kSamplesPerSymbol, kPacketSize, kPageSize> qpsk;
    qpsk.Init(kCRCSeed);

    TimeStamp time = 0;
    int flash_write_delay = 0;

    // Begin decoding
    for (auto sample : signal)
    {
        qpsk.Push(sample);

        if (flash_write_delay == 0)
        {
            auto result = qpsk.Receive();
            if (result == RESULT_PAGE_COMPLETE)
            {
                uint32_t* page = qpsk.GetPage();
                for (uint32_t i = 0; i < kPageSize / 4; i++)
                {
                    decoded_data.push_back(page[i] >>  0);
                    decoded_data.push_back(page[i] >>  8);
                    decoded_data.push_back(page[i] >> 16);
                    decoded_data.push_back(page[i] >> 24);
                }
                flash_write_delay = kSampleRate * kFlashWriteTime;
            }
        }
        else
        {
            flash_write_delay--;
        }

        v_q_in.change(time, sample);
        v_q_state.change(time, qpsk.state());

        v_pkt_byte.change(time, qpsk.PacketByte());
        v_dm_state.change(time, qpsk.DemodulatorState());
        v_dm_symbol.change(time, qpsk.LastSymbol());
        v_dm_early.change(time, qpsk.Early());
        v_dm_late.change(time, qpsk.Late());
        v_dm_decide.change(time, qpsk.Decide());
        v_dm_power.change(time, qpsk.SignalPower());
        v_dm_dec_ph.change(time, qpsk.DecisionPhase());

        v_pll_phase.change(time, qpsk.PllPhase());
        v_pll_phase_error.change(time, qpsk.PllPhaseError());
        v_pll_phase_inc.change(time, qpsk.PllPhaseIncrement());
        v_pll_crfi_out.change(time, qpsk.RecoveredI());
        v_pll_crfq_out.change(time, qpsk.RecoveredQ());
        v_corr_out.change(time, qpsk.Correlation());

        time += 1000000 / kSampleRate;
    }

    if (decoded_data.size() > expected_data.size())
    {
        expected_data.resize(decoded_data.size(), kFillByte);
    }

    if (decoded_data == expected_data)
    {
        std::cout << "Success!" << std::endl;
    }
    else
    {
        std::cerr << "Failure!" << std::endl;
        throw std::runtime_error("Decoded incorrect data");
    }
}

}