#include "test_harness.h"
#include "image/preprocessor.h"

TEST_CASE(Preprocessor, GrayscalePreservedUnchanged) {
    stegoscan::DecodedImage decoded;
    decoded.width = 3;
    decoded.height = 2;
    decoded.channels = 1;
    decoded.pixels = {10, 20, 30, 40, 50, 60};

    stegoscan::Image normalized = stegoscan::preprocess_image(decoded);
    ASSERT_EQ(normalized.width, 3);
    ASSERT_EQ(normalized.height, 2);
    ASSERT_EQ(normalized.channels, 1);
    ASSERT_EQ(normalized.pixels.size(), 6);
    for (std::size_t i = 0; i < decoded.pixels.size(); ++i) {
        ASSERT_EQ(normalized.pixels[i], decoded.pixels[i]);
    }
}

TEST_CASE(Preprocessor, RgbPreservedUnchanged) {
    stegoscan::DecodedImage decoded;
    decoded.width = 2;
    decoded.height = 2;
    decoded.channels = 3;
    decoded.pixels = {
        255, 0, 0,     0, 255, 0,
        0, 0, 255,     100, 150, 200
    };

    stegoscan::Image normalized = stegoscan::preprocess_image(decoded);
    ASSERT_EQ(normalized.width, 2);
    ASSERT_EQ(normalized.height, 2);
    ASSERT_EQ(normalized.channels, 3);
    ASSERT_EQ(normalized.pixels.size(), 12);
    for (std::size_t i = 0; i < decoded.pixels.size(); ++i) {
        ASSERT_EQ(normalized.pixels[i], decoded.pixels[i]);
    }
}

TEST_CASE(Preprocessor, GrayscaleAlphaDiscardsAlphaPreservesGray) {
    stegoscan::DecodedImage decoded;
    decoded.width = 2;
    decoded.height = 2;
    decoded.channels = 2; // (Y, A)
    decoded.pixels = {
        15, 255,    35, 128,
        55, 64,     75, 0
    };

    stegoscan::Image normalized = stegoscan::preprocess_image(decoded);
    ASSERT_EQ(normalized.width, 2);
    ASSERT_EQ(normalized.height, 2);
    ASSERT_EQ(normalized.channels, 1); // Alpha stripped -> 1 channel
    ASSERT_EQ(normalized.pixels.size(), 4);

    ASSERT_EQ(normalized.pixels[0], 15);
    ASSERT_EQ(normalized.pixels[1], 35);
    ASSERT_EQ(normalized.pixels[2], 55);
    ASSERT_EQ(normalized.pixels[3], 75);
}

TEST_CASE(Preprocessor, RgbaDiscardsAlphaPreservesRgb) {
    stegoscan::DecodedImage decoded;
    decoded.width = 2;
    decoded.height = 2;
    decoded.channels = 4; // (R, G, B, A)
    decoded.pixels = {
        10, 20, 30, 99,       40, 50, 60, 199,
        70, 80, 90, 50,       110, 120, 130, 255
    };

    stegoscan::Image normalized = stegoscan::preprocess_image(decoded);
    ASSERT_EQ(normalized.width, 2);
    ASSERT_EQ(normalized.height, 2);
    ASSERT_EQ(normalized.channels, 3); // Alpha stripped -> 3 channels
    ASSERT_EQ(normalized.pixels.size(), 12);

    // Pixel 0
    ASSERT_EQ(normalized.pixels[0], 10);
    ASSERT_EQ(normalized.pixels[1], 20);
    ASSERT_EQ(normalized.pixels[2], 30);

    // Pixel 1
    ASSERT_EQ(normalized.pixels[3], 40);
    ASSERT_EQ(normalized.pixels[4], 50);
    ASSERT_EQ(normalized.pixels[5], 60);

    // Pixel 2
    ASSERT_EQ(normalized.pixels[6], 70);
    ASSERT_EQ(normalized.pixels[7], 80);
    ASSERT_EQ(normalized.pixels[8], 90);

    // Pixel 3
    ASSERT_EQ(normalized.pixels[9], 110);
    ASSERT_EQ(normalized.pixels[10], 120);
    ASSERT_EQ(normalized.pixels[11], 130);
}

TEST_CASE(Preprocessor, RejectZeroDimensions) {
    stegoscan::DecodedImage decoded;
    decoded.width = 0;
    decoded.height = 4;
    decoded.channels = 1;
    decoded.pixels.resize(0);

    ASSERT_THROW(stegoscan::preprocess_image(decoded), stegoscan::PreprocessingError);
}

TEST_CASE(Preprocessor, RejectUnsupportedChannels) {
    stegoscan::DecodedImage decoded;
    decoded.width = 2;
    decoded.height = 2;
    decoded.channels = 5; // Unsupported
    decoded.pixels.resize(20, 0);

    ASSERT_THROW(stegoscan::preprocess_image(decoded), stegoscan::PreprocessingError);
}

TEST_CASE(Preprocessor, RejectMismatchedBufferSize) {
    stegoscan::DecodedImage decoded;
    decoded.width = 2;
    decoded.height = 2;
    decoded.channels = 3;
    decoded.pixels.resize(11, 0); // Expected 12

    ASSERT_THROW(stegoscan::preprocess_image(decoded), stegoscan::PreprocessingError);
}
