/*#include "app/app_controller.h"
#include <iostream>
#include <string_view>

namespace stegoscan {

void print_help(const std::string& program_name) {
    std::cout << "Usage:\n"
              << "  " << program_name << " <image.png> --mode sequential\n"
              << "  " << program_name << " <image.png> --mode parallel --threads <N>\n"
              << "  " << program_name << " <image.png> --mode compare --threads <N>\n"
              << "  " << program_name << " <image.png> --mode parallel --threads <N> --threshold <P>\n"
              << "  " << program_name << " --evaluate <manifest.csv> --threshold <P>\n"
              << "  " << program_name << " --help\n"
              << "  " << program_name << " --version\n\n"
              << "Options:\n"
              << "  --mode <sequential|parallel|compare>  Histogram analysis mode (default: sequential)\n"
              << "  --threads <N>                         OpenMP thread count for parallel mode (default: 1)\n"
              << "  --threshold <P>                       P-value threshold for classification (default: 0.95)\n"
              << "  --evaluate <manifest.csv>             Evaluate detector against labeled dataset\n"
              << "  --help                                Display this help message\n"
              << "  --version                             Display version information\n";
}

CliOptions parse_cli_args(int argc, char* argv[]) {
    CliOptions options;

    if (argc <= 1) {
        options.error_message = "No arguments provided. Use --help for usage information.";
        return options;
    }

    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            options.show_help = true;
            return options;
        } else if (arg == "--version" || arg == "-v") {
            options.show_version = true;
            return options;
        } else if (arg == "--mode") {
            if (i + 1 >= argc) {
                options.error_message = "--mode requires an argument (sequential, parallel, compare)";
                return options;
            }
            std::string_view mode_str = argv[++i];
            if (mode_str == "sequential") {
                options.mode = ExecutionMode::Sequential;
            } else if (mode_str == "parallel") {
                options.mode = ExecutionMode::Parallel;
            } else if (mode_str == "compare") {
                options.mode = ExecutionMode::Compare;
            } else {
                options.error_message = "Unknown mode: " + std::string(mode_str) +
                                       " (valid modes: sequential, parallel, compare)";
                return options;
            }
        } else if (arg == "--threads") {
            if (i + 1 >= argc) {
                options.error_message = "--threads requires an integer argument";
                return options;
            }
            try {
                options.threads = std::stoi(argv[++i]);
                if (options.threads < 1) {
                    options.error_message = "--threads must be at least 1";
                    return options;
                }
            } catch (const std::exception&) {
                options.error_message = "Invalid integer for --threads";
                return options;
            }
        } else if (arg == "--threshold") {
            if (i + 1 >= argc) {
                options.error_message = "--threshold requires a numeric argument between 0.0 and 1.0";
                return options;
            }
            try {
                options.threshold = std::stod(argv[++i]);
                if (options.threshold < 0.0 || options.threshold > 1.0) {
                    options.error_message = "--threshold must be between 0.0 and 1.0";
                    return options;
                }
            } catch (const std::exception&) {
                options.error_message = "Invalid number for --threshold";
                return options;
            }
        } else if (arg == "--evaluate") {
            if (i + 1 >= argc) {
                options.error_message = "--evaluate requires a manifest path";
                return options;
            }
            options.mode = ExecutionMode::Evaluate;
            options.manifest_path = argv[++i];
        } else if (!arg.empty() && arg[0] == '-') {
            options.error_message = "Unknown option: " + std::string(arg);
            return options;
        } else {
            if (options.image_path.empty()) {
                options.image_path = arg;
            } else {
                options.error_message = "Unexpected extra argument: " + std::string(arg);
                return options;
            }
        }
    }

    if (options.mode == ExecutionMode::Evaluate) {
        if (options.manifest_path.empty()) {
            options.error_message = "--evaluate requires a valid manifest path";
        }
    } else {
        if (options.image_path.empty()) {
            options.error_message = "No input image PNG path specified";
        }
    }

    return options;
}

int run_application(const CliOptions& options) {
    if (options.show_help) {
        print_help("stegdetector");
        return 0;
    }

    if (options.show_version) {
        std::cout << "stegdetector version 0.1.0 (Scaffolding Phase)\n";
        return 0;
    }

    if (!options.error_message.empty()) {
        std::cerr << "Error: " << options.error_message << "\n\n";
        print_help("stegdetector");
        return 1;
    }

    std::cout << "Stegoscan CLI initialized in scaffolding mode.\n"
              << "Modules are currently defined as interfaces and stubs.\n";
    return 0;
}

} // namespace stegoscan
*/
#include "app/app_controller.h"

// Include the modules you are orchestrating
#include "image/image_loader.h"
#include "image/preprocessor.h"
#include "analysis/sequential_analyzer.h"
#include "analysis/parallel_analyzer.h"
#include "analysis/histogram.h"
#include "analysis/chi_square.h"
#include "detection/classifier.h"
#include "evaluation/evaluation.h"

