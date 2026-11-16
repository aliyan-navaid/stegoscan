#include "test_harness.h"
#include "analysis/parallel_analyzer.h"
#include "analysis/sequential_analyzer.h"

#include <cstdint>
#include <vector>

namespace {

// Helper to generate a deterministic pseudo-random image
stegoscan::Image create_test_image(std::uint32_t w, std::uint32_t h, std::uint8_t channels, std::uint32_t seed) {
    stegoscan::Image img;
    img.width = w;
    img.height = h;
    img.channels = channels;
    img.pixels.resize(static_cast<std::size_t>(w) * h * channels);

    std::uint32_t state = seed;
    for (std::size_t i = 0; i < img.pixels.size(); ++i) {
        state = state * 1664525u + 1013904223u;
        img.pixels[i] = static_cast<std::uint8_t>(state >> 24);
    }
    return img;
}

} // namespace

TEST_CASE(ParallelAnalyzer, GrayscaleEqualityAcrossThreadCounts) {
    stegoscan::Image img = create_test_image(64, 48, 1, 12345);
    stegoscan::HistogramResult seq = stegoscan::compute_histogram_sequential(img);

    const std::vector<int> thread_counts = {1, 2, 4, 8};
    for (int t : thread_counts) {
        stegoscan::HistogramResult par = stegoscan::compute_histogram_parallel(img, t);

        // Bins must match sequentially computed ground truth bit-for-bit
        ASSERT_TRUE(stegoscan::are_histograms_equal(seq, par));
        ASSERT_TRUE(par.actual_threads >= 1);
        ASSERT_TRUE(par.actual_threads <= t);

        // Verify row counts across active threads sum exactly to image height
        std::uint64_t total_rows = 0;
        for (const auto& w : par.thread_work) {
            ASSERT_TRUE(w.rows_processed <= img.height);
            total_rows += w.rows_processed;
        }
        ASSERT_EQ(total_rows, static_cast<std::uint64_t>(img.height));
    }
}

TEST_CASE(ParallelAnalyzer, RgbEqualityAcrossThreadCounts) {
    stegoscan::Image img = create_test_image(64, 48, 3, 54321);
    stegoscan::HistogramResult seq = stegoscan::compute_histogram_sequential(img);

    const std::vector<int> thread_counts = {1, 2, 4, 8};
    for (int t : thread_counts) {
        stegoscan::HistogramResult par = stegoscan::compute_histogram_parallel(img, t);

        ASSERT_TRUE(stegoscan::are_histograms_equal(seq, par));

        std::uint64_t total_rows = 0;
        for (const auto& w : par.thread_work) {
            total_rows += w.rows_processed;
        }
        ASSERT_EQ(total_rows, static_cast<std::uint64_t>(img.height));
    }
}

TEST_CASE(ParallelAnalyzer, OddDimensions) {
    // Non-power-of-two and odd image dimensions
    stegoscan::Image img = create_test_image(37, 19, 3, 9999);
    stegoscan::HistogramResult seq = stegoscan::compute_histogram_sequential(img);

    stegoscan::HistogramResult par = stegoscan::compute_histogram_parallel(img, 4);
    ASSERT_TRUE(stegoscan::are_histograms_equal(seq, par));

    std::uint64_t total_rows = 0;
    for (const auto& w : par.thread_work) {
        total_rows += w.rows_processed;
    }
    ASSERT_EQ(total_rows, static_cast<std::uint64_t>(img.height));
}

