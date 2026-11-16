#include "analysis/sequential_analyzer.h"

#include <cstdint>
#include <stdexcept>
#include <string>

namespace stegoscan {

HistogramResult compute_histogram_sequential(const Image& image) {
    // 1. Validate the input normalized Image
    std::string val_err;
    if (!validate_image(image, &val_err)) {
        throw std::invalid_argument("Invalid Image passed to compute_histogram_sequential: " + val_err);
    }

    const std::uint64_t num_pixels =
        static_cast<std::uint64_t>(image.width) * image.height;

    HistogramResult result;
    result.channel_count = image.channels;
    result.samples_per_channel = num_pixels;
    result.actual_threads = 1;

    // Diagnostic thread work entry for sequential execution
    ThreadWorkInfo work;
    work.thread_id = 0;
    work.rows_processed = image.height;
    work.first_row = 0;
    work.last_row = image.height > 0 ? image.height - 1 : 0;
    result.thread_work.push_back(work);

    // 2. Accumulate histogram bins deterministically
    if (image.channels == 1) {
        // Grayscale image: single channel accumulation
        for (std::uint64_t i = 0; i < num_pixels; ++i) {
            const std::uint8_t pixel_val = image.pixels[i];
            result.channels[0][pixel_val]++;
        }
    } else {
        // RGB image: interleaved channels (R, G, B, R, G, B, ...)
        for (std::uint64_t i = 0; i < num_pixels; ++i) {
            const std::uint8_t r = image.pixels[3 * i];
            const std::uint8_t g = image.pixels[3 * i + 1];
            const std::uint8_t b = image.pixels[3 * i + 2];

            result.channels[0][r]++;
            result.channels[1][g]++;
            result.channels[2][b]++;
        }
    }

    return result;
}

} // namespace stegoscan
