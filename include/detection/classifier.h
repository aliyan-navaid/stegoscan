#ifndef STEGOSCAN_DETECTION_CLASSIFIER_H_
#define STEGOSCAN_DETECTION_CLASSIFIER_H_

#include "analysis/chi_square.h"
#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

namespace stegoscan {

/**
 * @brief Image-level classification assessment.
 *
 * Cautious terminology is enforced per AGENTS.md requirements:
 * - PotentialLsbEvidence: At least one valid channel has p_value >= p_value_threshold.
 * - NoEvidenceDetected: Usable channels analyzed and none exceeded threshold (must NOT claim "clean").
 * - InsufficientData: No valid channel results available to perform classification.
 */
enum class DetectionStatus {
    PotentialLsbEvidence,
    NoEvidenceDetected,
    InsufficientData
};

/**
 * @brief Configuration parameters for the classification decision rule.
 */
struct ClassifierConfig {
    double p_value_threshold = 0.95; ///< Threshold for flagging a channel (must be in [0.0, 1.0])
};

/**
 * @brief Detailed classification outcome for a specific channel.
 */
struct ChannelDetectionResult {
    std::size_t channel_index = 0;   ///< 0: Gray or R, 1: G, 2: B
    ChiSquareResult chi_square{};   ///< Underlying chi-square statistics
    bool is_suspicious = false;      ///< True if valid and p_value >= threshold
};

/**
 * @brief Overall image-level classification result with channel breakdowns.
 */
struct DetectionResult {
    DetectionStatus status = DetectionStatus::InsufficientData;
    double p_value_threshold = 0.95;
    std::vector<ChannelDetectionResult> channels;
    std::string explanation;
};

/**
 * @brief Converts per-channel chi-square results into an image-level assessment.
 *
 * Decision Rule (confirmed in AGENTS.md):
 * 1. Validate that config.p_value_threshold is within [0.0, 1.0]; throw std::invalid_argument otherwise.
 * 2. For each valid channel (is_valid == true):
 *    - If p_value >= p_value_threshold, flag channel as suspicious.
 * 3. Status determination:
 *    - If at least one valid channel is suspicious -> PotentialLsbEvidence.
 *    - Else if at least one valid channel exists -> NoEvidenceDetected.
 *    - Else (no usable channels) -> InsufficientData.
 * 4. Generate a human-readable explanation that includes which channel(s) triggered a flag.
 *
 * @param channel_results Vector of ChiSquareResult for the channels of an image.
 * @param config Classifier configuration specifying the p-value decision threshold.
 * @return Completed DetectionResult.
 * @throws std::invalid_argument if p_value_threshold is outside [0.0, 1.0].
 */
DetectionResult classify(
    const std::vector<ChiSquareResult>& channel_results,
    const ClassifierConfig& config = ClassifierConfig{});

/**
 * @brief Helper to convert a DetectionStatus enum to a display string.
 *
 * @param status The status enum value.
 * @return String representation: "PotentialLsbEvidence", "NoEvidenceDetected", or "InsufficientData".
 */
std::string detection_status_to_string(DetectionStatus status);

} // namespace stegoscan

#endif // STEGOSCAN_DETECTION_CLASSIFIER_H_
