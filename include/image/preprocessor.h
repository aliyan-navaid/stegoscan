#ifndef STEGOSCAN_IMAGE_PREPROCESSOR_H_
#define STEGOSCAN_IMAGE_PREPROCESSOR_H_

#include "image/decoded_image.h"
#include "image/image.h"
#include <stdexcept>
#include <string>

namespace stegoscan {

/**
 * @brief Exception thrown when preprocessing or normalization fails.
 */
class PreprocessingError : public std::runtime_error {
public:
    explicit PreprocessingError(const std::string& message) : std::runtime_error(message) {}
};

/**
 * @brief Normalizes a DecodedImage into the canonical Image representation.
 *
 * Contract:
 * - Grayscale (1 channel): preserved as 1 channel.
 * - Grayscale + Alpha (2 channels): preserved as 1 channel (grayscale); alpha channel is discarded.
 * - RGB (3 channels): preserved as 3 channels (RGB).
 * - RGBA (4 channels): preserved as 3 channels (RGB); alpha channel is discarded.
 * - Rejects invalid decoded images (invalid dimensions, unsupported channel count, buffer length mismatch).
 * - Does not alter color/grayscale sample values.
 * - Does not invent pixel values or attempt to repair invalid buffers.
 *
 * Design rationale: Alpha channels are ignored because the pairwise chi-square test
 * models physical color intensity sample pairs, not transparency metadata.
 *
 * @param decoded Raw decoded image from decoder.
 * @return Normalized Image with exactly 1 or 3 channels.
 * @throws PreprocessingError if decoded image is invalid or cannot be normalized.
 */
Image preprocess_image(const DecodedImage& decoded);

} // namespace stegoscan

#endif // STEGOSCAN_IMAGE_PREPROCESSOR_H_