TEST_CASE(ParallelAnalyzer, SinglePixelAndSingleRow) {
    // 1x1 image
    stegoscan::Image single_pixel;
    single_pixel.width = 1;
    single_pixel.height = 1;
    single_pixel.channels = 1;
    single_pixel.pixels = {200};

    stegoscan::HistogramResult par_1x1 = stegoscan::compute_histogram_parallel(single_pixel, 4);
    ASSERT_EQ(par_1x1.channels[0][200], 1ULL);
    ASSERT_EQ(par_1x1.samples_per_channel, 1ULL);

    // 100x1 single row image
    stegoscan::Image single_row = create_test_image(100, 1, 3, 777);
    stegoscan::HistogramResult seq_row = stegoscan::compute_histogram_sequential(single_row);
    stegoscan::HistogramResult par_row = stegoscan::compute_histogram_parallel(single_row, 4);
    ASSERT_TRUE(stegoscan::are_histograms_equal(seq_row, par_row));
}

TEST_CASE(ParallelAnalyzer, MoreThreadsThanRows) {
    // Image has height = 3, but 8 threads are requested
    stegoscan::Image img = create_test_image(20, 3, 1, 1111);
    stegoscan::HistogramResult seq = stegoscan::compute_histogram_sequential(img);

    stegoscan::HistogramResult par = stegoscan::compute_histogram_parallel(img, 8);
    ASSERT_TRUE(stegoscan::are_histograms_equal(seq, par));

    std::uint64_t total_rows = 0;
    std::size_t active_workers = 0;
    for (const auto& w : par.thread_work) {
        total_rows += w.rows_processed;
        if (w.rows_processed > 0) {
            active_workers++;
        }
    }
    ASSERT_EQ(total_rows, 3ULL);
    ASSERT_TRUE(active_workers <= 3);
}

TEST_CASE(ParallelAnalyzer, UniformAndAlternatingPixelValues) {
    // Uniform image (all pixels = 128)
    stegoscan::Image uniform_img;
    uniform_img.width = 16;
    uniform_img.height = 16;
    uniform_img.channels = 1;
    uniform_img.pixels.resize(256, 128);

    stegoscan::HistogramResult par_uni = stegoscan::compute_histogram_parallel(uniform_img, 4);
    ASSERT_EQ(par_uni.channels[0][128], 256ULL);
    ASSERT_EQ(par_uni.channels[0][0], 0ULL);

    // Alternating 0 and 255
    stegoscan::Image alt_img;
    alt_img.width = 10;
    alt_img.height = 10;
    alt_img.channels = 3;
    alt_img.pixels.resize(300);
    for (std::size_t i = 0; i < 300; ++i) {
        alt_img.pixels[i] = (i % 2 == 0) ? 0 : 255;
    }

    stegoscan::HistogramResult seq_alt = stegoscan::compute_histogram_sequential(alt_img);
    stegoscan::HistogramResult par_alt = stegoscan::compute_histogram_parallel(alt_img, 4);
    ASSERT_TRUE(stegoscan::are_histograms_equal(seq_alt, par_alt));
}

TEST_CASE(ParallelAnalyzer, RepeatedRunsExposingRaceConditions) {
    // Run repeated parallel executions on the same image to detect race conditions
    stegoscan::Image img = create_test_image(128, 128, 3, 8888);
    stegoscan::HistogramResult seq = stegoscan::compute_histogram_sequential(img);

    for (int run = 0; run < 10; ++run) {
        stegoscan::HistogramResult par = stegoscan::compute_histogram_parallel(img, 4);
        ASSERT_TRUE(stegoscan::are_histograms_equal(seq, par));
    }
}

TEST_CASE(ParallelAnalyzer, RejectInvalidParameters) {
    stegoscan::Image img = create_test_image(8, 8, 1, 123);

    // requested_threads < 1 must be rejected
    ASSERT_THROW(stegoscan::compute_histogram_parallel(img, 0), std::invalid_argument);
    ASSERT_THROW(stegoscan::compute_histogram_parallel(img, -2), std::invalid_argument);

    // Invalid image must be rejected
    stegoscan::Image bad_img;
    bad_img.width = 0;
    bad_img.height = 10;
    bad_img.channels = 1;
    ASSERT_THROW(stegoscan::compute_histogram_parallel(bad_img, 4), std::invalid_argument);
}
