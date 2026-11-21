#include "evaluation/synthetic_dataset_generator.h"

#include <cstdint>
#include <exception>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void print_usage(std::ostream& os) {
    os << "Usage:\n"
       << "  generate_test_dataset --output-dir <DIR> [--width <W>] [--height <H>]\n"
       << "                        [--seed <S>] [--payload-rates <r1,r2,...>]\n\n"
       << "Options:\n"
       << "  --output-dir <DIR>        Destination directory for dataset PNGs and manifest.csv\n"
       << "  --width <W>               Image width in pixels (default: 512)\n"
       << "  --height <H>              Image height in pixels (default: 512)\n"
       << "  --seed <S>                Seed for the pseudo-random generator (default: 12345)\n"
       << "  --payload-rates <list>    Comma-separated LSB payload rates, 0 < r <= 1\n"
       << "                            (default: 0.10,0.25,0.50,1.00)\n"
       << "  --help                    Display this help message\n\n"
       << "Note: the generated images are synthetic. Metrics computed on them are not\n"
       << "representative of real-world photographic detection accuracy.\n";
}

std::uint64_t parse_unsigned(const std::string& option, const std::string& text) {
    // std::stoull accepts a leading '-' (wrapping around); reject it explicitly.
    if (text.empty() || text.find_first_not_of("0123456789") != std::string::npos) {
        throw std::invalid_argument(option + " expects a non-negative integer, got '" + text + "'");
    }
    try {
        return std::stoull(text);
    } catch (const std::exception&) {
        throw std::invalid_argument(option + " value out of range: '" + text + "'");
    }
}

std::vector<double> parse_rates(const std::string& text) {
    std::vector<double> rates;
    std::stringstream ss(text);
    std::string token;
    while (std::getline(ss, token, ',')) {
        std::size_t used = 0;
        double value = 0.0;
        try {
            value = std::stod(token, &used);
        } catch (const std::exception&) {
            used = 0;
        }
        if (token.empty() || used != token.size()) {
            throw std::invalid_argument("--payload-rates: invalid number '" + token + "'");
        }
        rates.push_back(value);
    }
    if (rates.empty()) {
        throw std::invalid_argument("--payload-rates: at least one rate is required");
    }
    return rates;
}

} // namespace

int main(int argc, char* argv[]) {
    stegoscan::SyntheticDatasetConfig config;

    try {
        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            auto value = [&](const char* name) -> std::string {
                if (i + 1 >= argc) {
                    throw std::invalid_argument(std::string(name) + " requires a value");
                }
                return argv[++i];
            };

            if (arg == "--help" || arg == "-h") {
                print_usage(std::cout);
                return 0;
            } else if (arg == "--output-dir") {
                config.output_directory = value("--output-dir");
            } else if (arg == "--width") {
                const auto v = parse_unsigned("--width", value("--width"));
                if (v > UINT32_MAX) throw std::invalid_argument("--width out of range");
                config.width = static_cast<std::uint32_t>(v);
            } else if (arg == "--height") {
                const auto v = parse_unsigned("--height", value("--height"));
                if (v > UINT32_MAX) throw std::invalid_argument("--height out of range");
                config.height = static_cast<std::uint32_t>(v);
            } else if (arg == "--seed") {
                config.seed = parse_unsigned("--seed", value("--seed"));
            } else if (arg == "--payload-rates") {
                config.payload_rates = parse_rates(value("--payload-rates"));
            } else {
                throw std::invalid_argument("Unknown argument: " + arg);
            }
        }

        if (argc <= 1 || config.output_directory.empty()) {
            print_usage(std::cerr);
            return 1;
        }

        stegoscan::generate_synthetic_dataset(config);
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
    }

    std::cout << "Synthetic dataset written to " << config.output_directory.string()
              << " (manifest.csv, " << config.width << "x" << config.height
              << ", seed " << config.seed << ", " << config.payload_rates.size()
              << " payload rate(s)).\n"
              << "Reminder: synthetic images only; results are not real-world detection accuracy.\n";
    return 0;
}
