#include "test_harness.h"
#include "image/image_loader.h"

TEST_CASE(PngLoader, StubContractThrows) {
    // In scaffolding phase, load_png stub throws PngLoadError indicating unimplemented
    ASSERT_THROW(stegoscan::load_png("test_images/clean/sample.png"), stegoscan::PngLoadError);
}
