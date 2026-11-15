#include "test_harness.h"
#include "analysis/chi_square.h"

TEST_CASE(ChiSquare, StubContractThrows) {
    stegoscan::ChannelHistogram hist{};
    hist.fill(10);

    // In scaffolding phase, compute_channel_chi_square stub throws runtime_error
    ASSERT_THROW(stegoscan::compute_channel_chi_square(hist), std::runtime_error);
    ASSERT_THROW(stegoscan::regularized_upper_incomplete_gamma(1.0, 1.0), std::runtime_error);
}