#include <iostream>
#include <string>
#include <string_view>
#include <chrono>
#include <iomanip>
#include <exception>
#include <filesystem>

namespace stegoscan {

void print_help(const std::string& program_name) {
    std::cout << "Usage:\n"
              << "  " << program_name << " <image.png> --mode sequential\n"
              << "  " << program_name << " <image.png> --mode parallel --threads <N>\n"
              << "  " << program_name << " <image.png> --mode compare --threads <N>\n"
              << "  " << program_name << " <image.png> --mode parallel --threads <N> --threshold <P>\n"
              << "  " << program_name << " --evaluate <manifest.csv> --threshold <P>\n"
              << "  " << program_name << " --help\n"
              << "  " << program_name << " --version\n\n"
              << "Options:\n"
              << "  --mode <sequential|parallel|compare>  Histogram analysis mode (default: sequential)\n"
              << "  --threads <N>                         OpenMP thread count for parallel mode (default: 1)\n"
              << "  --threshold <P>                       P-value threshold for classification (default: 0.95)\n"
              << "  --evaluate <manifest.csv>             Evaluate detector against labeled dataset\n"
              << "  --help                                Display this help message\n"
              << "  --version                             Display version information\n";
}

CliOptions parse_cli_args(int argc, char* argv[]) {
    CliOptions options;

    if (argc <= 1) {
        options.error_message = "No arguments provided. Use --help for usage information.";
        return options;
    }

    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            options.show_help = true;
            return options;
        } else if (arg == "--version" || arg == "-v") {
            options.show_version = true;
            return options;
        } else if (arg == "--mode") {
            if (i + 1 >= argc) {
                options.error_message = "--mode requires an argument (sequential, parallel, compare)";
                return options;
            }
            std::string_view mode_str = argv[++i];
            if (mode_str == "sequential") {
                options.mode = ExecutionMode::Sequential;
            } else if (mode_str == "parallel") {
                options.mode = ExecutionMode::Parallel;
            } else if (mode_str == "compare") {
                options.mode = ExecutionMode::Compare;
            } else {
                options.error_message = "Unknown mode: " + std::string(mode_str) +
                                       " (valid modes: sequential, parallel, compare)";
                return options;
            }
        } else if (arg == "--threads") {
            if (i + 1 >= argc) {
                options.error_message = "--threads requires an integer argument";
                return options;
            }
            try {
                options.threads = std::stoi(argv[++i]);
                if (options.threads < 1) {
                    options.error_message = "--threads must be at least 1";
                    return options;
                }
            } catch (const std::exception&) {
                options.error_message = "Invalid integer for --threads";
                return options;
            }
        } else if (arg == "--threshold") {
            if (i + 1 >= argc) {
                options.error_message = "--threshold requires a numeric argument between 0.0 and 1.0";
                return options;
            }
            try {
                options.threshold = std::stod(argv[++i]);
                if (options.threshold < 0.0 || options.threshold > 1.0) {
                    options.error_message = "--threshold must be between 0.0 and 1.0";
                    return options;
                }
            } catch (const std::exception&) {
                options.error_message = "Invalid number for --threshold";
                return options;
            }
        } else if (arg == "--evaluate") {
            if (i + 1 >= argc) {
                options.error_message = "--evaluate requires a manifest path";
                return options;
            }
            options.mode = ExecutionMode::Evaluate;
            options.manifest_path = argv[++i];
        } else if (!arg.empty() && arg[0] == '-') {
            options.error_message = "Unknown option: " + std::string(arg);
            return options;
        } else {
            if (options.image_path.empty()) {
                options.image_path = arg;
            } else {
                options.error_message = "Unexpected extra argument: " + std::string(arg);
                return options;
            }
        }
    }

    if (options.mode == ExecutionMode::Evaluate) {
        if (options.manifest_path.empty()) {
            options.error_message = "--evaluate requires a valid manifest path";
        }
    } else {
        if (options.image_path.empty()) {
            options.error_message = "No input image PNG path specified";
        }
    }

    return options;
}

