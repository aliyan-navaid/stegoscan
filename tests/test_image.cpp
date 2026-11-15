#include "test_harness.h"
#include "image/decoded_image.h"
#include "image/image.h"
#include "analysis/histogram.h"
#include <limits>

TEST_CASE(ImageTypes, ValidDecodedImage) {
    stegoscan::DecodedImage img;
    img.width = 10;
    img.height = 10;
    img.channels = 3;
    img.pixels.resize(300, 128);

    std::string err;
    ASSERT_TRUE(stegoscan::validate_decoded_image(img, &err));
    ASSERT_TRUE(err.empty());
}

TEST_CASE(ImageTypes, RejectZeroDimensionsDecoded) {
    stegoscan::DecodedImage img;
    img.width = 0;
    img.height = 10;
    img.channels = 1;
    img.pixels.resize(0);

    std::string err;
    ASSERT_FALSE(stegoscan::validate_decoded_image(img, &err));
    ASSERT_FALSE(err.empty());
}

TEST_CASE(ImageTypes, RejectInvalidChannelsDecoded) {
    stegoscan::DecodedImage img;
    img.width = 2;
    img.height = 2;
    img.channels = 5; // Invalid: must be 1, 2, 3, or 4
    img.pixels.resize(20);

    std::string err;
    ASSERT_FALSE(stegoscan::validate_decoded_image(img, &err));
    ASSERT_FALSE(err.empty());
}

TEST_CASE(ImageTypes, RejectBufferSizeMismatchDecoded) {
    stegoscan::DecodedImage img;
    img.width = 4;
    img.height = 4;
    img.channels = 1;
    img.pixels.resize(15); // Expected 16

    std::string err;
    ASSERT_FALSE(stegoscan::validate_decoded_image(img, &err));
    ASSERT_FALSE(err.empty());
}

TEST_CASE(ImageTypes, ValidNormalizedImage) {
    stegoscan::Image img;
    img.width = 8;
    img.height = 8;
    img.channels = 1;
    img.pixels.resize(64, 0);

    std::string err;
    ASSERT_TRUE(stegoscan::validate_image(img, &err));
    ASSERT_TRUE(err.empty());
}

TEST_CASE(ImageTypes, RejectNormalizedImageAlpha) {
    stegoscan::Image img;
    img.width = 8;
    img.height = 8;
    img.channels = 4; // Normalized Image can only have 1 (Gray) or 3 (RGB)
    img.pixels.resize(256, 0);

    std::string err;
    ASSERT_FALSE(stegoscan::validate_image(img, &err));
    ASSERT_FALSE(err.empty());
}

TEST_CASE(HistogramModel, InitialValuesZero) {
    stegoscan::HistogramResult hist;
    ASSERT_EQ(hist.channel_count, 0);
    ASSERT_EQ(hist.samples_per_channel, 0);
    for (std::size_t c = 0; c < stegoscan::kMaxColorChannels; ++c) {
        for (std::size_t bin = 0; bin < stegoscan::kHistogramBins; ++bin) {
            ASSERT_EQ(hist.channels[c][bin], 0ULL);
        }
    }
}

TEST_CASE(HistogramModel, HistogramEquality) {
    stegoscan::HistogramResult h1;
    h1.channel_count = 1;
    h1.samples_per_channel = 100;
    h1.channels[0][10] = 50;
    h1.channels[0][20] = 50;

    stegoscan::HistogramResult h2 = h1;
    ASSERT_TRUE(stegoscan::are_histograms_equal(h1, h2));

    // Mismatch in bin
    h2.channels[0][20] = 49;
    ASSERT_FALSE(stegoscan::are_histograms_equal(h1, h2));

    // Execution metadata difference should still be considered equal
    h2 = h1;
    h2.actual_threads = 4;
    h2.thread_work.push_back({0, 100, 0, 99});
    ASSERT_TRUE(stegoscan::are_histograms_equal(h1, h2));
}
