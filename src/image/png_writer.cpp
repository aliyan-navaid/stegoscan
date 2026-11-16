#include "image/png_writer.h"
#include "lodepng/lodepng.h"

#include <cctype>
#include <cstdlib>
#include <string>

namespace stegoscan {

void write_png(const std::filesystem::path& path, const Image& image) {
    // 1. Validate the normalized Image object
    std::string val_err;
    if (!validate_image(image, &val_err)) {
        throw PngWriteError("Invalid image buffer passed to write_png: " + val_err);
    }

    if (image.channels != 1 && image.channels != 3) {
        throw PngWriteError("write_png only supports 1 channel (Grayscale) or 3 channels (RGB); got " +
                            std::to_string(static_cast<int>(image.channels)) + " channels");
    }

    // 2. Validate destination path and extension
    if (std::filesystem::is_directory(path)) {
        throw PngWriteError("Destination path is an existing directory, not a file: " + path.string());
    }

    if (path.has_parent_path() && !std::filesystem::exists(path.parent_path())) {
        throw PngWriteError("Destination directory does not exist: " + path.parent_path().string());
    }

    const std::string ext = path.extension().string();
    std::string ext_lower;
    ext_lower.reserve(ext.size());
    for (char c : ext) {
        ext_lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    if (ext_lower != ".png") {
        throw PngWriteError("Output file path must have a '.png' extension (got '" + ext + "'): " + path.string());
    }

    // 3. Configure LodePNG encoder for exact 8-bit output without alpha
    LodePNGState state;
    lodepng_state_init(&state);

    if (image.channels == 1) {
        state.info_raw.colortype = LCT_GREY;
        state.info_raw.bitdepth = 8;
        state.info_png.color.colortype = LCT_GREY;
        state.info_png.color.bitdepth = 8;
    } else {
        state.info_raw.colortype = LCT_RGB;
        state.info_raw.bitdepth = 8;
        state.info_png.color.colortype = LCT_RGB;
        state.info_png.color.bitdepth = 8;
    }

    // Disable auto-conversion to enforce exact 8-bit grayscale or RGB output without alpha
    state.encoder.auto_convert = 0;

    unsigned char* png_bytes = nullptr;
    size_t png_size = 0;
    const unsigned int encode_err = lodepng_encode(
        &png_bytes, &png_size, image.pixels.data(), image.width, image.height, &state);

    lodepng_state_cleanup(&state);

    if (encode_err != 0 || png_bytes == nullptr) {
        if (png_bytes != nullptr) {
            std::free(png_bytes);
        }
        throw PngWriteError("PNG encoding failed for " + path.string() + ": " + lodepng_error_text(encode_err));
    }

    // 4. Save encoded PNG bytes to disk
    const unsigned int save_err = lodepng_save_file(png_bytes, png_size, path.string().c_str());
    std::free(png_bytes);

    if (save_err != 0) {
        throw PngWriteError("Failed to write PNG file to disk at " + path.string() + ": " +
                            lodepng_error_text(save_err));
    }
}

} // namespace stegoscan