int run_application(const CliOptions& options) {
    if (options.show_help) {
        print_help("stegdetector");
        return 0;
    }

    if (options.show_version) {
        std::cout << "stegdetector version 1.0.0\n";
        return 0;
    }

    if (!options.error_message.empty()) {
        std::cerr << "Error: " << options.error_message << "\n\n";
        print_help("stegdetector");
        return 1;
    }

    try {
        // --- EVALUATE MODE ---
        if (options.mode == ExecutionMode::Evaluate) {
            std::cout << "Evaluating dataset from manifest: " << options.manifest_path << "\n";
            EvaluationConfig eval_config;
            eval_config.threshold = options.threshold;
            
            auto report = evaluate_dataset(options.manifest_path, eval_config);
            
            std::cout << "\n=== Evaluation Report ===\n";
            std::cout << "Threshold: " << report.threshold << "\n";
            std::cout << "Total images evaluated: " << report.total_images_evaluated << "\n";
            std::cout << "Failed images: " << report.failed_images << "\n";
            std::cout << "True Positives:  " << report.metrics.true_positives << "\n";
            std::cout << "True Negatives:  " << report.metrics.true_negatives << "\n";
            std::cout << "False Positives: " << report.metrics.false_positives << "\n";
            std::cout << "False Negatives: " << report.metrics.false_negatives << "\n";
            std::cout << "Accuracy:        " << std::fixed << std::setprecision(4) << report.metrics.accuracy() << "\n";
            std::cout << "False Pos Rate:  " << std::fixed << std::setprecision(4) << report.metrics.false_positive_rate() << "\n";
            std::cout << "Precision:       " << std::fixed << std::setprecision(4) << report.metrics.precision() << "\n";
            std::cout << "Recall:          " << std::fixed << std::setprecision(4) << report.metrics.recall() << "\n";
            
            if (!report.error_messages.empty()) {
                std::cout << "\nErrors encountered:\n";
                for (const auto& err : report.error_messages) {
                    std::cout << "  - " << err << "\n";
                }
            }
            return 0;
        }

        // --- SEQUENTIAL, PARALLEL, COMPARE MODES ---
        std::cout << "Loading image: " << options.image_path << "\n";
        auto decoded = load_png(options.image_path);
        auto image = preprocess_image(decoded);
        std::cout << "Image preprocessed: " << image.width << "x" << image.height 
                  << ", " << static_cast<int>(image.channels) << " channel(s)\n";

        HistogramResult hist_seq;
        HistogramResult hist_par;
        double seq_time = 0.0;
        double par_time = 0.0;

        if (options.mode == ExecutionMode::Sequential || options.mode == ExecutionMode::Compare) {
            auto start = std::chrono::high_resolution_clock::now();
            hist_seq = compute_histogram_sequential(image);
            auto end = std::chrono::high_resolution_clock::now();
            seq_time = std::chrono::duration<double>(end - start).count();
            std::cout << "Sequential histogram computed in " << std::fixed << std::setprecision(6) << seq_time << "s\n";
        }

        if (options.mode == ExecutionMode::Parallel || options.mode == ExecutionMode::Compare) {
            auto start = std::chrono::high_resolution_clock::now();
            hist_par = compute_histogram_parallel(image, options.threads);
            auto end = std::chrono::high_resolution_clock::now();
            par_time = std::chrono::duration<double>(end - start).count();
            std::cout << "Parallel histogram computed in " << std::fixed << std::setprecision(6) << par_time 
                      << "s using " << hist_par.actual_threads << " thread(s)\n";
        }

        if (options.mode == ExecutionMode::Compare) {
            std::cout << "\n=== Comparison Results ===\n";
            bool equal = are_histograms_equal(hist_seq, hist_par);
            std::cout << "Histograms identical: " << (equal ? "Yes" : "NO - MISMATCH!") << "\n";
            if (!equal) {
                std::cerr << "Error: Parallel and sequential histograms do not match!\n";
                return 2;
            }
            if (par_time > 0.0) {
                double speedup = seq_time / par_time;
                double efficiency = speedup / hist_par.actual_threads;
                std::cout << "Speedup: " << std::fixed << std::setprecision(2) << speedup << "x\n";
                std::cout << "Efficiency: " << std::fixed << std::setprecision(2) << (efficiency * 100.0) << "%\n";
            }
        }

        // Run Detection Pipeline (Chi-Square + Classifier)
        HistogramResult final_hist = (options.mode == ExecutionMode::Sequential) ? hist_seq : hist_par;
        auto chi_results = compute_chi_square(final_hist);

        ClassifierConfig config;
        config.p_value_threshold = options.threshold;
        auto detection = classify(chi_results, config);

        std::cout << "\n=== Detection Results ===\n";
        std::cout << "Status: " << detection_status_to_string(detection.status) << "\n";
        std::cout << "Threshold: " << detection.p_value_threshold << "\n";
        std::cout << "Explanation: " << detection.explanation << "\n";
        std::cout << "Channel Details:\n";
        for (const auto& ch : detection.channels) {
            std::cout << "  Channel " << ch.channel_index << ": ";
            if (ch.chi_square.is_valid) {
                std::cout << "p-value=" << std::fixed << std::setprecision(4) << ch.chi_square.p_value
                          << " (Suspicious: " << (ch.is_suspicious ? "Yes" : "No") << ")\n";
            } else {
                std::cout << "Invalid/Insufficient data (" << ch.chi_square.note << ")\n";
            }
        }

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "\nFatal error: " << e.what() << "\n";
        return 2;
    }
}

} // namespace stegoscan