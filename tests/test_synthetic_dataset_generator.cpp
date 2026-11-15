#include "test_harness.h"
#include "evaluation/synthetic_dataset_generator.h"

TEST_CASE(DatasetGenerator, StubContractThrows) {
    stegoscan::SyntheticDatasetConfig config;
    config.output_directory = "test_images/generated";

    // In scaffolding phase, generate_synthetic_dataset stub throws runtime_error
    ASSERT_THROW(stegoscan::generate_synthetic_dataset(config), std::runtime_error);
}
