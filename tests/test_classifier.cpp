#include "test_harness.h"
#include "detection/classifier.h"

TEST_CASE(Classifier, StatusStrings) {
    ASSERT_EQ(stegoscan::detection_status_to_string(stegoscan::DetectionStatus::PotentialLsbEvidence),
              "PotentialLsbEvidence");
    ASSERT_EQ(stegoscan::detection_status_to_string(stegoscan::DetectionStatus::NoEvidenceDetected),
              "NoEvidenceDetected");
    ASSERT_EQ(stegoscan::detection_status_to_string(stegoscan::DetectionStatus::InsufficientData),
              "InsufficientData");
}

TEST_CASE(Classifier, StubContractThrows) {
    std::vector<stegoscan::ChiSquareResult> results;
    stegoscan::ClassifierConfig config;

    // In scaffolding phase, classify stub throws runtime_error
    ASSERT_THROW(stegoscan::classify(results, config), std::runtime_error);
}
