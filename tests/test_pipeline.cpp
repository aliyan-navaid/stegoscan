#include "test_harness.h"
#include "app/app_controller.h"
#include "evaluation/evaluation.h"
#include "evaluation/synthetic_dataset_generator.h"
#include "image/png_writer.h"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include <vector>

namespace {

namespace fs = std::filesystem;

struct EvalFixtureDirectory {
    fs::path dir;
    explicit EvalFixtureDirectory(const std::string& name) {
        dir = fs::temp_directory_path() / name;
        std::error_code ec;
        fs::remove_all(dir, ec);
        fs::create_directories(dir);
    }
    ~EvalFixtureDirectory() {
        std::error_code ec;
        fs::remove_all(dir, ec);
    }
};

void write_text(const fs::path& p, const std::string& text) {
    fs::create_directories(p.parent_path());
    std::ofstream out(p, std::ios::binary);
    out << text;
}

// 16x16 grayscale image containing every value 0..255 exactly once. Every even/odd pair is
// perfectly balanced, so the pairwise chi-square statistic is 0 and p == 1 (flagged).
stegoscan::Image equalized_image() {
    stegoscan::Image img;
    img.width = 16;
    img.height = 16;
    img.channels = 1;
    for (int v = 0; v < 256; ++v) img.pixels.push_back(static_cast<std::uint8_t>(v));
    return img;
}

// 16x16 grayscale image where every sample is 10 (even). The (10,11) pair is maximally
// unbalanced: statistic 256, df 1, p ~ 0 (not flagged at any sensible threshold).
stegoscan::Image skewed_image() {
    stegoscan::Image img;
    img.width = 16;
    img.height = 16;
    img.channels = 1;
    img.pixels.assign(256, 10);
    return img;
}

stegoscan::Image rgb_test_image(std::uint32_t w, std::uint32_t h) {
    stegoscan::Image img;
    img.width = w;
    img.height = h;
    img.channels = 3;
    img.pixels.resize(static_cast<std::size_t>(w) * h * 3);
    for (std::size_t i = 0; i < img.pixels.size(); ++i) {
        img.pixels[i] = static_cast<std::uint8_t>((i * 131 + 7) % 251);
    }
    return img;
}

} // namespace

TEST_CASE(EvaluationMetrics, ConfusionMatrixCalculations) {
    stegoscan::ConfusionMatrix cm;
    cm.true_positives = 8;
    cm.true_negatives = 8;
    cm.false_positives = 2;
    cm.false_negatives = 2;

    ASSERT_NEAR(cm.accuracy(), 16.0 / 20.0, 1e-6);
    ASSERT_NEAR(cm.false_positive_rate(), 2.0 / 10.0, 1e-6);
    ASSERT_NEAR(cm.precision(), 8.0 / 10.0, 1e-6);
    ASSERT_NEAR(cm.recall(), 8.0 / 10.0, 1e-6);
}

TEST_CASE(EvaluationMetrics, ZeroDenominatorProtection) {
    stegoscan::ConfusionMatrix cm;
    ASSERT_EQ(cm.accuracy(), 0.0);
    ASSERT_EQ(cm.false_positive_rate(), 0.0);
    ASSERT_EQ(cm.precision(), 0.0);
    ASSERT_EQ(cm.recall(), 0.0);
}

TEST_CASE(CliParsing, HelpAndVersion) {
    char prog[] = "stegdetector";
    char flag_h[] = "--help";
    char* argv_h[] = {prog, flag_h};
    auto opts_h = stegoscan::parse_cli_args(2, argv_h);
    ASSERT_TRUE(opts_h.show_help);

    char flag_v[] = "--version";
    char* argv_v[] = {prog, flag_v};
    auto opts_v = stegoscan::parse_cli_args(2, argv_v);
    ASSERT_TRUE(opts_v.show_version);
}

TEST_CASE(CliParsing, ModeAndParameters) {
    char prog[] = "stegdetector";
    char img[] = "image.png";
    char mode_flag[] = "--mode";
    char mode_val[] = "parallel";
    char thr_flag[] = "--threads";
    char thr_val[] = "4";
    char thres_flag[] = "--threshold";
    char thres_val[] = "0.90";

    char* argv[] = {prog, img, mode_flag, mode_val, thr_flag, thr_val, thres_flag, thres_val};
    auto opts = stegoscan::parse_cli_args(8, argv);

    ASSERT_TRUE(opts.error_message.empty());
    ASSERT_EQ(opts.image_path.string(), "image.png");
    ASSERT_EQ(static_cast<int>(opts.mode), static_cast<int>(stegoscan::ExecutionMode::Parallel));
    ASSERT_EQ(opts.threads, 4);
    ASSERT_NEAR(opts.threshold, 0.90, 1e-6);
}

