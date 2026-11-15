#ifndef STEGOSCAN_IMAGE_IMAGE_H_
#define STEGOSCAN_IMAGE_IMAGE_H_

#include <cstdint>
#include <string>
#include <vector>

namespace stegoscan {

/**
 * @brief Normalized image format used across all analysis, statistical, and export modules.
 *
 * Invariants:
 * - Pixel order is row-major and interleaved (for RGB: R, G, B, R, G, B, ...).
 * - Pixel values are unsigned 8-bit integers (0-255).
 * - Normalized channels: exactly 1 (Grayscale) or 3 (RGB).
 * - Alpha channels are NEVER present in normalized Image (they are discarded during preprocessing).
 * - width > 0 and height > 0.
 * - pixels.size() == width * height * channels without arithmetic overflow.
 */
struct Image {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint8_t channels = 0; // Exactly 1 (Grayscale) or 3 (RGB)
    std::vector<std::uint8_t> pixels;
};

/**
 * @brief Validates that an Image satisfies all contract invariants.
 *
 * Checks:
 * - width > 0 and height > 0
 * - channels == 1 (Grayscale) or channels == 3 (RGB)
 * - pixels.size() == width * height * channels (calculated safely against 32-bit integer overflow)
 *
 * @param image The image to validate.
 * @param error_msg Optional pointer to receive a descriptive error message if invalid.
 * @return true if valid, false otherwise.
 */
bool validate_image(const Image& image, std::string* error_msg = nullptr);

} // namespace stegoscan

#endif // STEGOSCAN_IMAGE_IMAGE_H_
