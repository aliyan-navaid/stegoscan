#include "test_harness.h"
#include "analysis/sequential_analyzer.h"

TEST_CASE(SequentialAnalyzer, StubContractThrows) {
    stegoscan::Image img;
    img.width = 4;
    img.height = 4;
    img.channels = 1;
    img.pixels.resize(16, 50);

    // In scaffolding phase, compute_histogram_sequential stub throws runtime_error
    ASSERT_THROW(stegoscan::compute_histogram_sequential(img), std::runtime_error);
}
