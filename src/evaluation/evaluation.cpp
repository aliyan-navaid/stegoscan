#include "evaluation/evaluation.h"

#include "analysis/chi_square.h"
#include "analysis/histogram.h"
#include "analysis/parallel_analyzer.h"
#include "analysis/sequential_analyzer.h"
#include "detection/classifier.h"
#include "image/image_loader.h"
#include "image/preprocessor.h"

// Only omp_get_wtime() is used here, as the timing source required by the project spec.
// No OpenMP parallelism lives in this module; the parallel work is in parallel_analyzer.cpp.
#include <omp.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace stegoscan {

// ---------------------------------------------------------------------------------------
// ConfusionMatrix
// Zero-denominator policy (documented in evaluation.h): every metric returns 0.0.
// ---------------------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------------------
// Manifest parsing
// ---------------------------------------------------------------------------------------

namespace {

std::string trim(const std::string& s) {
    std::size_t b = 0;
    std::size_t e = s.size();
    while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
    return s.substr(b, e - b);
}

std::string to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

// Splits one CSV record. Supports double-quoted fields with "" escapes (RFC 4180 style).
// Throws std::runtime_error on an unterminated quote.
std::vector<std::string> split_csv_line(const std::string& line) {
    std::vector<std::string> fields;
    std::string cur;
    bool in_quotes = false;
    for (std::size_t i = 0; i < line.size(); ++i) {
        const char ch = line[i];
        if (in_quotes) {
            if (ch == '"') {
                if (i + 1 < line.size() && line[i + 1] == '"') {
                    cur += '"';
                    ++i;
                } else {
                    in_quotes = false;
                }
            } else {
                cur += ch;
            }
        } else if (ch == '"') {
            in_quotes = true;
        } else if (ch == ',') {
            fields.push_back(cur);
            cur.clear();
        } else {
            cur += ch;
        }
    }
    if (in_quotes) {
        throw std::runtime_error("unterminated quoted field");
    }
    fields.push_back(cur);
    return fields;
}

enum class Label { Clean, Stego };

bool parse_label(const std::string& text, Label* out) {
    const std::string l = to_lower(trim(text));
    if (l == "clean") {
        *out = Label::Clean;
        return true;
    }
    if (l == "stego") {
        *out = Label::Stego;
        return true;
    }
    return false;
}

} // namespace

// ---------------------------------------------------------------------------------------
// Dataset evaluation
// ---------------------------------------------------------------------------------------

EvaluationReport evaluate_dataset(
    const std::filesystem::path& manifest_path,
    const EvaluationConfig& config) {
    if (!std::isfinite(config.threshold) || config.threshold < 0.0 || config.threshold > 1.0) {
        throw std::invalid_argument("evaluate_dataset: threshold must be within [0, 1]");
    }

    std::ifstream in(manifest_path, std::ios::binary);
    if (!in) {
        throw std::runtime_error("Cannot open manifest: " + manifest_path.string());
    }

    // Header: locate the required columns by name so extra columns (source_cover, ...) are fine.
    std::string line;
    bool have_header = false;
    std::size_t path_col = 0;
    std::size_t label_col = 0;
    std::size_t line_no = 0;
    while (std::getline(in, line)) {
        ++line_no;
        if (line_no == 1 && line.size() >= 3 && line.compare(0, 3, "\xEF\xBB\xBF") == 0) {
            line.erase(0, 3); // UTF-8 BOM
        }
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (trim(line).empty()) continue;

        std::vector<std::string> cols;
        try {
            cols = split_csv_line(line);
        } catch (const std::exception& ex) {
            throw std::runtime_error("Manifest " + manifest_path.string() + ": bad header: " + ex.what());
        }
        bool found_path = false;
        bool found_label = false;
        for (std::size_t i = 0; i < cols.size(); ++i) {
            const std::string name = to_lower(trim(cols[i]));
            if (name == "path") {
                path_col = i;
                found_path = true;
            } else if (name == "label") {
                label_col = i;
                found_label = true;
            }
        }
        if (!found_path || !found_label) {
            throw std::runtime_error("Manifest " + manifest_path.string() +
                                     ": header must contain 'path' and 'label' columns");
        }
        have_header = true;
        break;
    }
    if (!have_header) {
        throw std::runtime_error("Manifest " + manifest_path.string() + " is empty (no header row)");
    }

    EvaluationReport report;
    report.threshold = config.threshold;

    ClassifierConfig classifier_config;
    classifier_config.p_value_threshold = config.threshold;

    const std::filesystem::path base_dir = manifest_path.parent_path();
    const std::size_t min_columns = std::max(path_col, label_col) + 1;

    auto record_failure = [&report](const std::string& message) {
        ++report.failed_images;
        report.error_messages.push_back(message);
    };

    while (std::getline(in, line)) {
        ++line_no;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (trim(line).empty()) continue;

        const std::string where = "line " + std::to_string(line_no);

        std::vector<std::string> cols;
        try {
            cols = split_csv_line(line);
        } catch (const std::exception& ex) {
            record_failure(where + ": " + ex.what());
            continue;
        }
        if (cols.size() < min_columns) {
            record_failure(where + ": expected at least " + std::to_string(min_columns) + " columns");
            continue;
        }

        const std::string rel = trim(cols[path_col]);
        if (rel.empty()) {
            record_failure(where + ": empty path");
            continue;
        }
        Label expected;
        if (!parse_label(cols[label_col], &expected)) {
            record_failure(where + " (" + rel + "): invalid label '" + trim(cols[label_col]) +
                           "' (expected 'clean' or 'stego')");
            continue;
        }

        // Relative paths resolve against the manifest's directory; absolute paths are kept.
        std::filesystem::path image_path(rel);
        if (image_path.is_relative()) {
            image_path = base_dir / image_path;
        }

        DetectionResult detection;
        try {
            const DecodedImage decoded = load_png(image_path);
            const Image image = preprocess_image(decoded);
            const HistogramResult histogram = compute_histogram_sequential(image);
            detection = classify(compute_chi_square(histogram), classifier_config);
        } catch (const std::exception& ex) {
            record_failure(where + " (" + rel + "): " + ex.what());
            continue;
        }

        // Positive prediction == flagged. NoEvidenceDetected and InsufficientData are both
        // "not flagged": a stego image with either outcome is a miss (FN), a clean image with
        // either outcome is correctly not flagged (TN).
        const bool flagged = detection.status == DetectionStatus::PotentialLsbEvidence;
        if (expected == Label::Stego) {
            if (flagged) ++report.metrics.true_positives;
            else ++report.metrics.false_negatives;
        } else {
            if (flagged) ++report.metrics.false_positives;
            else ++report.metrics.true_negatives;
        }
        ++report.total_images_evaluated; // images that produced a prediction (failures excluded)
    }

    return report;
}

