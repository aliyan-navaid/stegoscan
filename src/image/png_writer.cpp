#include "image/png_writer.h"

namespace stegoscan {

void write_png(const std::filesystem::path& path, const Image& /*image*/) {
    // Placeholder stub implementation for interface scaffolding.
    // Functional implementation will be added in Module B2 step.
    throw PngWriteError("write_png is not yet implemented (scaffolding stub): " + path.string());
}

} // namespace stegoscan
