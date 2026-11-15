#include "app/app_controller.h"
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
