#include "test_harness.h"
#include "image/png_writer.h"
#include "image/image_loader.h"

#include <filesystem>
#include <vector>

namespace {

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

} // namespace

TEST_CASE(PngWriter, RoundTripGrayscale) {
    FixtureDirectory fixtures("stegoscan_writer_gray_rt");
    auto path = fixtures.dir / "written_gray.png";

    // 4x4 Grayscale image with distinct ramp values
    stegoscan::Image original;
    original.width = 4;
    original.height = 4;
    original.channels = 1;
    original.pixels.resize(16);
    for (std::size_t i = 0; i < 16; ++i) {
        original.pixels[i] = static_cast<std::uint8_t>(i * 16);
    }

    // Write PNG
    ASSERT_NO_THROW(stegoscan::write_png(path, original));
    ASSERT_TRUE(std::filesystem::exists(path));
    ASSERT_TRUE(std::filesystem::file_size(path) > 0);

    // Read back using load_png
    stegoscan::DecodedImage loaded = stegoscan::load_png(path);
    ASSERT_EQ(loaded.width, original.width);
    ASSERT_EQ(loaded.height, original.height);
    ASSERT_EQ(loaded.channels, 1);
    ASSERT_EQ(loaded.pixels.size(), original.pixels.size());

    for (std::size_t i = 0; i < original.pixels.size(); ++i) {
        ASSERT_EQ(loaded.pixels[i], original.pixels[i]);
    }
}

TEST_CASE(PngWriter, RoundTripRgb) {
    FixtureDirectory fixtures("stegoscan_writer_rgb_rt");
    auto path = fixtures.dir / "written_rgb.png";

    // 3x3 RGB image with varied color values
    stegoscan::Image original;
    original.width = 3;
    original.height = 3;
    original.channels = 3;
    original.pixels.resize(3 * 3 * 3);
    for (std::size_t i = 0; i < original.pixels.size(); ++i) {
        original.pixels[i] = static_cast<std::uint8_t>((i * 37) % 256);
    }

    // Write PNG
    ASSERT_NO_THROW(stegoscan::write_png(path, original));
    ASSERT_TRUE(std::filesystem::exists(path));
    ASSERT_TRUE(std::filesystem::file_size(path) > 0);

    // Read back using load_png
    stegoscan::DecodedImage loaded = stegoscan::load_png(path);
    ASSERT_EQ(loaded.width, original.width);
    ASSERT_EQ(loaded.height, original.height);
    ASSERT_EQ(loaded.channels, 3);
    ASSERT_EQ(loaded.pixels.size(), original.pixels.size());

    for (std::size_t i = 0; i < original.pixels.size(); ++i) {
        ASSERT_EQ(loaded.pixels[i], original.pixels[i]);
    }
}

TEST_CASE(PngWriter, RejectInvalidBufferDimensions) {
    FixtureDirectory fixtures("stegoscan_writer_bad_dim");
    auto path = fixtures.dir / "bad_dim.png";

    stegoscan::Image img;
    img.width = 0; // Invalid zero dimension
    img.height = 10;
    img.channels = 1;
    img.pixels.resize(0);

    ASSERT_THROW(stegoscan::write_png(path, img), stegoscan::PngWriteError);
}

TEST_CASE(PngWriter, RejectInvalidBufferSizeMismatch) {
    FixtureDirectory fixtures("stegoscan_writer_size_mismatch");
    auto path = fixtures.dir / "size_mismatch.png";

    stegoscan::Image img;
    img.width = 4;
    img.height = 4;
    img.channels = 1;
    img.pixels.resize(10); // Expected 16

    ASSERT_THROW(stegoscan::write_png(path, img), stegoscan::PngWriteError);
}

TEST_CASE(PngWriter, RejectUnsupportedChannelsAlpha) {
    FixtureDirectory fixtures("stegoscan_writer_bad_channels");
    auto path = fixtures.dir / "bad_channels.png";

    stegoscan::Image img;
    img.width = 2;
    img.height = 2;
    img.channels = 4; // Normalized Image must not have 4 channels (no alpha)
    img.pixels.resize(16, 0);

    ASSERT_THROW(stegoscan::write_png(path, img), stegoscan::PngWriteError);
}

TEST_CASE(PngWriter, RejectNonexistentParentDirectory) {
    auto path = std::filesystem::path("/nonexistent_folder_abc_123/out.png");

    stegoscan::Image img;
    img.width = 2;
    img.height = 2;
    img.channels = 1;
    img.pixels.resize(4, 128);

    ASSERT_THROW(stegoscan::write_png(path, img), stegoscan::PngWriteError);
}

TEST_CASE(PngWriter, RejectInvalidExtension) {
    FixtureDirectory fixtures("stegoscan_writer_bad_ext");
    auto path = fixtures.dir / "out.jpg";

    stegoscan::Image img;
    img.width = 2;
    img.height = 2;
    img.channels = 1;
    img.pixels.resize(4, 128);

    ASSERT_THROW(stegoscan::write_png(path, img), stegoscan::PngWriteError);
}

TEST_CASE(PngWriter, RejectUnwritableOutputPath) {
    FixtureDirectory fixtures("stegoscan_writer_unwritable");
    auto readonly_dir = fixtures.dir / "readonly_subdir";
    std::filesystem::create_directories(readonly_dir);

    // Remove write permission from readonly_subdir
    std::filesystem::permissions(readonly_dir,
                                 std::filesystem::perms::owner_read | std::filesystem::perms::owner_exec,
                                 std::filesystem::perm_options::replace);

    auto path = readonly_dir / "unwritable.png";

    stegoscan::Image img;
    img.width = 2;
    img.height = 2;
    img.channels = 1;
    img.pixels.resize(4, 128);

    ASSERT_THROW(stegoscan::write_png(path, img), stegoscan::PngWriteError);

    // Restore permissions so cleanup succeeds
    std::filesystem::permissions(readonly_dir,
                                 std::filesystem::perms::owner_all,
                                 std::filesystem::perm_options::replace);
}
