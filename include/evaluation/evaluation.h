#ifndef STEGOSCAN_EVALUATION_EVALUATION_H_
#define STEGOSCAN_EVALUATION_EVALUATION_H_

#include "image/image.h"
#include "detection/classifier.h"
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace stegoscan {

/**
 * @brief Binary confusion matrix and derived classification metrics.
 */
struct ConfusionMatrix {
    std::uint64_t true_positives = 0;   ///< Stego image flagged as PotentialLsbEvidence
    std::uint64_t true_negatives = 0;   ///< Clean image classified as NoEvidenceDetected
    std::uint64_t false_positives = 0;  ///< Clean image flagged as PotentialLsbEvidence
    std::uint64_t false_negatives = 0;  ///< Stego image missed (NoEvidenceDetected or InsufficientData)

    /**
     * @brief Accuracy = (TP + TN) / (TP + TN + FP + FN)
     * Returns 0.0 if total evaluated images is zero.
     */
    double accuracy() const;

    /**
     * @brief False Positive Rate = FP / (FP + TN)
     * Returns 0.0 if denominator is zero.
     */
    double false_positive_rate() const;

    /**
     * @brief Precision = TP / (TP + FP)
     * Returns 0.0 if denominator is zero.
     */
    double precision() const;

    /**
     * @brief Recall (True Positive Rate) = TP / (TP + FN)
     * Returns 0.0 if denominator is zero.
     */
    double recall() const;
};

/**
 * @brief Configuration for evaluating a labeled dataset manifest.
 */
struct EvaluationConfig {
    double threshold = 0.95; ///< p-value threshold used for classification
};

/**
 * @brief Summary report resulting from dataset evaluation.
 */
struct EvaluationReport {
    ConfusionMatrix metrics;
    double threshold = 0.95;
    std::size_t total_images_evaluated = 0;
    std::size_t failed_images = 0;
    std::vector<std::string> error_messages;
};

/**
 * @brief Evaluates detector accuracy against a labeled dataset manifest CSV.
 *
 * Contract:
 * - Reads CSV manifest with columns: path,label[,...]
 * - Paths in the manifest are resolved relative to the directory containing manifest_path.
 * - For each image: loads, preprocesses, analyzes, computes chi-square, and classifies.
 * - Compares classification outcome against ground-truth label ("clean" vs "stego").
 * - Images that fail to load or process are recorded in failed_images and error_messages;
 *   they are NOT silently counted as predictions in the confusion matrix.
 * - Computes TP, TN, FP, FN, accuracy, FPR, precision, and recall safely without division by zero.
 *
 * @param manifest_path Path to the CSV manifest file.
 * @param config Evaluation configuration (specifying threshold).
 * @return EvaluationReport containing metrics and diagnostic error counts.
 * @throws std::runtime_error if manifest cannot be opened or parsed.
 */
EvaluationReport evaluate_dataset(
    const std::filesystem::path& manifest_path,
    const EvaluationConfig& config = EvaluationConfig{});

/**
 * @brief Measurement record for a single thread count configuration.
 */
struct BenchmarkRun {
    int requested_threads = 1;
    int actual_threads = 1;
    double elapsed_seconds = 0.0;
    double speedup = 0.0;    ///< sequential_time_seconds / elapsed_seconds
    double efficiency = 0.0; ///< speedup / actual_threads
};

/**
 * @brief Complete benchmark results comparing sequential and parallel analyzers.
 */
struct BenchmarkResult {
    double sequential_time_seconds = 0.0;
    std::vector<BenchmarkRun> parallel_runs;
};

/**
 * @brief Configuration for analyzer benchmarking.
 */
struct BenchmarkConfig {
    std::vector<int> thread_counts{1, 2, 4, 8};
    int warmup_repetitions = 2;
    int repetitions = 5;
};

/**
 * @brief Benchmarks sequential vs OpenMP parallel histogram construction.
 *
 * Contract:
 * - Benchmarks ONLY histogram calculation (excludes image loading and preprocessing).
 * - Verifies that sequential and parallel histograms are mathematically identical before
 *   recording timing results. Throws std::runtime_error if histograms mismatch.
 * - Performs warmup repetitions to prime cache/runtime.
 * - Computes the median execution time across repetitions.
 * - Computes Speedup = T_seq / T_par and Efficiency = Speedup / actual_threads.
 *
 * @param image Preprocessed normalized Image to benchmark.
 * @param config Thread counts and repetition parameters.
 * @return BenchmarkResult containing baseline timing and parallel scaling metrics.
 * @throws std::runtime_error if histogram equality fails or image is invalid.
 */
BenchmarkResult benchmark_analyzers(
    const Image& image,
    const BenchmarkConfig& config = BenchmarkConfig{});

} // namespace stegoscan

#endif // STEGOSCAN_EVALUATION_EVALUATION_H_
