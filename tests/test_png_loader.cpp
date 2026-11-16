#include "test_harness.h"
#include "image/image_loader.h"
#include "lodepng/lodepng.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <vector>

namespace {

// Helper RAII directory for creating and cleaning test fixture PNGs
struct FixtureDirectory {
    std::filesystem::path dir;
    explicit FixtureDirectory(const std::string& name) {
        dir = std::filesystem::temp_directory_path() / name;
        std::filesystem::create_directories(dir);
    }
    ~FixtureDirectory() {
        std::error_code ec;
        std::filesystem::remove_all(dir, ec);
    }
};

// Encodes a PNG buffer and saves to disk using LodePNG
void save_custom_png(const std::filesystem::path& path,
                     const std::vector<unsigned char>& pixels,
                     unsigned int w, unsigned int h,
                     LodePNGColorType png_color, unsigned int png_depth,
                     LodePNGColorType raw_color, unsigned int raw_depth,
                     const std::vector<std::vector<unsigned char>>& palette_entries = {}) {
    LodePNGState state;
    lodepng_state_init(&state);

    state.info_png.color.colortype = png_color;
    state.info_png.color.bitdepth = png_depth;
    state.info_raw.colortype = raw_color;
    state.info_raw.bitdepth = raw_depth;
    state.encoder.auto_convert = 0;

    for (const auto& pal : palette_entries) {
        lodepng_palette_add(&state.info_png.color, pal[0], pal[1], pal[2], pal[3]);
        lodepng_palette_add(&state.info_raw, pal[0], pal[1], pal[2], pal[3]);
    }

    unsigned char* png_buf = nullptr;
    size_t png_size = 0;
    unsigned int err = lodepng_encode(&png_buf, &png_size, pixels.data(), w, h, &state);
    lodepng_state_cleanup(&state);

    if (err != 0 || png_buf == nullptr) {
        if (png_buf) std::free(png_buf);
        throw std::runtime_error("Test fixture encoding failed: " + std::string(lodepng_error_text(err)));
    }

    std::ofstream ofs(path, std::ios::binary);
    ofs.write(reinterpret_cast<const char*>(png_buf), png_size);
    std::free(png_buf);
}

} // namespace

TEST_CASE(PngLoader, LoadTinyGrayscale8) {
    FixtureDirectory fixtures("stegoscan_test_gray8");
    auto path = fixtures.dir / "tiny_gray.png";

    // 2x2 Grayscale 8-bit image: [10, 20, 30, 40]
    std::vector<unsigned char> pixels = {10, 20, 30, 40};
    save_custom_png(path, pixels, 2, 2, LCT_GREY, 8, LCT_GREY, 8);

    stegoscan::DecodedImage img = stegoscan::load_png(path);
    ASSERT_EQ(img.width, 2);
    ASSERT_EQ(img.height, 2);
    ASSERT_EQ(img.channels, 1);
    ASSERT_EQ(img.pixels.size(), 4);
    ASSERT_EQ(img.pixels[0], 10);
    ASSERT_EQ(img.pixels[1], 20);
    ASSERT_EQ(img.pixels[2], 30);
    ASSERT_EQ(img.pixels[3], 40);
}

TEST_CASE(PngLoader, LoadTinyRgb8) {
    FixtureDirectory fixtures("stegoscan_test_rgb8");
    auto path = fixtures.dir / "tiny_rgb.png";

    // 2x2 RGB 8-bit image
    std::vector<unsigned char> pixels = {
        255, 0, 0,     0, 255, 0,
        0, 0, 255,     128, 128, 128
    };
    save_custom_png(path, pixels, 2, 2, LCT_RGB, 8, LCT_RGB, 8);

    stegoscan::DecodedImage img = stegoscan::load_png(path);
    ASSERT_EQ(img.width, 2);
    ASSERT_EQ(img.height, 2);
    ASSERT_EQ(img.channels, 3);
    ASSERT_EQ(img.pixels.size(), 12);

    // Pixel 0 (Red)
    ASSERT_EQ(img.pixels[0], 255);
    ASSERT_EQ(img.pixels[1], 0);
    ASSERT_EQ(img.pixels[2], 0);

    // Pixel 1 (Green)
    ASSERT_EQ(img.pixels[3], 0);
    ASSERT_EQ(img.pixels[4], 255);
    ASSERT_EQ(img.pixels[5], 0);

    // Pixel 2 (Blue)
    ASSERT_EQ(img.pixels[6], 0);
    ASSERT_EQ(img.pixels[7], 0);
    ASSERT_EQ(img.pixels[8], 255);

    // Pixel 3 (Gray)
    ASSERT_EQ(img.pixels[9], 128);
    ASSERT_EQ(img.pixels[10], 128);
    ASSERT_EQ(img.pixels[11], 128);
}