TEST_CASE(CliParsing, RejectInvalidParameters) {
    char prog[] = "stegdetector";
    char img[] = "image.png";
    char mode_flag[] = "--mode";
    char mode_val[] = "invalid_mode";
    char* argv[] = {prog, img, mode_flag, mode_val};

    auto opts = stegoscan::parse_cli_args(4, argv);
    ASSERT_FALSE(opts.error_message.empty());
}

// ---------------------------------------------------------------------------------------
// evaluate_dataset
// ---------------------------------------------------------------------------------------

TEST_CASE(EvaluationDataset, HandLabeledSetYieldsKnownConfusionMatrix) {
    EvalFixtureDirectory fx("stegoscan_eval_known");
    stegoscan::write_png(fx.dir / "stego_hit.png", equalized_image());   // stego, flagged  -> TP
    stegoscan::write_png(fx.dir / "stego_miss.png", skewed_image());     // stego, missed   -> FN
    stegoscan::write_png(fx.dir / "clean_ok.png", skewed_image());       // clean, ignored  -> TN
    stegoscan::write_png(fx.dir / "clean_bad.png", equalized_image());   // clean, flagged  -> FP
    write_text(fx.dir / "manifest.csv",
               "path,label\n"
               "stego_hit.png,stego\n"
               "stego_miss.png,stego\n"
               "clean_ok.png,clean\n"
               "clean_bad.png,clean\n");

    const auto report = stegoscan::evaluate_dataset(fx.dir / "manifest.csv");
    ASSERT_EQ(report.metrics.true_positives, static_cast<std::uint64_t>(1));
    ASSERT_EQ(report.metrics.false_negatives, static_cast<std::uint64_t>(1));
    ASSERT_EQ(report.metrics.true_negatives, static_cast<std::uint64_t>(1));
    ASSERT_EQ(report.metrics.false_positives, static_cast<std::uint64_t>(1));
    ASSERT_EQ(report.total_images_evaluated, static_cast<std::size_t>(4));
    ASSERT_EQ(report.failed_images, static_cast<std::size_t>(0));
    ASSERT_NEAR(report.metrics.accuracy(), 0.5, 1e-12);
    ASSERT_NEAR(report.threshold, 0.95, 1e-12);
}

TEST_CASE(EvaluationDataset, ThresholdIsAppliedAndRecorded) {
    EvalFixtureDirectory fx("stegoscan_eval_threshold");
    stegoscan::write_png(fx.dir / "skewed.png", skewed_image());      // p ~ 0 but >= 0.0
    stegoscan::write_png(fx.dir / "equal.png", equalized_image());    // p == 1
    write_text(fx.dir / "m.csv", "path,label\nskewed.png,clean\nequal.png,clean\n");

    stegoscan::EvaluationConfig cfg;
    cfg.threshold = 0.0; // every valid channel satisfies p >= 0
    auto r = stegoscan::evaluate_dataset(fx.dir / "m.csv", cfg);
    ASSERT_NEAR(r.threshold, 0.0, 1e-12);
    ASSERT_EQ(r.metrics.false_positives, static_cast<std::uint64_t>(2));

    cfg.threshold = 1.0; // only p == 1 (to within the classifier's >= rule) is flagged
    r = stegoscan::evaluate_dataset(fx.dir / "m.csv", cfg);
    ASSERT_NEAR(r.threshold, 1.0, 1e-12);
    ASSERT_EQ(r.metrics.false_positives, static_cast<std::uint64_t>(1));
    ASSERT_EQ(r.metrics.true_negatives, static_cast<std::uint64_t>(1));
}

TEST_CASE(EvaluationDataset, RelativePathsResolveAgainstManifestDirectory) {
    EvalFixtureDirectory fx("stegoscan_eval_paths");
    fs::create_directories(fx.dir / "data/sub");
    stegoscan::write_png(fx.dir / "data/sub/a.png", equalized_image());
    stegoscan::write_png(fx.dir / "abs.png", equalized_image());
    // Manifest sits in its own directory; the working directory is irrelevant.
    write_text(fx.dir / "meta/manifest.csv",
               "path,label\n"
               "../data/sub/a.png,stego\n" +
               (fx.dir / "abs.png").string() + ",stego\n");

    const auto report = stegoscan::evaluate_dataset(fx.dir / "meta/manifest.csv");
    ASSERT_EQ(report.failed_images, static_cast<std::size_t>(0));
    ASSERT_EQ(report.metrics.true_positives, static_cast<std::uint64_t>(2));
}

