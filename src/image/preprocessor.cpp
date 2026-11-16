#include "image/preprocessor.h"

#include <string>

namespace stegoscan {

Image preprocess_image(const DecodedImage& decoded) {
    // 1. Validate the input DecodedImage structure
    std::string val_err;
    if (!validate_decoded_image(decoded, &val_err)) {
        throw PreprocessingError("Invalid DecodedImage passed to preprocess_image: " + val_err);
    }

    const std::uint64_t num_pixels =
        static_cast<std::uint64_t>(decoded.width) * decoded.height;

    Image out;
    out.width = decoded.width;
    out.height = decoded.height;

    // 2. Normalize pixel layout based on input channel count
    switch (decoded.channels) {
        case 1: { // Grayscale: preserve directly as 1 channel
            out.channels = 1;
            out.pixels = decoded.pixels;
            break;
        }

        case 2: { // Grayscale + Alpha: preserve grayscale, discard alpha channel
            out.channels = 1;
            out.pixels.resize(num_pixels);
            for (std::uint64_t i = 0; i < num_pixels; ++i) {
                out.pixels[i] = decoded.pixels[2 * i]; // Copy Y, drop A
            }
            break;
        }

        case 3: { // RGB: preserve directly as 3 channels
            out.channels = 3;
            out.pixels = decoded.pixels;
            break;
        }

        case 4: { // RGBA: preserve RGB, discard alpha channel
            out.channels = 3;
            out.pixels.resize(num_pixels * 3);
            for (std::uint64_t i = 0; i < num_pixels; ++i) {
                out.pixels[3 * i]     = decoded.pixels[4 * i];     // R
                out.pixels[3 * i + 1] = decoded.pixels[4 * i + 1]; // G
                out.pixels[3 * i + 2] = decoded.pixels[4 * i + 2]; // B
                // decoded.pixels[4 * i + 3] (Alpha) is discarded
            }
            break;
        }

        default: {
            throw PreprocessingError("Unsupported channel count in DecodedImage: " +
                                     std::to_string(static_cast<int>(decoded.channels)));
        }
    }

    // 3. Validate resulting normalized Image invariants
    std::string out_err;
    if (!validate_image(out, &out_err)) {
        throw PreprocessingError("Normalized Image invariant validation failed: " + out_err);
    }

    return out;
}

} // namespace stegoscan