TEST_CASE(PngLoader, LoadTinyRgba8PreservesAlpha) {
    FixtureDirectory fixtures("stegoscan_test_rgba8");
    auto path = fixtures.dir / "tiny_rgba.png";

    // 2x2 RGBA 8-bit image with non-opaque alpha
    std::vector<unsigned char> pixels = {
        255, 0, 0, 100,      0, 255, 0, 150,
        0, 0, 255, 200,      128, 128, 128, 255
    };
    save_custom_png(path, pixels, 2, 2, LCT_RGBA, 8, LCT_RGBA, 8);

    stegoscan::DecodedImage img = stegoscan::load_png(path);
    ASSERT_EQ(img.width, 2);
    ASSERT_EQ(img.height, 2);
    ASSERT_EQ(img.channels, 4);
    ASSERT_EQ(img.pixels.size(), 16);

    // Verify RGBA preserved exactly
    ASSERT_EQ(img.pixels[3], 100);
    ASSERT_EQ(img.pixels[7], 150);
    ASSERT_EQ(img.pixels[11], 200);
    ASSERT_EQ(img.pixels[15], 255);
}

TEST_CASE(PngLoader, LoadTinyGrayscaleAlpha8PreservesAlpha) {
    FixtureDirectory fixtures("stegoscan_test_gray_alpha8");
    auto path = fixtures.dir / "tiny_ga.png";

    // 2x2 Grayscale + Alpha: (Y, A)
    std::vector<unsigned char> pixels = {
        10, 100,    20, 150,
        30, 200,    40, 255
    };
    save_custom_png(path, pixels, 2, 2, LCT_GREY_ALPHA, 8, LCT_GREY_ALPHA, 8);

    stegoscan::DecodedImage img = stegoscan::load_png(path);
    ASSERT_EQ(img.width, 2);
    ASSERT_EQ(img.height, 2);
    ASSERT_EQ(img.channels, 2);
    ASSERT_EQ(img.pixels.size(), 8);

    ASSERT_EQ(img.pixels[0], 10);
    ASSERT_EQ(img.pixels[1], 100);
    ASSERT_EQ(img.pixels[2], 20);
    ASSERT_EQ(img.pixels[3], 150);
}

TEST_CASE(PngLoader, LoadPaletteOpaqueExpandsToRgb) {
    FixtureDirectory fixtures("stegoscan_test_pal_opaque");
    auto path = fixtures.dir / "palette_opaque.png";

    std::vector<std::vector<unsigned char>> palette = {
        {255, 0, 0, 255},     // 0: Red
        {0, 255, 0, 255},     // 1: Green
        {0, 0, 255, 255},     // 2: Blue
        {255, 255, 0, 255}    // 3: Yellow
    };

    // 2x2 palette indices: 0, 1, 2, 3
    std::vector<unsigned char> indices = {0, 1, 2, 3};
    save_custom_png(path, indices, 2, 2, LCT_PALETTE, 8, LCT_PALETTE, 8, palette);

    stegoscan::DecodedImage img = stegoscan::load_png(path);
    ASSERT_EQ(img.width, 2);
    ASSERT_EQ(img.height, 2);
    ASSERT_EQ(img.channels, 3); // Must expand to RGB (3 channels) when no transparency
    ASSERT_EQ(img.pixels.size(), 12);

    // Pixel 0 is Red
    ASSERT_EQ(img.pixels[0], 255);
    ASSERT_EQ(img.pixels[1], 0);
    ASSERT_EQ(img.pixels[2], 0);

    // Pixel 1 is Green
    ASSERT_EQ(img.pixels[3], 0);
    ASSERT_EQ(img.pixels[4], 255);
    ASSERT_EQ(img.pixels[5], 0);

    // Pixel 2 is Blue
    ASSERT_EQ(img.pixels[6], 0);
    ASSERT_EQ(img.pixels[7], 0);
    ASSERT_EQ(img.pixels[8], 255);

    // Pixel 3 is Yellow
    ASSERT_EQ(img.pixels[9], 255);
    ASSERT_EQ(img.pixels[10], 255);
    ASSERT_EQ(img.pixels[11], 0);
}