TEST_CASE(EvaluationDataset, FailedImagesAreReportedNotCounted) {
    EvalFixtureDirectory fx("stegoscan_eval_failures");
    stegoscan::write_png(fx.dir / "ok.png", equalized_image());
    write_text(fx.dir / "notpng.png", "this is not a png");
    write_text(fx.dir / "m.csv",
               "path,label\n"
               "ok.png,stego\n"
               "missing.png,clean\n"      // missing file
               "notpng.png,stego\n"       // corrupt
               "ok.png,maybe\n"           // invalid label
               "ok.png\n"                 // too few columns
               "\n"                       // blank line (ignored, not an error)
               ",clean\n");               // empty path

    const auto report = stegoscan::evaluate_dataset(fx.dir / "m.csv");
    ASSERT_EQ(report.total_images_evaluated, static_cast<std::size_t>(1));
    ASSERT_EQ(report.failed_images, static_cast<std::size_t>(5));
    ASSERT_EQ(report.error_messages.size(), static_cast<std::size_t>(5));
    // Only the one good image reached the confusion matrix.
    const auto& m = report.metrics;
    ASSERT_EQ(m.true_positives + m.true_negatives + m.false_positives + m.false_negatives,
              static_cast<std::uint64_t>(1));
    ASSERT_EQ(m.true_positives, static_cast<std::uint64_t>(1));
}

TEST_CASE(EvaluationDataset, AcceptsCrlfBomExtraColumnsQuotingAndColumnOrder) {
    EvalFixtureDirectory fx("stegoscan_eval_csvforms");
    stegoscan::write_png(fx.dir / "a b.png", equalized_image());
    write_text(fx.dir / "m.csv",
               "\xEF\xBB\xBF" "note,Label,PATH\r\n"
               "first,STEGO,\"a b.png\"\r\n"
               "second , clean ,a b.png\r\n");

    const auto report = stegoscan::evaluate_dataset(fx.dir / "m.csv");
    ASSERT_EQ(report.failed_images, static_cast<std::size_t>(0));
    ASSERT_EQ(report.metrics.true_positives, static_cast<std::uint64_t>(1));
    ASSERT_EQ(report.metrics.false_positives, static_cast<std::uint64_t>(1));
}

TEST_CASE(EvaluationDataset, RejectsBadManifestAndThreshold) {
    EvalFixtureDirectory fx("stegoscan_eval_bad_manifest");
    ASSERT_THROW(stegoscan::evaluate_dataset(fx.dir / "does_not_exist.csv"), std::runtime_error);

    write_text(fx.dir / "empty.csv", "");
    ASSERT_THROW(stegoscan::evaluate_dataset(fx.dir / "empty.csv"), std::runtime_error);

    write_text(fx.dir / "nolabel.csv", "path,kind\na.png,clean\n");
    ASSERT_THROW(stegoscan::evaluate_dataset(fx.dir / "nolabel.csv"), std::runtime_error);

    write_text(fx.dir / "ok.csv", "path,label\n");
    stegoscan::EvaluationConfig cfg;
    cfg.threshold = 1.5;
    ASSERT_THROW(stegoscan::evaluate_dataset(fx.dir / "ok.csv", cfg), std::invalid_argument);
    cfg.threshold = -0.1;
    ASSERT_THROW(stegoscan::evaluate_dataset(fx.dir / "ok.csv", cfg), std::invalid_argument);
    cfg.threshold = std::numeric_limits<double>::quiet_NaN();
    ASSERT_THROW(stegoscan::evaluate_dataset(fx.dir / "ok.csv", cfg), std::invalid_argument);

    // Header-only manifest is valid and yields an empty report with zero-safe metrics.
    const auto report = stegoscan::evaluate_dataset(fx.dir / "ok.csv");
    ASSERT_EQ(report.total_images_evaluated, static_cast<std::size_t>(0));
    ASSERT_EQ(report.metrics.accuracy(), 0.0);
}

TEST_CASE(EvaluationDataset, ConsumesGeneratedSyntheticDataset) {
    EvalFixtureDirectory fx("stegoscan_eval_generated");
    stegoscan::SyntheticDatasetConfig gen;
    gen.output_directory = fx.dir / "ds";
    gen.width = 64;
    gen.height = 64;
    gen.payload_rates = {0.5, 1.0};
    stegoscan::generate_synthetic_dataset(gen);

    const std::size_t covers = 6;
    const std::size_t stego = covers * gen.payload_rates.size();

    stegoscan::EvaluationConfig cfg;
    cfg.threshold = 0.0;
    auto r = stegoscan::evaluate_dataset(gen.output_directory / "manifest.csv", cfg);
    ASSERT_EQ(r.failed_images, static_cast<std::size_t>(0));
    ASSERT_EQ(r.total_images_evaluated, covers + stego);
    // Threshold 0 flags everything: all stego are TP, all clean are FP.
    ASSERT_EQ(r.metrics.true_positives, static_cast<std::uint64_t>(stego));
    ASSERT_EQ(r.metrics.false_positives, static_cast<std::uint64_t>(covers));
    ASSERT_EQ(r.metrics.false_negatives + r.metrics.true_negatives, static_cast<std::uint64_t>(0));

    // Raising the threshold can only reduce the number of flagged images.
    std::uint64_t previous_flagged = r.metrics.true_positives + r.metrics.false_positives;
    for (double t : {0.25, 0.5, 0.95, 1.0}) {
        cfg.threshold = t;
        r = stegoscan::evaluate_dataset(gen.output_directory / "manifest.csv", cfg);
        ASSERT_EQ(r.failed_images, static_cast<std::size_t>(0));
        ASSERT_EQ(r.total_images_evaluated, covers + stego);
        const std::uint64_t flagged = r.metrics.true_positives + r.metrics.false_positives;
        ASSERT_TRUE(flagged <= previous_flagged);
        previous_flagged = flagged;
    }
}

