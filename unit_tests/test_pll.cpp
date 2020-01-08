#include <gtest/gtest.h>
#include <cmath>
#include "inc/pll.h"

namespace qpsk::test
{

constexpr double kTestDuration = 5;
constexpr double kSampleRate = 48000;

static const double kCarrierFrequencies[] =
{
    0.125,
    0.05,
    0.3,
};

static const double kMismatchFactors[] =
{
    1.0,
    0.99999,
    1.00001,
    0.99,
    1.01,
    0.95,
    1.05,
};

class PLLTest : public ::testing::TestWithParam<std::tuple<double, double>>
{
protected:
    double freq_;
    PhaseLockedLoop pll_;

    void SetUp() override
    {
        double carrier;
        double mismatch;

        std::tie(carrier, mismatch) = GetParam();
        pll_.Init(carrier);

        freq_ = carrier * mismatch;
    }
};

double PhaseDifference(double a, double b)
{
    return fmod(a + 1.0 - b, 1.0);
}

TEST_P(PLLTest, Lock)
{
    for (int32_t j = 0; j < kTestDuration * kSampleRate; j++)
    {
        double t = j / kSampleRate;
        double input_phase = fmod(freq_ * j, 1.0);

        // Normally we would multiply the input signal by sin and cos of the
        // PLL phase and then lowpass to extract the DC component, but since
        // we already know the input signal's phase, we can calculate the DC
        // component directly by using trigonometric product-to-sum identities.
        double delta = PhaseDifference(pll_.Phase(), input_phase);
        double i = 0.5 * cos(-2 * M_PI * delta);
        double q = 0.5 * sin(-2 * M_PI * delta);
        double phase_error = q - i;

        double offset = PhaseDifference(pll_.Phase(), input_phase);

        if (t > 0.25)
        {
            ASSERT_NEAR(offset, 0.375, 0.001)
                << "j = " << j << ", t = " << t;
        }

        pll_.Process(phase_error / 8);
    }
}

INSTANTIATE_TEST_CASE_P(Freq, PLLTest, ::testing::Combine(
    ::testing::ValuesIn(kCarrierFrequencies),
    ::testing::ValuesIn(kMismatchFactors)
    ));

}
