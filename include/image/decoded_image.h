#ifndef STEGOSCAN_IMAGE_DECODED_IMAGE_H_
#define STEGOSCAN_IMAGE_DECODED_IMAGE_H_

#include <cstdint>
#include <string>
#include <vector>

namespace stegoscan {

/**
 * @brief Represents raw pixel data as returned directly by the PNG decoder.
 *
 * Supported decoder channel counts:
 * - 1: 8-bit Grayscale (Y)
 * - 2: 8-bit Grayscale with alpha (Y, A)
 * - 3: 8-bit RGB (R, G, B)
 * - 4: 8-bit RGBA (R, G, B, A)
 *
 * Indexed/palette PNGs must be expanded to RGB (or RGBA if transparency is present)
 * by the PNG loader before returning DecodedImage.
 *
 * Pixel storage is row-major and interleaved:
 * - 1 ch:  Y, Y, Y, ...
 * - 2 ch:  Y, A, Y, A, ...
 * - 3 ch:  R, G, B, R, G, B, ...
 * - 4 ch:  R, G, B, A, R, G, B, A, ...
 */
struct DecodedImage {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint8_t channels = 0; // 1, 2, 3, or 4
    std::vector<std::uint8_t> pixels;
};

/**
 * @brief Validates that a DecodedImage satisfies contract invariants.
 *
 * Checks:
 * - width > 0 and height > 0
 * - channels in {1, 2, 3, 4}
 * - pixels.size() == width * height * channels (calculated safely against 32-bit integer overflow)
 *
 * @param image The decoded image to validate.
 * @param error_msg Optional pointer to receive a descriptive error message if invalid.
 * @return true if valid, false otherwise.
 */
bool validate_decoded_image(const DecodedImage& image, std::string* error_msg = nullptr);

} // namespace stegoscan

#endif // STEGOSCAN_IMAGE_DECODED_IMAGE_H_