// ---------------------------------------------------------------------------------------
// benchmark_analyzers
// ---------------------------------------------------------------------------------------

TEST_CASE(Benchmark, ProducesConsistentFiniteResults) {
    const auto image = rgb_test_image(257, 131);
    stegoscan::BenchmarkConfig cfg;
    cfg.thread_counts = {1, 2, 4};
    cfg.warmup_repetitions = 1;
    cfg.repetitions = 3;

    const auto result = stegoscan::benchmark_analyzers(image, cfg);
    ASSERT_TRUE(std::isfinite(result.sequential_time_seconds));
    ASSERT_TRUE(result.sequential_time_seconds >= 0.0);
    ASSERT_EQ(result.parallel_runs.size(), cfg.thread_counts.size());

    for (std::size_t i = 0; i < result.parallel_runs.size(); ++i) {
        const auto& run = result.parallel_runs[i];
        ASSERT_EQ(run.requested_threads, cfg.thread_counts[i]);
        ASSERT_TRUE(run.actual_threads >= 1);
        ASSERT_TRUE(run.actual_threads <= run.requested_threads);
        ASSERT_TRUE(std::isfinite(run.elapsed_seconds) && run.elapsed_seconds >= 0.0);
        ASSERT_TRUE(std::isfinite(run.speedup) && run.speedup >= 0.0);
        ASSERT_TRUE(std::isfinite(run.efficiency) && run.efficiency >= 0.0);
        if (run.elapsed_seconds > 0.0 && result.sequential_time_seconds > 0.0) {
            ASSERT_NEAR(run.speedup, result.sequential_time_seconds / run.elapsed_seconds, 1e-9);
            ASSERT_NEAR(run.efficiency, run.speedup / run.actual_threads, 1e-9);
        } else {
            // Below timer resolution: documented policy is speedup = efficiency = 0.
            ASSERT_EQ(run.speedup, 0.0);
            ASSERT_EQ(run.efficiency, 0.0);
        }
    }
}

TEST_CASE(Benchmark, WorksWithGrayscaleTinyImageAndMoreThreadsThanRows) {
    stegoscan::Image img;
    img.width = 5;
    img.height = 2;
    img.channels = 1;
    img.pixels = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    stegoscan::BenchmarkConfig cfg;
    cfg.thread_counts = {8};
    cfg.warmup_repetitions = 0;
    cfg.repetitions = 1;
    const auto result = stegoscan::benchmark_analyzers(img, cfg);
    ASSERT_EQ(result.parallel_runs.size(), static_cast<std::size_t>(1));
    ASSERT_EQ(result.parallel_runs[0].requested_threads, 8);
}

TEST_CASE(Benchmark, DefaultConfigRuns) {
    const auto image = rgb_test_image(64, 64);
    const auto result = stegoscan::benchmark_analyzers(image);
    ASSERT_EQ(result.parallel_runs.size(), static_cast<std::size_t>(4)); // 1, 2, 4, 8
}

TEST_CASE(Benchmark, RejectsInvalidConfigAndImage) {
    const auto image = rgb_test_image(8, 8);

    stegoscan::BenchmarkConfig cfg;
    cfg.thread_counts.clear();
    ASSERT_THROW(stegoscan::benchmark_analyzers(image, cfg), std::invalid_argument);

    cfg = stegoscan::BenchmarkConfig{};
    cfg.thread_counts = {2, 0};
    ASSERT_THROW(stegoscan::benchmark_analyzers(image, cfg), std::invalid_argument);

    cfg = stegoscan::BenchmarkConfig{};
    cfg.repetitions = 0;
    ASSERT_THROW(stegoscan::benchmark_analyzers(image, cfg), std::invalid_argument);

    cfg = stegoscan::BenchmarkConfig{};
    cfg.warmup_repetitions = -1;
    ASSERT_THROW(stegoscan::benchmark_analyzers(image, cfg), std::invalid_argument);

    stegoscan::Image bad = image;
    bad.pixels.pop_back();
    ASSERT_THROW(stegoscan::benchmark_analyzers(bad), std::runtime_error);
}
