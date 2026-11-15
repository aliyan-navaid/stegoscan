#ifndef STEGOSCAN_IMAGE_PNG_WRITER_H_
#define STEGOSCAN_IMAGE_PNG_WRITER_H_

#include "image/image.h"
#include <filesystem>
#include <stdexcept>
#include <string>

namespace stegoscan {

/**
 * @brief Exception thrown when PNG encoding or writing fails.
 */
class PngWriteError : public std::runtime_error {
public:
    explicit PngWriteError(const std::string& message) : std::runtime_error(message) {}
};

/**
 * @brief Encodes and writes a normalized Image to an 8-bit PNG file.
 *
 * Contract:
 * - Accepts only valid normalized Image objects with 1 channel (Grayscale) or 3 channels (RGB).
 * - Writes 8-bit grayscale or RGB PNG data without modifying pixel values.
 * - Does not add an alpha channel to the output PNG.
 * - Rejects invalid image buffers before encoding.
 * - Reports filesystem errors (e.g. non-existent directory, unwritable path) and encoder failures.
 * - Keeps third-party PNG library details private to the writer implementation.
 *
 * @param path Destination filesystem path for the output PNG.
 * @param image Valid normalized Image to write.
 * @throws PngWriteError if the image is invalid, the path is unwritable, or encoding fails.
 */
void write_png(const std::filesystem::path& path, const Image& image);

} // namespace stegoscan

#endif // STEGOSCAN_IMAGE_PNG_WRITER_H_
