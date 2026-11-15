#include "test_harness.h"
#include "image/png_writer.h"

TEST_CASE(PngWriter, StubContractThrows) {
    stegoscan::Image img;
    img.width = 4;
    img.height = 4;
    img.channels = 1;
    img.pixels.resize(16, 0);

    // In scaffolding phase, write_png stub throws PngWriteError indicating unimplemented
    ASSERT_THROW(stegoscan::write_png("test_images/clean/out.png", img), stegoscan::PngWriteError);
}
