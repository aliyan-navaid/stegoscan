#include "test_harness.h"
#include "app/app_controller.h"
#include "evaluation/evaluation.h"

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
