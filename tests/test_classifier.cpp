#include "test_harness.h"
#include "detection/classifier.h"

#include <limits>

TEST_CASE(Classifier, StatusStrings) {
    ASSERT_EQ(stegoscan::detection_status_to_string(stegoscan::DetectionStatus::PotentialLsbEvidence),
              "PotentialLsbEvidence");
    ASSERT_EQ(stegoscan::detection_status_to_string(stegoscan::DetectionStatus::NoEvidenceDetected),
              "NoEvidenceDetected");
    ASSERT_EQ(stegoscan::detection_status_to_string(stegoscan::DetectionStatus::InsufficientData),
              "InsufficientData");
}

TEST_CASE(Classifier, ExactThresholdBoundaryCondition) {
    // Channel p-value exactly equal to 0.95 must be flagged per `>=` rule
    stegoscan::ChiSquareResult cr;
    cr.is_valid = true;
    cr.statistic = 0.1;
    cr.degrees_of_freedom = 50;
    cr.p_value = 0.95;

    stegoscan::ClassifierConfig config;
    config.p_value_threshold = 0.95;

    stegoscan::DetectionResult res = stegoscan::classify({cr}, config);
    ASSERT_EQ(static_cast<int>(res.status),
              static_cast<int>(stegoscan::DetectionStatus::PotentialLsbEvidence));
    ASSERT_EQ(res.channels.size(), 1);
    ASSERT_TRUE(res.channels[0].is_suspicious);
}

TEST_CASE(Classifier, JustBelowThresholdNotFlagged) {
    // Channel p-value strictly below threshold is not flagged
    stegoscan::ChiSquareResult cr;
    cr.is_valid = true;
    cr.statistic = 0.1;
    cr.degrees_of_freedom = 50;
    cr.p_value = 0.949999;

    stegoscan::ClassifierConfig config;
    config.p_value_threshold = 0.95;

    stegoscan::DetectionResult res = stegoscan::classify({cr}, config);
    ASSERT_EQ(static_cast<int>(res.status),
              static_cast<int>(stegoscan::DetectionStatus::NoEvidenceDetected));
    ASSERT_EQ(res.channels.size(), 1);
    ASSERT_FALSE(res.channels[0].is_suspicious);
}

TEST_CASE(Classifier, OneFlaggedChannelFlagsOverallImage) {
    // In multi-channel image, any flagged channel flags the image
    stegoscan::ChiSquareResult c0{10.0, 50, 0.10, true, ""};
    stegoscan::ChiSquareResult c1{0.01, 50, 0.98, true, ""}; // Suspicious
    stegoscan::ChiSquareResult c2{5.0,  50, 0.40, true, ""};

    stegoscan::DetectionResult res = stegoscan::classify({c0, c1, c2});
    ASSERT_EQ(static_cast<int>(res.status),
              static_cast<int>(stegoscan::DetectionStatus::PotentialLsbEvidence));

    ASSERT_EQ(res.channels.size(), 3);
    ASSERT_FALSE(res.channels[0].is_suspicious);
    ASSERT_TRUE(res.channels[1].is_suspicious);
    ASSERT_FALSE(res.channels[2].is_suspicious);

    // Verify explanation references channel 1
    ASSERT_TRUE(res.explanation.find("1") != std::string::npos);
}

TEST_CASE(Classifier, NoFlaggedChannelsYieldsNoEvidenceDetected) {
    stegoscan::ChiSquareResult c0{10.0, 50, 0.20, true, ""};
    stegoscan::ChiSquareResult c1{5.0,  50, 0.50, true, ""};
    stegoscan::ChiSquareResult c2{3.0,  50, 0.80, true, ""};

    stegoscan::DetectionResult res = stegoscan::classify({c0, c1, c2});
    ASSERT_EQ(static_cast<int>(res.status),
              static_cast<int>(stegoscan::DetectionStatus::NoEvidenceDetected));

    // Explanation must not declare the image "clean"
    ASSERT_TRUE(res.explanation.find("clean") == std::string::npos);
}

TEST_CASE(Classifier, NoUsableChannelsYieldsInsufficientData) {
    // Empty vector of results
    stegoscan::DetectionResult res_empty = stegoscan::classify({});
    ASSERT_EQ(static_cast<int>(res_empty.status),
              static_cast<int>(stegoscan::DetectionStatus::InsufficientData));

    // Channels present but all marked invalid
    stegoscan::ChiSquareResult inv0{0.0, 0, 0.0, false, "empty"};
    stegoscan::ChiSquareResult inv1{0.0, 0, 0.0, false, "empty"};

    stegoscan::DetectionResult res_inv = stegoscan::classify({inv0, inv1});
    ASSERT_EQ(static_cast<int>(res_inv.status),
              static_cast<int>(stegoscan::DetectionStatus::InsufficientData));
}

TEST_CASE(Classifier, InvalidThresholdRejection) {
    stegoscan::ChiSquareResult cr{0.0, 10, 0.5, true, ""};

    stegoscan::ClassifierConfig cfg_neg;
    cfg_neg.p_value_threshold = -0.01;
    ASSERT_THROW(stegoscan::classify({cr}, cfg_neg), std::invalid_argument);

    stegoscan::ClassifierConfig cfg_high;
    cfg_high.p_value_threshold = 1.01;
    ASSERT_THROW(stegoscan::classify({cr}, cfg_high), std::invalid_argument);

    stegoscan::ClassifierConfig cfg_nan;
    cfg_nan.p_value_threshold = std::numeric_limits<double>::quiet_NaN();
    ASSERT_THROW(stegoscan::classify({cr}, cfg_nan), std::invalid_argument);
}
