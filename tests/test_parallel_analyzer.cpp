#include "test_harness.h"
#include "analysis/parallel_analyzer.h"

TEST_CASE(ParallelAnalyzer, StubContractThrows) {
    stegoscan::Image img;
    img.width = 4;
    img.height = 4;
    img.channels = 1;
    img.pixels.resize(16, 50);

    // In scaffolding phase, compute_histogram_parallel stub throws runtime_error
    ASSERT_THROW(stegoscan::compute_histogram_parallel(img, 2), std::runtime_error);
}
