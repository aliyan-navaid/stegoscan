#include "evaluation/synthetic_dataset_generator.h"
#include <iostream>
#include <string_view>

int main(int argc, char* argv[]) {
    stegoscan::SyntheticDatasetConfig config;
    bool show_help = false;

    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            show_help = true;
            break;
        } else if (arg == "--output-dir" && i + 1 < argc) {
            config.output_directory = argv[++i];
        } else if (arg == "--width" && i + 1 < argc) {
            config.width = static_cast<std::uint32_t>(std::stoul(argv[++i]));
        } else if (arg == "--height" && i + 1 < argc) {
            config.height = static_cast<std::uint32_t>(std::stoul(argv[++i]));
        } else if (arg == "--seed" && i + 1 < argc) {
            config.seed = std::stoull(argv[++i]);
        }
    }

    if (show_help || argc <= 1) {
        std::cout << "Usage:\n"
                  << "  generate_test_dataset --output-dir <DIR> [--width <W>] [--height <H>] [--seed <S>]\n\n"
                  << "Options:\n"
                  << "  --output-dir <DIR>   Destination directory for dataset PNGs and manifest.csv\n"
                  << "  --width <W>          Image width in pixels (default: 512)\n"
                  << "  --height <H>         Image height in pixels (default: 512)\n"
                  << "  --seed <S>           Seed for pseudo-random number generator (default: 12345)\n"
                  << "  --help               Display this help message\n";
        return 0;
    }

    std::cout << "generate_test_dataset tool initialized (Scaffolding Phase).\n";
    try {
        stegoscan::generate_synthetic_dataset(config);
    } catch (const std::exception& ex) {
        std::cerr << "Notice: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}
