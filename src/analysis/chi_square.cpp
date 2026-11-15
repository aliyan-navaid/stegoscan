#include "analysis/chi_square.h"
#include <stdexcept>

namespace stegoscan {

double regularized_upper_incomplete_gamma(double /*a*/, double /*x*/) {
    // Placeholder stub implementation for interface scaffolding.
    // Functional implementation will be added in Module G step.
    throw std::runtime_error("regularized_upper_incomplete_gamma is not yet implemented (scaffolding stub)");
}

ChiSquareResult compute_channel_chi_square(const ChannelHistogram& /*histogram*/) {
    // Placeholder stub implementation for interface scaffolding.
    // Functional implementation will be added in Module G step.
    throw std::runtime_error("compute_channel_chi_square is not yet implemented (scaffolding stub)");
}

std::vector<ChiSquareResult> compute_chi_square(const HistogramResult& /*histogram*/) {
    // Placeholder stub implementation for interface scaffolding.
    // Functional implementation will be added in Module G step.
    throw std::runtime_error("compute_chi_square is not yet implemented (scaffolding stub)");
}

} // namespace stegoscan
