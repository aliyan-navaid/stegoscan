#include "evaluation/evaluation.h"
#include <stdexcept>

namespace stegoscan {

double ConfusionMatrix::accuracy() const {
    const double total = static_cast<double>(true_positives + true_negatives + false_positives + false_negatives);
    if (total == 0.0) {
        return 0.0;
    }
    return static_cast<double>(true_positives + true_negatives) / total;
}

double ConfusionMatrix::false_positive_rate() const {
    const double denominator = static_cast<double>(false_positives + true_negatives);
    if (denominator == 0.0) {
        return 0.0;
    }
    return static_cast<double>(false_positives) / denominator;
}

double ConfusionMatrix::precision() const {
    const double denominator = static_cast<double>(true_positives + false_positives);
    if (denominator == 0.0) {
        return 0.0;
    }
    return static_cast<double>(true_positives) / denominator;
}

double ConfusionMatrix::recall() const {
    const double denominator = static_cast<double>(true_positives + false_negatives);
    if (denominator == 0.0) {
        return 0.0;
    }
    return static_cast<double>(true_positives) / denominator;
}

EvaluationReport evaluate_dataset(
    const std::filesystem::path& /*manifest_path*/,
    const EvaluationConfig& /*config*/) {
    // Placeholder stub implementation for interface scaffolding.
    // Functional implementation will be added in Module I (Evaluation) step.
    throw std::runtime_error("evaluate_dataset is not yet implemented (scaffolding stub)");
}

BenchmarkResult benchmark_analyzers(
    const Image& /*image*/,
    const BenchmarkConfig& /*config*/) {
    // Placeholder stub implementation for interface scaffolding.
    // Functional implementation will be added in Module I (Benchmarking) step.
    throw std::runtime_error("benchmark_analyzers is not yet implemented (scaffolding stub)");
}

} // namespace stegoscan
