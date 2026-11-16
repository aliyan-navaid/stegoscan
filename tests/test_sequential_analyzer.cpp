#include "test_harness.h"
#include "analysis/sequential_analyzer.h"

#include <numeric>

TEST_CASE(SequentialAnalyzer, TinyGrayscaleHandCalculated) {
    // 3x2 Grayscale image: 6 pixels
    stegoscan::Image img;
    img.width = 3;
    img.height = 2;
    img.channels = 1;
    img.pixels = {0, 5, 5, 10, 10, 10};

    stegoscan::HistogramResult res = stegoscan::compute_histogram_sequential(img);
    ASSERT_EQ(res.channel_count, 1);
    ASSERT_EQ(res.samples_per_channel, 6);
    ASSERT_EQ(res.actual_threads, 1);

    ASSERT_EQ(res.channels[0][0], 1ULL);
    ASSERT_EQ(res.channels[0][5], 2ULL);
    ASSERT_EQ(res.channels[0][10], 3ULL);
    ASSERT_EQ(res.channels[0][1], 0ULL);
    ASSERT_EQ(res.channels[0][255], 0ULL);

    // Sum of all 256 bins must equal width * height
    std::uint64_t total_bins = 0;
    for (std::size_t bin = 0; bin < stegoscan::kHistogramBins; ++bin) {
        total_bins += res.channels[0][bin];
    }
    ASSERT_EQ(total_bins, 6ULL);
}

TEST_CASE(SequentialAnalyzer, TinyRgbDifferentChannels) {
    // 2x2 RGB image: 4 pixels
    stegoscan::Image img;
    img.width = 2;
    img.height = 2;
    img.channels = 3;
    img.pixels = {
        10, 20, 30,     10, 20, 31,
        11, 20, 30,     11, 21, 30
    };

    stegoscan::HistogramResult res = stegoscan::compute_histogram_sequential(img);
    ASSERT_EQ(res.channel_count, 3);
    ASSERT_EQ(res.samples_per_channel, 4);

    // Channel 0 (R): two 10s, two 11s
    ASSERT_EQ(res.channels[0][10], 2ULL);
    ASSERT_EQ(res.channels[0][11], 2ULL);
    ASSERT_EQ(res.channels[0][20], 0ULL);

    // Channel 1 (G): three 20s, one 21
    ASSERT_EQ(res.channels[1][20], 3ULL);
    ASSERT_EQ(res.channels[1][21], 1ULL);
    ASSERT_EQ(res.channels[1][10], 0ULL);

    // Channel 2 (B): three 30s, one 31
    ASSERT_EQ(res.channels[2][30], 3ULL);
    ASSERT_EQ(res.channels[2][31], 1ULL);
    ASSERT_EQ(res.channels[2][0], 0ULL);

    for (std::size_t c = 0; c < 3; ++c) {
        std::uint64_t ch_sum = 0;
        for (std::size_t bin = 0; bin < stegoscan::kHistogramBins; ++bin) {
            ch_sum += res.channels[c][bin];
        }
        ASSERT_EQ(ch_sum, 4ULL);
    }
}

TEST_CASE(SequentialAnalyzer, OnlyZerosAnd255s) {
    // 4x4 Grayscale: 8 zeros, 8 255s
    stegoscan::Image img;
    img.width = 4;
    img.height = 4;
    img.channels = 1;
    img.pixels.resize(16);
    for (std::size_t i = 0; i < 16; ++i) {
        img.pixels[i] = (i % 2 == 0) ? 0 : 255;
    }

    stegoscan::HistogramResult res = stegoscan::compute_histogram_sequential(img);
    ASSERT_EQ(res.channels[0][0], 8ULL);
    ASSERT_EQ(res.channels[0][255], 8ULL);

    for (std::size_t bin = 1; bin < 255; ++bin) {
        ASSERT_EQ(res.channels[0][bin], 0ULL);
    }
}

TEST_CASE(SequentialAnalyzer, DeterministicPseudoRandomBinTotals) {
    // 32x32 RGB image = 1024 pixels per channel
    stegoscan::Image img;
    img.width = 32;
    img.height = 32;
    img.channels = 3;
    img.pixels.resize(32 * 32 * 3);

    // Deterministic pseudo-random generation
    std::uint32_t state = 42;
    for (std::size_t i = 0; i < img.pixels.size(); ++i) {
        state = state * 1664525u + 1013904223u;
        img.pixels[i] = static_cast<std::uint8_t>(state >> 24);
    }

    stegoscan::HistogramResult res1 = stegoscan::compute_histogram_sequential(img);
    stegoscan::HistogramResult res2 = stegoscan::compute_histogram_sequential(img);

    // Verification of determinism
    ASSERT_TRUE(stegoscan::are_histograms_equal(res1, res2));

    for (std::size_t c = 0; c < 3; ++c) {
        std::uint64_t sum = 0;
        for (std::size_t bin = 0; bin < stegoscan::kHistogramBins; ++bin) {
            sum += res1.channels[c][bin];
        }
        ASSERT_EQ(sum, 1024ULL);
    }
}

TEST_CASE(SequentialAnalyzer, DiagnosticThreadWorkRecorded) {
    stegoscan::Image img;
    img.width = 10;
    img.height = 8;
    img.channels = 1;
    img.pixels.resize(80, 50);

    stegoscan::HistogramResult res = stegoscan::compute_histogram_sequential(img);
    ASSERT_EQ(res.actual_threads, 1);
    ASSERT_EQ(res.thread_work.size(), 1);
    ASSERT_EQ(res.thread_work[0].thread_id, 0);
    ASSERT_EQ(res.thread_work[0].rows_processed, 8ULL);
    ASSERT_EQ(res.thread_work[0].first_row, 0U);
    ASSERT_EQ(res.thread_work[0].last_row, 7U);
}

TEST_CASE(SequentialAnalyzer, RejectInvalidImages) {
    stegoscan::Image zero_dim;
    zero_dim.width = 0;
    zero_dim.height = 4;
    zero_dim.channels = 1;
    ASSERT_THROW(stegoscan::compute_histogram_sequential(zero_dim), std::invalid_argument);

    stegoscan::Image bad_channels;
    bad_channels.width = 2;
    bad_channels.height = 2;
    bad_channels.channels = 4; // Not allowed in normalized Image
    bad_channels.pixels.resize(16, 0);
    ASSERT_THROW(stegoscan::compute_histogram_sequential(bad_channels), std::invalid_argument);

    stegoscan::Image size_mismatch;
    size_mismatch.width = 2;
    size_mismatch.height = 2;
    size_mismatch.channels = 1;
    size_mismatch.pixels.resize(3, 0); // Expected 4
    ASSERT_THROW(stegoscan::compute_histogram_sequential(size_mismatch), std::invalid_argument);
}
