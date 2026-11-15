#ifndef STEGOSCAN_EVALUATION_SYNTHETIC_DATASET_GENERATOR_H_
#define STEGOSCAN_EVALUATION_SYNTHETIC_DATASET_GENERATOR_H_

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace stegoscan {

/**
 * @brief Configuration parameters for synthetic test dataset generation.
 */
struct SyntheticDatasetConfig {
    std::filesystem::path output_directory;               ///< Directory where PNGs and manifest.csv are written
    std::uint32_t width = 512;                            ///< Image width in pixels
    std::uint32_t height = 512;                           ///< Image height in pixels
    std::uint64_t seed = 12345;                           ///< Seed for deterministic pseudo-random generation
    std::vector<double> payload_rates{0.10, 0.25, 0.50, 1.00}; ///< Embedding fractions (0 < rate <= 1.0)
};

/**
 * @brief Metadata record for a single image entry in the generated manifest.
 */
struct DatasetManifestEntry {
    std::string relative_path;   ///< Relative path from manifest file to image PNG
    std::string label;           ///< "clean" or "stego"
    std::string source_cover;    ///< Source cover image filename (or empty for covers)
    double payload_rate = 0.0;   ///< Fraction of samples modified (0.0 for clean)
    std::uint64_t seed = 0;      ///< Seed used for generation
    std::string pattern;         ///< Synthetic pattern description (e.g. "gradient", "checker", "noise")
};

/**
 * @brief Deterministically generates synthetic clean and LSB-replacement PNGs with a CSV manifest.
 *
 * Contract:
 * - Creates output_directory if it does not already exist.
 * - Generates multiple distinct synthetic cover image patterns (e.g. smooth gradients,
 *   structured checkerboards, seeded pseudo-random textures) for both Grayscale and RGB.
 * - Saves each cover image as an 8-bit PNG and labels it "clean" in the manifest.
 * - For each cover image and each payload rate in config.payload_rates:
 *   - Calculates count = round(payload_rate * total_channel_samples).
 *   - Selects count distinct sample positions without replacement using the seeded PRNG.
 *   - Embeds pseudo-random bits by LSB replacement: new_val = (old_val & 0xFE) | bit.
 *   - Leaves all unselected sample positions unmodified.
 *   - Does not modify the in-memory cover image while creating stego variants.
 *   - Saves stego variant as an 8-bit PNG and labels it "stego" in the manifest.
 * - Generates manifest.csv using relative paths (relative to output_directory).
 * - Must NOT use OpenMP (dataset generation must be strictly deterministic and reproducible).
 * - Validates: width > 0, height > 0, seed, and 0 < rate <= 1.0 for all payload rates.
 *
 * Note: These synthetic images are solely for functional verification and pipeline demonstration.
 * They do NOT represent real-world camera noise or natural photographic statistics.
 *
 * @param config Dataset generation configuration options.
 * @throws std::invalid_argument on invalid configuration parameters.
 * @throws std::runtime_error on filesystem or PNG encoding failures.
 */
void generate_synthetic_dataset(const SyntheticDatasetConfig& config);

} // namespace stegoscan

#endif // STEGOSCAN_EVALUATION_SYNTHETIC_DATASET_GENERATOR_H_
