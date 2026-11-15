#ifndef STEGOSCAN_ANALYSIS_PARALLEL_ANALYZER_H_
#define STEGOSCAN_ANALYSIS_PARALLEL_ANALYZER_H_

#include "image/image.h"
#include "analysis/histogram.h"
#include <stdexcept>
#include <string>

namespace stegoscan {

/**
 * @brief Computes 256-bin channel histograms in parallel using OpenMP.
 *
 * Contract:
 * - Validates normalized Image and requires requested_threads >= 1.
 * - Distributes image rows across OpenMP threads using #pragma omp for schedule(static).
 * - Each thread writes ONLY to its private local histogram and private work-tracking slot.
 * - Threads NEVER update a shared histogram, avoiding per-pixel critical sections,
 *   locks, or atomic operations.
 * - Thread-local histograms are merged into a single HistogramResult after the parallel region.
 * - Records actual OpenMP team size (actual_threads), which may differ from requested_threads.
 * - Collects diagnostic per-thread row counts (thread_work) in a race-free manner.
 * - Produces channel bins that are mathematically identical to compute_histogram_sequential(image).
 * - Handles edge cases gracefully: requested_threads == 1, odd dimensions, and cases where
 *   requested_threads > image.height (threads receive 0 rows).
 *
 * Note: <omp.h> is intentionally NOT included in this header; OpenMP dependencies
 * are kept strictly internal to the implementation file.
 *
 * @param image The normalized input image.
 * @param requested_threads Number of OpenMP threads requested (must be >= 1).
 * @return Completed HistogramResult with merged bins and thread work statistics.
 * @throws std::invalid_argument if image is invalid or requested_threads < 1.
 */
HistogramResult compute_histogram_parallel(const Image& image, int requested_threads);

} // namespace stegoscan

#endif // STEGOSCAN_ANALYSIS_PARALLEL_ANALYZER_H_
