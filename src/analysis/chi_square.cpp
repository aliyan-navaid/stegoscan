#include "analysis/chi_square.h"

#include <cmath>
#include <limits>
#include <stdexcept>

namespace stegoscan {

namespace {

// Series expansion for lower incomplete gamma P(a, x) = gamma(a, x) / Gamma(a)
double gamma_series(double a, double x) {
    constexpr int kMaxIter = 200;
    constexpr double kEps = 1e-15;

    double sum = 1.0 / a;
    double term = 1.0 / a;
    for (int n = 1; n < kMaxIter; ++n) {
        term *= x / (a + static_cast<double>(n));
        sum += term;
        if (std::abs(term) < std::abs(sum) * kEps) {
            break;
        }
    }
    const double log_factor = a * std::log(x) - x - std::lgamma(a);
    return std::exp(log_factor) * sum;
}

// Continued fraction expansion for upper incomplete gamma Q(a, x) = Gamma(a, x) / Gamma(a)
// via modified Lentz method
double gamma_cf(double a, double x) {
    constexpr int kMaxIter = 200;
    constexpr double kEps = 1e-15;
    constexpr double kTiny = 1e-30;

    double b = x + 1.0 - a;
    double c = 1.0 / kTiny;
    double d = 1.0 / b;
    double f = d;

    for (int i = 1; i <= kMaxIter; ++i) {
        const double an = -static_cast<double>(i) * (static_cast<double>(i) - a);
        b += 2.0;
        d = an * d + b;
        if (std::abs(d) < kTiny) d = kTiny;
        c = b + an / c;
        if (std::abs(c) < kTiny) c = kTiny;
        d = 1.0 / d;
        const double delta = d * c;
        f *= delta;
        if (std::abs(delta - 1.0) < kEps) {
            break;
        }
    }
    const double log_factor = a * std::log(x) - x - std::lgamma(a);
    return std::exp(log_factor) * f;
}

} // namespace

double regularized_upper_incomplete_gamma(double a, double x) {
    if (a <= 0.0) {
        throw std::invalid_argument("Parameter a in incomplete gamma must be positive (got " +
                                    std::to_string(a) + ")");
    }
    if (x < 0.0) {
        throw std::invalid_argument("Parameter x in incomplete gamma must be non-negative (got " +
                                    std::to_string(x) + ")");
    }
    if (x == 0.0) {
        return 1.0;
    }

    double q = 0.0;
    if (x < a + 1.0) {
        // Use series expansion for lower incomplete gamma P(a, x), then Q = 1 - P
        const double p = gamma_series(a, x);
        q = 1.0 - p;
    } else {
        // Use continued fraction directly for Q(a, x)
        q = gamma_cf(a, x);
    }

    // Clamp strictly within [0.0, 1.0] to prevent floating point inaccuracies
    if (q < 0.0) q = 0.0;
    if (q > 1.0) q = 1.0;
    return q;
}

ChiSquareResult compute_channel_chi_square(const ChannelHistogram& histogram) {
    double statistic = 0.0;
    std::uint32_t df = 0;

    for (std::size_t k = 0; k < kHistogramBins / 2; ++k) {
        const std::uint64_t a = histogram[2 * k];
        const std::uint64_t b = histogram[2 * k + 1];

        if (a + b > 0) {
            const double diff = static_cast<double>(a) - static_cast<double>(b);
            const double sum = static_cast<double>(a + b);
            statistic += (diff * diff) / sum;
            df++;
        }
    }

    ChiSquareResult result;
    result.statistic = statistic;
    result.degrees_of_freedom = df;

    if (df == 0) {
        result.p_value = 0.0;
        result.is_valid = false;
        result.note = "No non-empty pairs";
        return result;
    }

    result.is_valid = true;
    result.p_value = regularized_upper_incomplete_gamma(
        static_cast<double>(df) / 2.0, statistic / 2.0);
    return result;
}

std::vector<ChiSquareResult> compute_chi_square(const HistogramResult& histogram) {
    std::vector<ChiSquareResult> results;
    results.reserve(histogram.channel_count);
    for (std::uint8_t c = 0; c < histogram.channel_count; ++c) {
        results.push_back(compute_channel_chi_square(histogram.channels[c]));
    }
    return results;
}

} // namespace stegoscan
