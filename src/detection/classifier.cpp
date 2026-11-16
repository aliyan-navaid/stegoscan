#include "detection/classifier.h"

#include <cmath>
#include <sstream>
#include <stdexcept>

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
    const std::vector<ChiSquareResult>& channel_results,
    const ClassifierConfig& config) {
    // 1. Validate decision threshold in [0.0, 1.0]
    if (config.p_value_threshold < 0.0 || config.p_value_threshold > 1.0 ||
        std::isnan(config.p_value_threshold)) {
        throw std::invalid_argument("p_value_threshold must be between 0.0 and 1.0 (got " +
                                    std::to_string(config.p_value_threshold) + ")");
    }

    DetectionResult result;
    result.p_value_threshold = config.p_value_threshold;
    result.channels.reserve(channel_results.size());

    std::size_t valid_channels_count = 0;
    std::vector<std::size_t> suspicious_channels;

    // 2. Evaluate each channel independently
    for (std::size_t i = 0; i < channel_results.size(); ++i) {
        const auto& cr = channel_results[i];
        ChannelDetectionResult cdr;
        cdr.channel_index = i;
        cdr.chi_square = cr;

        if (cr.is_valid) {
            valid_channels_count++;
            // Flag channel as suspicious if p_value >= threshold
            if (cr.p_value >= config.p_value_threshold) {
                cdr.is_suspicious = true;
                suspicious_channels.push_back(i);
            } else {
                cdr.is_suspicious = false;
            }
        } else {
            cdr.is_suspicious = false;
        }

        result.channels.push_back(cdr);
    }

    // 3. Determine overall image-level classification status and cautious explanation
    if (!suspicious_channels.empty()) {
        result.status = DetectionStatus::PotentialLsbEvidence;
        std::ostringstream oss;
        oss << "Statistical indicator consistent with LSB replacement detected in channel(s): [";
        for (std::size_t k = 0; k < suspicious_channels.size(); ++k) {
            oss << suspicious_channels[k];
            if (k + 1 < suspicious_channels.size()) {
                oss << ", ";
            }
        }
        oss << "] with p-value >= " << config.p_value_threshold << ".";
        result.explanation = oss.str();
    } else if (valid_channels_count > 0) {
        result.status = DetectionStatus::NoEvidenceDetected;
        result.explanation = "No statistical signs of LSB replacement detected across analyzed channels (all p-values < threshold).";
    } else {
        result.status = DetectionStatus::InsufficientData;
        result.explanation = "Insufficient usable data across all channels to perform steganalysis.";
    }

    return result;
}

} // namespace stegoscan