// ---------------------------------------------------------------------------------------
// Benchmarking
// ---------------------------------------------------------------------------------------

namespace {

// Written to after every timed call so the optimizer cannot discard the histogram work.
volatile std::uint64_t g_benchmark_sink = 0;

double median(std::vector<double> values) {
    std::sort(values.begin(), values.end());
    const std::size_t n = values.size();
    return (n % 2 == 1) ? values[n / 2] : 0.5 * (values[n / 2 - 1] + values[n / 2]);
}

template <typename Fn>
double time_median(Fn&& fn, int warmups, int repetitions) {
    for (int i = 0; i < warmups; ++i) {
        g_benchmark_sink = g_benchmark_sink + fn();
    }
    std::vector<double> samples;
    samples.reserve(static_cast<std::size_t>(repetitions));
    for (int i = 0; i < repetitions; ++i) {
        const double start = omp_get_wtime();
        const std::uint64_t checksum = fn();
        const double stop = omp_get_wtime();
        g_benchmark_sink = g_benchmark_sink + checksum;
        samples.push_back(std::max(0.0, stop - start));
    }
    return median(std::move(samples));
}

} // namespace

BenchmarkResult benchmark_analyzers(
    const Image& image,
    const BenchmarkConfig& config) {
    if (config.thread_counts.empty()) {
        throw std::invalid_argument("benchmark_analyzers: thread_counts must not be empty");
    }
    for (int t : config.thread_counts) {
        if (t < 1) {
            throw std::invalid_argument("benchmark_analyzers: every thread count must be >= 1");
        }
    }
    if (config.repetitions < 1) {
        throw std::invalid_argument("benchmark_analyzers: repetitions must be >= 1");
    }
    if (config.warmup_repetitions < 0) {
        throw std::invalid_argument("benchmark_analyzers: warmup_repetitions must be >= 0");
    }

    std::string error;
    if (!validate_image(image, &error)) {
        throw std::runtime_error("benchmark_analyzers: invalid image: " + error);
    }

    // Correctness first: nothing is timed until every parallel configuration reproduces the
    // sequential histogram bin-for-bin.
    const HistogramResult reference = compute_histogram_sequential(image);
    std::vector<int> actual_threads;
    actual_threads.reserve(config.thread_counts.size());
    for (int t : config.thread_counts) {
        const HistogramResult parallel = compute_histogram_parallel(image, t);
        if (!are_histograms_equal(reference, parallel)) {
            throw std::runtime_error("benchmark_analyzers: parallel histogram differs from the "
                                     "sequential histogram with " + std::to_string(t) +
                                     " requested thread(s)");
        }
        actual_threads.push_back(parallel.actual_threads);
    }

    auto checksum_of = [](const HistogramResult& h) {
        return h.channels[0][0] + h.samples_per_channel;
    };

    BenchmarkResult result;
    result.sequential_time_seconds = time_median(
        [&]() { return checksum_of(compute_histogram_sequential(image)); },
        config.warmup_repetitions, config.repetitions);

    for (std::size_t i = 0; i < config.thread_counts.size(); ++i) {
        const int requested = config.thread_counts[i];

        BenchmarkRun run;
        run.requested_threads = requested;
        run.actual_threads = actual_threads[i];
        run.elapsed_seconds = time_median(
            [&]() { return checksum_of(compute_histogram_parallel(image, requested)); },
            config.warmup_repetitions, config.repetitions);

        // A time below timer resolution would divide by zero; report 0 speedup/efficiency then.
        if (run.elapsed_seconds > 0.0 && result.sequential_time_seconds > 0.0) {
            run.speedup = result.sequential_time_seconds / run.elapsed_seconds;
            run.efficiency = run.speedup / static_cast<double>(run.actual_threads);
        }
        result.parallel_runs.push_back(run);
    }

    return result;
}

} // namespace stegoscan
