#include "test_harness.h"
#include "image/preprocessor.h"

TEST_CASE(Preprocessor, StubContractThrows) {
    stegoscan::DecodedImage decoded;
    decoded.width = 2;
    decoded.height = 2;
    decoded.channels = 3;
    decoded.pixels.resize(12, 100);

    // In scaffolding phase, preprocess_image stub throws PreprocessingError indicating unimplemented
    ASSERT_THROW(stegoscan::preprocess_image(decoded), stegoscan::PreprocessingError);
}
