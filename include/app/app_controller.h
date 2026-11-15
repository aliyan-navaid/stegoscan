#ifndef STEGOSCAN_APP_APP_CONTROLLER_H_
#define STEGOSCAN_APP_APP_CONTROLLER_H_

#include <filesystem>
#include <string>
#include <vector>

namespace stegoscan {

/**
 * @brief Execution mode selected by CLI arguments.
 */
enum class ExecutionMode {
    Sequential, ///< Analyze using sequential histogram analyzer
    Parallel,   ///< Analyze using OpenMP parallel histogram analyzer
    Compare,    ///< Run both analyzers, verify equivalence, and display speedup
    Evaluate    ///< Evaluate detector against labeled manifest CSV
};

/**
 * @brief Parsed command line options for stegdetector.
 */
struct CliOptions {
    ExecutionMode mode = ExecutionMode::Sequential;
    std::filesystem::path image_path;
    std::filesystem::path manifest_path;
    int threads = 1;
    double threshold = 0.95;
    bool show_help = false;
    bool show_version = false;
    std::string error_message;
};

/**
 * @brief Parses CLI arguments into structured CliOptions.
 *
 * Supported syntax:
 *   stegdetector <image.png> --mode sequential
 *   stegdetector <image.png> --mode parallel --threads 4
 *   stegdetector <image.png> --mode compare --threads 4
 *   stegdetector <image.png> --mode parallel --threads 4 --threshold 0.95
 *   stegdetector --evaluate <manifest.csv> --threshold 0.95
 *   stegdetector --help
 *   stegdetector --version
 *
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Parsed CliOptions. If invalid, error_message is populated.
 */
CliOptions parse_cli_args(int argc, char* argv[]);

/**
 * @brief Executes the pipeline according to the provided CliOptions.
 *
 * Orchestrates:
 * 1. Mode dispatch (Sequential, Parallel, Compare, or Evaluate).
 * 2. Image loading and preprocessing.
 * 3. Histogram construction.
 * 4. Pairwise chi-square detection and classification.
 * 5. Formatting human-readable results to std::cout.
 *
 * @param options Validated CLI options.
 * @return 0 on success, non-zero exit code on failure or invalid arguments.
 */
int run_application(const CliOptions& options);

/**
 * @brief Prints usage and help documentation to standard output.
 *
 * @param program_name Name of the invoked binary (typically argv[0]).
 */
void print_help(const std::string& program_name);

} // namespace stegoscan

#endif // STEGOSCAN_APP_APP_CONTROLLER_H_
