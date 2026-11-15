#ifndef STEGOSCAN_ANALYSIS_CHI_SQUARE_H_
#define STEGOSCAN_ANALYSIS_CHI_SQUARE_H_

#include "analysis/histogram.h"
#include <cstdint>
#include <string>
#include <vector>

namespace stegoscan {

/**
 * @brief Statistical outcome of pairwise chi-square analysis on a single channel histogram.
 */
struct ChiSquareResult {
    double statistic = 0.0;              ///< Sum of pairwise chi-square contributions
    std::uint32_t degrees_of_freedom = 0;///< Number of non-empty pairs contributing (df)
    double p_value = 0.0;                ///< Upper-tail probability Q(df/2, statistic/2)
    bool is_valid = false;               ///< True if degrees_of_freedom > 0 and calculation succeeded
    std::string note;                    ///< Explanatory note (e.g. error or insufficient data message)
};

/**
 * @brief Computes regularized upper incomplete gamma function Q(a, x) = Gamma(a, x) / Gamma(a).
 *
 * Used to calculate the upper-tail probability for chi-square distribution with k degrees of freedom:
 * p_value = Q(k / 2.0, chi_square / 2.0)
 *
 * Must satisfy:
 * - Returns 1.0 when x <= 0 (and a > 0)
 * - Returns 0.0 when x -> infinity
 * - Known test vector: for a = 1.0 (df = 2) and x = 1.0 (statistic = 2), Q(1, 1) = exp(-1) ~= 0.36787944117
 *
 * @param a Parameter a (k / 2.0, must be > 0.0)
 * @param x Upper limit x (chi_square / 2.0, must be >= 0.0)
 * @return Probability in the range [0.0, 1.0].
 */
double regularized_upper_incomplete_gamma(double a, double x);

/**
 * @brief Computes pairwise chi-square test on a single 256-bin channel histogram.
 *
 * Algorithm:
 * - Pairs adjacent bins: (0,1), (2,3), ..., (254,255).
 * - For each pair (a, b):
 *   - If a + b > 0:
 *       contribution = (a - b)^2 / (a + b)
 *       statistic += contribution
 *       degrees_of_freedom += 1
 *   - If a + b == 0: pair is skipped (does not contribute to statistic or degrees of freedom).
 * - If degrees_of_freedom == 0:
 *   - Returns is_valid = false, statistic = 0.0, p_value = 0.0, note = "No non-empty pairs".
 * - Otherwise:
 *   - p_value = regularized_upper_incomplete_gamma(df / 2.0, statistic / 2.0)
 *   - is_valid = true.
 *
 * Statistical rationale:
 * LSB replacement steganography equalizes the frequencies of adjacent pairs of values (PoVs),
 * making (a - b)^2 / (a + b) unusually small and driving the upper-tail p-value close to 1.0.
 *
 * @param histogram The 256-bin histogram for a single channel.
 * @return ChiSquareResult containing statistic, degrees of freedom, and p-value.
 */
ChiSquareResult compute_channel_chi_square(const ChannelHistogram& histogram);

/**
 * @brief Computes pairwise chi-square tests for all active channels in a HistogramResult.
 *
 * @param histogram Complete histogram result from sequential or parallel analyzer.
 * @return Vector of ChiSquareResult (one per active channel: length 1 for Grayscale, 3 for RGB).
 */
std::vector<ChiSquareResult> compute_chi_square(const HistogramResult& histogram);

} // namespace stegoscan

#endif // STEGOSCAN_ANALYSIS_CHI_SQUARE_H_
