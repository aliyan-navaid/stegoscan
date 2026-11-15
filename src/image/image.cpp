#include "image/image.h"
#include <limits>

namespace stegoscan {

bool validate_image(const Image& image, std::string* error_msg) {
    if (image.width == 0) {
        if (error_msg) *error_msg = "Image width must be greater than zero";
        return false;
    }
    if (image.height == 0) {
        if (error_msg) *error_msg = "Image height must be greater than zero";
        return false;
    }
    if (image.channels != 1 && image.channels != 3) {
        if (error_msg) {
            *error_msg = "Normalized Image channels must be 1 (Grayscale) or 3 (RGB), got " +
                         std::to_string(static_cast<int>(image.channels));
        }
        return false;
    }

    // Overflow-safe expected size calculation using 64-bit unsigned integers
    const std::uint64_t w = image.width;
    const std::uint64_t h = image.height;
    const std::uint64_t c = image.channels;
    const std::uint64_t expected_size = w * h * c;

    // Guard against potential size_t overflow
    if (expected_size > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
        if (error_msg) *error_msg = "Image dimensions exceed addressable memory limits";
        return false;
    }

    if (image.pixels.size() != static_cast<std::size_t>(expected_size)) {
        if (error_msg) {
            *error_msg = "Image buffer size mismatch: expected " +
                         std::to_string(expected_size) + " bytes, but got " +
                         std::to_string(image.pixels.size());
        }
        return false;
    }

    return true;
}

} // namespace stegoscan