TEST_CASE(PngLoader, LoadPaletteWithTransparencyExpandsToRgba) {
    FixtureDirectory fixtures("stegoscan_test_pal_trans");
    auto path = fixtures.dir / "palette_trans.png";

    // Palette with transparency metadata (alpha < 255 produces tRNS chunk)
    std::vector<std::vector<unsigned char>> palette = {
        {255, 0, 0, 50},      // 0: Red with alpha 50
        {0, 255, 0, 100},     // 1: Green with alpha 100
        {0, 0, 255, 150},     // 2: Blue with alpha 150
        {255, 255, 0, 255}    // 3: Yellow with alpha 255
    };

    std::vector<unsigned char> indices = {0, 1, 2, 3};
    save_custom_png(path, indices, 2, 2, LCT_PALETTE, 8, LCT_PALETTE, 8, palette);

    stegoscan::DecodedImage img = stegoscan::load_png(path);
    ASSERT_EQ(img.width, 2);
    ASSERT_EQ(img.height, 2);
    ASSERT_EQ(img.channels, 4); // Must expand to RGBA (4 channels) preserving transparency
    ASSERT_EQ(img.pixels.size(), 16);

    // Verify color and alpha preserved
    ASSERT_EQ(img.pixels[0], 255);
    ASSERT_EQ(img.pixels[3], 50);

    ASSERT_EQ(img.pixels[4], 0);
    ASSERT_EQ(img.pixels[5], 255);
    ASSERT_EQ(img.pixels[7], 100);

    ASSERT_EQ(img.pixels[8], 0);
    ASSERT_EQ(img.pixels[10], 255);
    ASSERT_EQ(img.pixels[11], 150);

    ASSERT_EQ(img.pixels[12], 255);
    ASSERT_EQ(img.pixels[13], 255);
    ASSERT_EQ(img.pixels[15], 255);
}

TEST_CASE(PngLoader, Reject16BitPng) {
    FixtureDirectory fixtures("stegoscan_test_16bit");
    auto path = fixtures.dir / "image_16bit.png";

    // 2x2 16-bit RGB PNG
    std::vector<unsigned char> pixels(2 * 2 * 3 * 2, 0);
    save_custom_png(path, pixels, 2, 2, LCT_RGB, 16, LCT_RGB, 16);

    // Must reject 16-bit PNGs rather than down-converting
    ASSERT_THROW(stegoscan::load_png(path), stegoscan::PngLoadError);
}

TEST_CASE(PngLoader, Reject4BitPalettePng) {
    FixtureDirectory fixtures("stegoscan_test_4bit_pal");
    auto path = fixtures.dir / "palette_4bit.png";

    std::vector<std::vector<unsigned char>> palette;
    for (int i = 0; i < 16; ++i) {
        palette.push_back({static_cast<unsigned char>(i * 16),
                           static_cast<unsigned char>(i * 16),
                           static_cast<unsigned char>(i * 16), 255});
    }
    // 2x2 4-bit indices (2 pixels per byte = 2 bytes)
    std::vector<unsigned char> indices = {0x01, 0x23};
    save_custom_png(path, indices, 2, 2, LCT_PALETTE, 4, LCT_PALETTE, 4, palette);

    // Must reject 4-bit palette PNG rather than expanding it
    ASSERT_THROW(stegoscan::load_png(path), stegoscan::PngLoadError);
}

TEST_CASE(PngLoader, Reject1BitGrayscalePng) {
    FixtureDirectory fixtures("stegoscan_test_1bit_gray");
    auto path = fixtures.dir / "gray_1bit.png";

    // 2x2 1-bit grayscale (packed bits)
    std::vector<unsigned char> pixels = {0xAA};
    save_custom_png(path, pixels, 2, 2, LCT_GREY, 1, LCT_GREY, 1);

    // Must reject 1-bit PNG
    ASSERT_THROW(stegoscan::load_png(path), stegoscan::PngLoadError);
}

TEST_CASE(PngLoader, RejectMissingFile) {
    ASSERT_THROW(stegoscan::load_png("test_images/non_existent_file_98765.png"),
                 stegoscan::PngLoadError);
}

TEST_CASE(PngLoader, RejectDirectoryAsPath) {
    ASSERT_THROW(stegoscan::load_png("test_images"),
                 stegoscan::PngLoadError);
}

TEST_CASE(PngLoader, RejectInvalidExtension) {
    FixtureDirectory fixtures("stegoscan_test_bad_ext");
    auto path = fixtures.dir / "test.jpg";

    std::ofstream ofs(path);
    ofs << "dummy content";
    ofs.close();

    ASSERT_THROW(stegoscan::load_png(path), stegoscan::PngLoadError);
}

TEST_CASE(PngLoader, RejectCorruptFileOrNonPngSignature) {
    FixtureDirectory fixtures("stegoscan_test_corrupt");
    auto path = fixtures.dir / "corrupted.png";

    std::ofstream ofs(path, std::ios::binary);
    // 64 bytes of pseudo-random data without PNG signature
    std::vector<char> garbage(64, 'X');
    ofs.write(garbage.data(), garbage.size());
    ofs.close();

    ASSERT_THROW(stegoscan::load_png(path), stegoscan::PngLoadError);
}
