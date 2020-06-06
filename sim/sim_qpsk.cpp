#include <algorithm>
#include <climits>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <cassert>
#include <string>
#include <vector>
#include <random>
#include <cstdio>
#include <cmath>

#include "sim/vcd-writer/vcd_writer.h"
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

Signal LoadAudio(std::string file_path)
{
    std::ifstream wav_file;
    wav_file.open(file_path, std::ios::in | std::ios::binary);
        assert(wav_file.good());
    wav_file.seekg(44);
    Signal signal;
    while (!wav_file.eof())
    {
        int16_t sample = (wav_file.get() & 0xFF);
        sample |= (wav_file.get() << 8);
        signal.push_back(sample / 32767.f);
    }
    wav_file.close();
    return signal;
}

Signal LoadAudio(int carrier_rate, int packet_size, int page_size)
{
    Signal signal;
    std::stringstream ss;
    ss << "python3 encoder.py -s 48000 -w 0.05 -t bin"
        << " -i unit_tests/data/data.bin -o -"
        << " -c " << carrier_rate
        << " -p " << packet_size
        << " -f " << page_size;
    std::string cmd = ss.str();
    auto wav_file = popen(cmd.c_str(), "r");
    fseek(wav_file, 44, SEEK_SET);
    while (!feof(wav_file))
    {
        int16_t sample = (fgetc(wav_file) & 0xFF);
        sample |= (fgetc(wav_file) << 8);
        signal.push_back(sample / 32767.f);
    }
    pclose(wav_file);
    return signal;
}

Signal Resample(Signal signal, double ratio)
{
    if (ratio != 1.0)
    {
        Signal resampled;
        uint32_t length = floor(signal.size() * ratio);

        for (uint32_t i = 0; i < length; i++)
        {
            double position = i / ratio;
            uint32_t x0 = floor(position);
            float y0 = signal[x0];
            float y1 = signal[x0 + 1];
            position = fmod(position, 1.0);
            resampled.push_back(y0 + position * (y1 - y0));
        }

        return resampled;
    }
    else
    {
        return signal;
    }
}

Signal Scale(Signal signal, float level)
{
    if (level != 1.f)
    {
        for (auto it = signal.begin(); it != signal.end(); it++)
        {
            *it *= level;
        }
    }

    return signal;
}

Signal AddNoise(Signal signal, float noise_level)
{
    if (noise_level != 0.f)
    {
        std::minstd_rand rng;
        std::uniform_real_distribution<float> dist(-1, 1);

        for (auto it = signal.begin(); it != signal.end(); it++)
        {
            float sample = *it;
            sample += noise_level * dist(rng);
            sample = (sample > 1) ? 1 : (sample < -1) ? -1 : sample;
            *it = sample;
        }
    }

    return signal;
}

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

void SimQPSK(std::string vcd_file)
{
    auto signal = LoadAudio(kSymbolRate, kPacketSize, kPageSize);
    // Resample, attenuate, and add noise
    signal = Resample(signal, 1.02f);
    signal = Scale(signal, 0.1f);
    signal = AddNoise(signal, 0.01f);

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

    // Begin decoding
    for (auto sample : signal)
    {
        qpsk.Push(sample);
        auto result = qpsk.Receive();

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
}

}
