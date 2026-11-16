#include "test_harness.h"
#include "analysis/chi_square.h"

#include <cmath>

TEST_CASE(ChiSquare, KnownGammaCases) {
    // df = 2, statistic = 2 -> a = 1.0, x = 1.0 -> Q(1, 1) = exp(-1)
    const double q_1_1 = stegoscan::regularized_upper_incomplete_gamma(1.0, 1.0);
    ASSERT_NEAR(q_1_1, std::exp(-1.0), 1e-10);

    // x = 0 must always return 1.0
    ASSERT_NEAR(stegoscan::regularized_upper_incomplete_gamma(0.5, 0.0), 1.0, 1e-12);
    ASSERT_NEAR(stegoscan::regularized_upper_incomplete_gamma(64.0, 0.0), 1.0, 1e-12);

    // a = 0.5, x = 0.5 -> erfc(sqrt(0.5))
    const double q_half = stegoscan::regularized_upper_incomplete_gamma(0.5, 0.5);
    ASSERT_NEAR(q_half, std::erfc(std::sqrt(0.5)), 1e-10);

    // a = 2.0, x = 3.0 -> 4 * exp(-3)
    const double q_2_3 = stegoscan::regularized_upper_incomplete_gamma(2.0, 3.0);
    ASSERT_NEAR(q_2_3, 4.0 * std::exp(-3.0), 1e-10);

    // Invalid parameters must throw invalid_argument
    ASSERT_THROW(stegoscan::regularized_upper_incomplete_gamma(0.0, 1.0), std::invalid_argument);
    ASSERT_THROW(stegoscan::regularized_upper_incomplete_gamma(-1.0, 1.0), std::invalid_argument);
    ASSERT_THROW(stegoscan::regularized_upper_incomplete_gamma(1.0, -0.5), std::invalid_argument);
}

TEST_CASE(ChiSquare, EqualCountsProduceZeroStatisticAndPValueOne) {
    stegoscan::ChannelHistogram hist{};
    // Set 4 pairs to equal counts
    hist[0] = 50; hist[1] = 50;
    hist[2] = 20; hist[3] = 20;
    hist[4] = 80; hist[5] = 80;
    hist[6] = 10; hist[7] = 10;

    stegoscan::ChiSquareResult res = stegoscan::compute_channel_chi_square(hist);
    ASSERT_TRUE(res.is_valid);
    ASSERT_EQ(res.degrees_of_freedom, 4U);
    ASSERT_NEAR(res.statistic, 0.0, 1e-12);
    ASSERT_NEAR(res.p_value, 1.0, 1e-12);
}

TEST_CASE(ChiSquare, SingleUnequalPairHandCalculated) {
    stegoscan::ChannelHistogram hist{};
    // Pair (0, 1): a = 20, b = 10
    hist[0] = 20;
    hist[1] = 10;

    // Expected statistic: (20 - 10)^2 / (20 + 10) = 100 / 30 = 3.333333333333...
    const double expected_stat = 100.0 / 30.0;

    stegoscan::ChiSquareResult res = stegoscan::compute_channel_chi_square(hist);
    ASSERT_TRUE(res.is_valid);
    ASSERT_EQ(res.degrees_of_freedom, 1U);
    ASSERT_NEAR(res.statistic, expected_stat, 1e-10);

    // p_value = Q(0.5, 3.3333333333 / 2.0)
    const double expected_p = stegoscan::regularized_upper_incomplete_gamma(0.5, expected_stat / 2.0);
    ASSERT_NEAR(res.p_value, expected_p, 1e-10);
    ASSERT_TRUE(res.p_value >= 0.0 && res.p_value <= 1.0);
}

TEST_CASE(ChiSquare, EmptyPairsSkippedAndDoNotIncreaseDf) {
    stegoscan::ChannelHistogram hist{};
    // Only two non-empty pairs out of 128
    hist[10] = 5; hist[11] = 5;
    hist[200] = 15; hist[201] = 25;

    stegoscan::ChiSquareResult res = stegoscan::compute_channel_chi_square(hist);
    ASSERT_TRUE(res.is_valid);
    ASSERT_EQ(res.degrees_of_freedom, 2U); // Exactly 2 degrees of freedom
    ASSERT_TRUE(res.statistic > 0.0);
}

TEST_CASE(ChiSquare, NoUsablePairsReturnsInvalidResult) {
    stegoscan::ChannelHistogram all_zeros{}; // All 256 bins are 0

    stegoscan::ChiSquareResult res = stegoscan::compute_channel_chi_square(all_zeros);
    ASSERT_FALSE(res.is_valid);
    ASSERT_EQ(res.degrees_of_freedom, 0U);
    ASSERT_EQ(res.statistic, 0.0);
    ASSERT_EQ(res.p_value, 0.0);
    ASSERT_FALSE(res.note.empty());
}

TEST_CASE(ChiSquare, MultiChannelIndependence) {
    stegoscan::HistogramResult hist_res;
    hist_res.channel_count = 3;
    hist_res.samples_per_channel = 200;

    // Channel 0: perfectly equal pairs (p = 1.0)
    hist_res.channels[0][0] = 100;
    hist_res.channels[0][1] = 100;

    // Channel 1: unequal pair (high statistic, low p)
    hist_res.channels[1][0] = 200;
    hist_res.channels[1][1] = 0;

    // Channel 2: empty (is_valid = false)
    // (all zero)

    std::vector<stegoscan::ChiSquareResult> results = stegoscan::compute_chi_square(hist_res);
    ASSERT_EQ(results.size(), 3);

    // Channel 0 check
    ASSERT_TRUE(results[0].is_valid);
    ASSERT_NEAR(results[0].p_value, 1.0, 1e-10);

    // Channel 1 check
    ASSERT_TRUE(results[1].is_valid);
    ASSERT_TRUE(results[1].p_value < 0.001);

    // Channel 2 check
    ASSERT_FALSE(results[2].is_valid);
    ASSERT_EQ(results[2].degrees_of_freedom, 0U);
}

TEST_CASE(ChiSquare, PValueAlwaysStrictlyInZeroOne) {
    // Test various synthetic combinations
    for (int delta = 0; delta <= 50; ++delta) {
        stegoscan::ChannelHistogram hist{};
        hist[0] = 100 + delta;
        hist[1] = 100 - delta;

        stegoscan::ChiSquareResult res = stegoscan::compute_channel_chi_square(hist);
        ASSERT_TRUE(res.is_valid);
        ASSERT_TRUE(res.p_value >= 0.0);
        ASSERT_TRUE(res.p_value <= 1.0);
    }
}
