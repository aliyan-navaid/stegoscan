#include "image/image_loader.h"
#include "lodepng/lodepng.h"

#include <cctype>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace stegoscan {

DecodedImage load_png(const std::filesystem::path& path) {
    // 1. Check path existence and regular file status
    if (!std::filesystem::exists(path)) {
        throw PngLoadError("PNG file does not exist: " + path.string());
    }
    if (!std::filesystem::is_regular_file(path)) {
        throw PngLoadError("Path is not a regular file: " + path.string());
    }

    // 2. Extension check for user feedback
    const std::string ext = path.extension().string();
    std::string ext_lower;
    ext_lower.reserve(ext.size());
    for (char c : ext) {
        ext_lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    if (ext_lower != ".png") {
        throw PngLoadError("Invalid file extension: expected '.png', got '" + ext + "' for file: " + path.string());
    }

    // 3. Load entire file into memory buffer
    std::vector<unsigned char> file_bytes;
    const unsigned int read_err = lodepng::load_file(file_bytes, path.string());
    if (read_err != 0) {
        throw PngLoadError("Failed to read file " + path.string() + ": " + lodepng_error_text(read_err));
    }

    // 4. Validate PNG magic signature (must be at least 8 bytes)
    if (file_bytes.size() < 8) {
        throw PngLoadError("File is too small to be a valid PNG: " + path.string());
    }
    const unsigned char kPngSignature[8] = {137, 80, 78, 71, 13, 10, 26, 10};
    if (std::memcmp(file_bytes.data(), kPngSignature, 8) != 0) {
        throw PngLoadError("File does not contain a valid PNG signature: " + path.string());
    }

    // 5. Inspect PNG header without decoding pixel data
    LodePNGState state;
    lodepng_state_init(&state);

    unsigned int width = 0;
    unsigned int height = 0;
    const unsigned int inspect_err = lodepng_inspect(
        &width, &height, &state, file_bytes.data(), file_bytes.size());

    if (inspect_err != 0) {
        const std::string err_text = lodepng_error_text(inspect_err);
        lodepng_state_cleanup(&state);
        throw PngLoadError("Corrupt or invalid PNG header in " + path.string() + ": " + err_text);
    }

    if (width == 0 || height == 0) {
        lodepng_state_cleanup(&state);
        throw PngLoadError("Invalid PNG dimensions (" + std::to_string(width) + "x" +
                           std::to_string(height) + ") in " + path.string());
    }

    // 6. Enforce strictly 8-bit depth requirement before any decoder transformation
    const unsigned int original_bitdepth = state.info_png.color.bitdepth;
    if (original_bitdepth != 8) {
        lodepng_state_cleanup(&state);
        throw PngLoadError("Unsupported PNG bit depth: only 8-bit PNG images are accepted (file " +
                           path.string() + " has " + std::to_string(original_bitdepth) + "-bit depth)");
    }

    // 7. Validate PNG color type
    const LodePNGColorType colortype = state.info_png.color.colortype;
    if (colortype != LCT_GREY &&
        colortype != LCT_GREY_ALPHA &&
        colortype != LCT_RGB &&
        colortype != LCT_RGBA &&
        colortype != LCT_PALETTE) {
        lodepng_state_cleanup(&state);
        throw PngLoadError("Unsupported PNG color type (" + std::to_string(colortype) + ") in " + path.string());
    }

    // 8. Detect palette transparency (tRNS chunk presence in palette/indexed images)
    bool palette_has_transparency = false;
    if (colortype == LCT_PALETTE) {
        const unsigned char* trns_chunk = lodepng_chunk_find_const(
            file_bytes.data() + 8, file_bytes.data() + file_bytes.size(), "tRNS");
        if (trns_chunk != nullptr) {
            palette_has_transparency = true;
        }
    }

    // 9. Configure target raw color mode for decoding
    std::uint8_t out_channels = 0;
    if (colortype == LCT_GREY) {
        state.info_raw.colortype = LCT_GREY;
        state.info_raw.bitdepth = 8;
        out_channels = 1;
    } else if (colortype == LCT_GREY_ALPHA) {
        state.info_raw.colortype = LCT_GREY_ALPHA;
        state.info_raw.bitdepth = 8;
        out_channels = 2;
    } else if (colortype == LCT_RGB) {
        state.info_raw.colortype = LCT_RGB;
        state.info_raw.bitdepth = 8;
        out_channels = 3;
    } else if (colortype == LCT_RGBA) {
        state.info_raw.colortype = LCT_RGBA;
        state.info_raw.bitdepth = 8;
        out_channels = 4;
    } else if (colortype == LCT_PALETTE) {
        if (palette_has_transparency) {
            // Expand palette to RGBA when palette transparency metadata exists
            state.info_raw.colortype = LCT_RGBA;
            state.info_raw.bitdepth = 8;
            out_channels = 4;
        } else {
            // Expand palette to RGB when no transparency metadata exists
            state.info_raw.colortype = LCT_RGB;
            state.info_raw.bitdepth = 8;
            out_channels = 3;
        }
    }

    // 10. Decode pixels into raw buffer
    unsigned char* raw_pixels = nullptr;
    unsigned int decoded_w = 0;
    unsigned int decoded_h = 0;
    const unsigned int decode_err = lodepng_decode(
        &raw_pixels, &decoded_w, &decoded_h, &state, file_bytes.data(), file_bytes.size());

    if (decode_err != 0 || raw_pixels == nullptr) {
        const std::string err_text = lodepng_error_text(decode_err);
        if (raw_pixels != nullptr) {
            std::free(raw_pixels);
        }
        lodepng_state_cleanup(&state);
        throw PngLoadError("Failed to decode PNG pixels for " + path.string() + ": " + err_text);
    }

    // 11. Populate DecodedImage result
    DecodedImage result;
    result.width = decoded_w;
    result.height = decoded_h;
    result.channels = out_channels;

    const std::uint64_t total_bytes =
        static_cast<std::uint64_t>(decoded_w) * decoded_h * out_channels;
    result.pixels.assign(raw_pixels, raw_pixels + total_bytes);

    std::free(raw_pixels);
    lodepng_state_cleanup(&state);

    // 12. Invariant verification
    std::string val_err;
    if (!validate_decoded_image(result, &val_err)) {
        throw PngLoadError("DecodedImage invariant validation failed for " + path.string() + ": " + val_err);
    }

    return result;
}

} // namespace stegoscan
