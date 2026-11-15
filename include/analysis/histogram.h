#ifndef STEGOSCAN_ANALYSIS_HISTOGRAM_H_
#define STEGOSCAN_ANALYSIS_HISTOGRAM_H_

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace stegoscan {

constexpr std::size_t kHistogramBins = 256;
constexpr std::size_t kMaxColorChannels = 3;

/**
 * @brief Intensity histogram for a single color channel (256 bins of 64-bit counts).
 */
using ChannelHistogram = std::array<std::uint64_t, kHistogramBins>;

/**
 * @brief Diagnostic work tracking information for an OpenMP worker thread.
 */
struct ThreadWorkInfo {
    int thread_id = 0;                  ///< OpenMP thread ID (omp_get_thread_num())
    std::uint64_t rows_processed = 0;   ///< Number of image rows processed by this thread
    std::uint32_t first_row = 0;        ///< Starting row index (inclusive; 0 if no rows assigned)
    std::uint32_t last_row = 0;         ///< Ending row index (inclusive; 0 if no rows assigned)
};

/**
 * @brief The shared histogram result contract produced by both sequential and parallel analyzers.
 */
struct HistogramResult {
    std::array<ChannelHistogram, kMaxColorChannels> channels{};
    std::uint8_t channel_count = 0;         ///< 1 for Grayscale, 3 for RGB
    std::uint64_t samples_per_channel = 0;  ///< Total samples counted per channel (width * height)
    int actual_threads = 1;                 ///< Number of threads in the team that produced the result
    std::vector<ThreadWorkInfo> thread_work;///< Diagnostic per-thread workload records
};

/**
 * @brief Compares two HistogramResult objects for mathematical equivalence.
 *
 * Checks:
 * - a.channel_count == b.channel_count
 * - a.samples_per_channel == b.samples_per_channel
 * - Every bin in every active channel matches exactly (a.channels[c][i] == b.channels[c][i])
 *
 * Intentionally ignores execution metadata: actual_threads and thread_work are not compared.
 *
 * @param a First histogram result.
 * @param b Second histogram result.
 * @return true if channel count, sample count, and all bins are identical; false otherwise.
 */
bool are_histograms_equal(const HistogramResult& a, const HistogramResult& b);

} // namespace stegoscan

#endif // STEGOSCAN_ANALYSIS_HISTOGRAM_H_
