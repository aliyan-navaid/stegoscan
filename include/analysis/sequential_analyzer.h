#ifndef STEGOSCAN_ANALYSIS_SEQUENTIAL_ANALYZER_H_
#define STEGOSCAN_ANALYSIS_SEQUENTIAL_ANALYZER_H_

#include "image/image.h"
#include "analysis/histogram.h"
#include <stdexcept>
#include <string>

namespace stegoscan {

/**
 * @brief Computes 256-bin channel histograms sequentially as the ground-truth baseline.
 *
 * Contract:
 * - Validates normalized Image (width > 0, height > 0, channels in {1, 3}, valid buffer size).
 * - Iterates through every row and pixel sequentially without OpenMP directives.
 * - Increments the intensity bin for each present channel.
 * - Every pixel sample is counted exactly once.
 * - The result is strictly deterministic.
 * - For each active channel, sum of all 256 bins == width * height.
 * - actual_threads is set to 1.
 * - thread_work contains a single entry (tid=0, rows=height, first_row=0, last_row=height-1).
 * - Throws std::invalid_argument if the input image is invalid.
 *
 * @param image The normalized input image.
 * @return Completed HistogramResult.
 * @throws std::invalid_argument if the image fails validation.
 */
HistogramResult compute_histogram_sequential(const Image& image);

} // namespace stegoscan

#endif // STEGOSCAN_ANALYSIS_SEQUENTIAL_ANALYZER_H_
