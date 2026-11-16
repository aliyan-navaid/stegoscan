#include "analysis/parallel_analyzer.h"
#include <omp.h>

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace stegoscan {

namespace {

struct ThreadLocalHistogram {
    std::array<ChannelHistogram, kMaxColorChannels> channels{};
};

} // namespace

HistogramResult compute_histogram_parallel(const Image& image, int requested_threads) {
    // 1. Validate requested thread count and image invariants
    if (requested_threads < 1) {
        throw std::invalid_argument("requested_threads must be at least 1 (got " +
                                    std::to_string(requested_threads) + ")");
    }

    std::string val_err;
    if (!validate_image(image, &val_err)) {
        throw std::invalid_argument("Invalid Image passed to compute_histogram_parallel: " + val_err);
    }

    // 2. Allocate thread-local storage for requested thread team
    std::vector<ThreadLocalHistogram> local_histograms(requested_threads);
    std::vector<ThreadWorkInfo> work(requested_threads);
    for (int i = 0; i < requested_threads; ++i) {
        work[i].thread_id = i;
        work[i].rows_processed = 0;
        work[i].first_row = 0;
        work[i].last_row = 0;
    }

    int actual_threads = 1;
    const std::int64_t height = static_cast<std::int64_t>(image.height);
    const std::uint32_t width = image.width;
    const std::uint8_t channels = image.channels;

    // 3. OpenMP parallel region with static row distribution
    #pragma omp parallel num_threads(requested_threads) shared(actual_threads, local_histograms, work)
    {
        const int tid = omp_get_thread_num();

        #pragma omp single
        {
            actual_threads = omp_get_num_threads();
        }

        #pragma omp for schedule(static)
        for (std::int64_t y = 0; y < height; ++y) {
            if (work[tid].rows_processed == 0) {
                work[tid].first_row = static_cast<std::uint32_t>(y);
            }
            work[tid].last_row = static_cast<std::uint32_t>(y);
            work[tid].rows_processed++;

            const std::uint64_t row_pixel_offset = static_cast<std::uint64_t>(y) * width;

            if (channels == 1) {
                const std::uint8_t* row_ptr = &image.pixels[row_pixel_offset];
                for (std::uint32_t x = 0; x < width; ++x) {
                    const std::uint8_t val = row_ptr[x];
                    local_histograms[tid].channels[0][val]++;
                }
            } else { // channels == 3
                const std::uint8_t* row_ptr = &image.pixels[row_pixel_offset * 3];
                for (std::uint32_t x = 0; x < width; ++x) {
                    const std::uint8_t r = row_ptr[3 * x];
                    const std::uint8_t g = row_ptr[3 * x + 1];
                    const std::uint8_t b = row_ptr[3 * x + 2];

                    local_histograms[tid].channels[0][r]++;
                    local_histograms[tid].channels[1][g]++;
                    local_histograms[tid].channels[2][b]++;
                }
            }
        }
    }

    // 4. Merge thread-local histograms into final HistogramResult outside the parallel region
    HistogramResult result;
    result.channel_count = channels;
    result.samples_per_channel = static_cast<std::uint64_t>(width) * image.height;
    result.actual_threads = actual_threads;
    result.thread_work.resize(actual_threads);

    for (int t = 0; t < actual_threads; ++t) {
        result.thread_work[t] = work[t];
        for (std::size_t c = 0; c < channels; ++c) {
            for (std::size_t bin = 0; bin < kHistogramBins; ++bin) {
                result.channels[c][bin] += local_histograms[t].channels[c][bin];
            }
        }
    }

    return result;
}

} // namespace stegoscan
