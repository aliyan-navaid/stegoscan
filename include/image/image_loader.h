#ifndef STEGOSCAN_IMAGE_IMAGE_LOADER_H_
#define STEGOSCAN_IMAGE_IMAGE_LOADER_H_

#include "image/decoded_image.h"
#include <filesystem>
#include <stdexcept>
#include <string>

namespace stegoscan {

/**
 * @brief Exception thrown when PNG loading or validation fails.
 */
class PngLoadError : public std::runtime_error {
public:
    explicit PngLoadError(const std::string& message) : std::runtime_error(message) {}
};

/**
 * @brief Loads and decodes an 8-bit PNG file into a DecodedImage.
 *
 * Contract:
 * - Accepts PNG files only. Verifies the actual PNG file signature/content.
 * - Rejects non-PNG files clearly with PngLoadError.
 * - Rejects any input whose original PNG bit depth is not exactly 8 (rejects 16-bit PNG
 *   and 1-, 2-, 4-bit indexed/grayscale PNGs). Does NOT silently down-convert or up-convert bit depth.
 * - Supports 8-bit grayscale, RGB, RGBA, and palette/indexed PNGs.
 * - Automatically expands 8-bit palette/indexed images to RGB (or RGBA if transparency is present).
 * - For RGBA, grayscale+alpha, or palette transparency, preserves alpha in DecodedImage;
 *   alpha removal is delegated to the preprocessor.
 * - Does not perform steganalysis, resizing, sharpening, or filtering in the loader.
 * - Keeps third-party PNG library details private to the loader implementation.
 *
 * @param path Path to the PNG file.
 * @return DecodedImage containing raw 8-bit decoded pixel data and dimensions.
 * @throws PngLoadError if the file cannot be loaded, is malformed, or fails validation.
 */
DecodedImage load_png(const std::filesystem::path& path);

} // namespace stegoscan

#endif // STEGOSCAN_IMAGE_IMAGE_LOADER_H_
