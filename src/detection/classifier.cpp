#include "detection/classifier.h"

namespace stegoscan {

std::string detection_status_to_string(DetectionStatus status) {
    switch (status) {
        case DetectionStatus::PotentialLsbEvidence:
            return "PotentialLsbEvidence";
        case DetectionStatus::NoEvidenceDetected:
            return "NoEvidenceDetected";
        case DetectionStatus::InsufficientData:
            return "InsufficientData";
    }
    return "Unknown";
}

DetectionResult classify(
    const std::vector<ChiSquareResult>& /*channel_results*/,
    const ClassifierConfig& /*config*/) {
    // Placeholder stub implementation for interface scaffolding.
    // Functional implementation will be added in Module H step.
    throw std::runtime_error("classify is not yet implemented (scaffolding stub)");
}

} // namespace stegoscan
