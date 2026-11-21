#include "evaluation/synthetic_dataset_generator.h"
#include "evaluation/lsb_embedding.h"

#include "image/image.h"
#include "image/png_writer.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

// NOTE: This translation unit intentionally does not use OpenMP. Dataset generation must be
// strictly deterministic and reproducible.

namespace stegoscan {

namespace {

// Upper bound on samples (width * height * 3) per generated image. Bounds the memory used by
// the without-replacement position shuffle and keeps every index inside uint32_t.
constexpr std::uint64_t kMaxSamplesPerImage = 1ULL << 31;

// ---------------------------------------------------------------------------------------
// Small platform-independent PRNG (SplitMix64). The standard <random> distributions are not
// guaranteed to give identical sequences across standard libraries, so we avoid them to make
// "same seed => same dataset" hold everywhere.
// ---------------------------------------------------------------------------------------
class SplitMix64 {
public:
    explicit SplitMix64(std::uint64_t seed) : state_(seed) {}

    std::uint64_t next() {
        state_ += 0x9E3779B97F4A7C15ULL;
        std::uint64_t z = state_;
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return z ^ (z >> 31);
    }

    // Unbiased integer in [0, n). Requires n > 0.
    std::uint64_t bounded(std::uint64_t n) {
        const std::uint64_t threshold = (0 - n) % n; // 2^64 mod n
        for (;;) {
            const std::uint64_t r = next();
            if (r >= threshold) {
                return r % n;
            }
        }
    }

private:
    std::uint64_t state_;
};

std::uint64_t mix_seed(std::uint64_t base, std::uint64_t a, std::uint64_t b = 0) {
    SplitMix64 rng(base);
    std::uint64_t x = rng.next();
    x ^= a * 0xD6E8FEB86659FD93ULL;
    SplitMix64 rng2(x);
    x = rng2.next();
    x ^= b * 0xA0761D6478BD642FULL;
    SplitMix64 rng3(x);
    return rng3.next();
}

std::uint8_t clamp_u8(std::int64_t v) {
    return static_cast<std::uint8_t>(std::min<std::int64_t>(255, std::max<std::int64_t>(0, v)));
}

// ---------------------------------------------------------------------------------------
// Cover patterns. Every pattern adds seeded per-pixel randomness so the clean histograms show
// natural sampling variation between neighbouring even/odd bins instead of being perfectly
// regular (a perfectly regular histogram is indistinguishable from an LSB-equalized one).
// ---------------------------------------------------------------------------------------
enum class Pattern { Gradient, Checker, Noise };

const char* pattern_name(Pattern p) {
    switch (p) {
        case Pattern::Gradient: return "gradient";
        case Pattern::Checker: return "checker";
        case Pattern::Noise: return "noise";
    }
    return "unknown";
}

Image make_cover(Pattern pattern, std::uint8_t channels, std::uint32_t width,
                 std::uint32_t height, std::uint64_t seed) {
    Image img;
    img.width = width;
    img.height = height;
    img.channels = channels;
    img.pixels.resize(static_cast<std::size_t>(width) * height * channels);

    SplitMix64 rng(seed);
    const std::int64_t span_x = std::max<std::int64_t>(1, static_cast<std::int64_t>(width) - 1);
    const std::int64_t span_y = std::max<std::int64_t>(1, static_cast<std::int64_t>(height) - 1);
    constexpr std::uint32_t kBlock = 16;

    std::size_t i = 0;
    for (std::uint32_t y = 0; y < height; ++y) {
        for (std::uint32_t x = 0; x < width; ++x) {
            for (std::uint8_t c = 0; c < channels; ++c, ++i) {
                std::int64_t v = 0;
                switch (pattern) {
                    case Pattern::Gradient: {
                        // Diagonal ramp over 20..235, phase-shifted per channel, +/-8 noise.
                        const std::int64_t t = (static_cast<std::int64_t>(x) * span_y +
                                                static_cast<std::int64_t>(y) * span_x);
                        const std::int64_t denom = 2 * span_x * span_y;
                        std::int64_t frac_1000 = (t * 1000) / denom;           // 0..1000
                        frac_1000 = (frac_1000 + 333 * static_cast<std::int64_t>(c)) % 1001;
                        v = 20 + (frac_1000 * 215) / 1000;
                        v += static_cast<std::int64_t>(rng.bounded(17)) - 8;
                        break;
                    }
                    case Pattern::Checker: {
                        const bool odd = (((x / kBlock) + (y / kBlock) + c) % 2) != 0;
                        v = odd ? 192 : 64;
                        v += static_cast<std::int64_t>(rng.bounded(25)) - 12;
                        break;
                    }
                    case Pattern::Noise: {
                        // Sum of four uniforms: bell-shaped around ~126, range 0..252.
                        for (int k = 0; k < 4; ++k) {
                            v += static_cast<std::int64_t>(rng.bounded(64));
                        }
                        break;
                    }
                }
                img.pixels[i] = clamp_u8(v);
            }
        }
    }
    return img;
}

// Rate -> filename-safe token, e.g. 0.1 -> "0p1", 1.0 -> "1".
std::string rate_token(double rate) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.6f", rate);
    std::string s(buf);
    while (!s.empty() && s.back() == '0') s.pop_back();
    if (!s.empty() && s.back() == '.') s.pop_back();
    std::replace(s.begin(), s.end(), '.', 'p');
    return s;
}

std::string format_rate(double rate) {
    std::ostringstream os;
    os.precision(10);
    os << rate;
    return os.str();
}

bool valid_rate(double rate) {
    return std::isfinite(rate) && rate > 0.0 && rate <= 1.0;
}

void validate_config(const SyntheticDatasetConfig& config) {
    if (config.output_directory.empty()) {
        throw std::invalid_argument("SyntheticDatasetConfig: output_directory must not be empty");
    }
    if (config.width == 0 || config.height == 0) {
        throw std::invalid_argument("SyntheticDatasetConfig: width and height must be > 0");
    }
    const std::uint64_t samples =
        static_cast<std::uint64_t>(config.width) * config.height * 3;
    if (samples > kMaxSamplesPerImage) {
        throw std::invalid_argument("SyntheticDatasetConfig: image is too large for the generator");
    }
    if (config.payload_rates.empty()) {
        throw std::invalid_argument("SyntheticDatasetConfig: at least one payload rate is required");
    }
    std::set<std::string> tokens;
    for (double rate : config.payload_rates) {
        if (!valid_rate(rate)) {
            throw std::invalid_argument(
                "SyntheticDatasetConfig: payload rates must be finite and satisfy 0 < rate <= 1");
        }
        if (!tokens.insert(rate_token(rate)).second) {
            throw std::invalid_argument(
                "SyntheticDatasetConfig: duplicate payload rates (they would share file names)");
        }
        // Smallest image is grayscale (width * height samples); fail before writing any file.
        const double gray_samples = static_cast<double>(config.width) * config.height;
        if (std::llround(rate * gray_samples) < 1) {
            throw std::invalid_argument(
                "SyntheticDatasetConfig: a payload rate selects zero samples for this image size");
        }
    }
}

std::string csv_escape(const std::string& field) {
    if (field.find_first_of(",\"\n\r") == std::string::npos) return field;
    std::string out = "\"";
    for (char ch : field) {
        if (ch == '"') out += '"';
        out += ch;
    }
    out += '"';
    return out;
}

void write_manifest(const std::filesystem::path& path,
                    const std::vector<DatasetManifestEntry>& entries) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
        throw std::runtime_error("Cannot open manifest for writing: " + path.string());
    }
    out << "path,label,source_cover,payload_rate,seed,pattern\n";
    for (const auto& e : entries) {
        out << csv_escape(e.relative_path) << ',' << e.label << ','
            << csv_escape(e.source_cover) << ',' << format_rate(e.payload_rate) << ','
            << e.seed << ',' << e.pattern << '\n';
    }
    out.flush();
    if (!out) {
        throw std::runtime_error("Failed while writing manifest: " + path.string());
    }
}

} // namespace

LsbEmbeddingResult embed_lsb_replacement(const Image& cover, double payload_rate,
                                         std::uint64_t seed) {
    std::string err;
    if (!validate_image(cover, &err)) {
        throw std::invalid_argument("embed_lsb_replacement: invalid cover image: " + err);
    }
    if (!valid_rate(payload_rate)) {
        throw std::invalid_argument(
            "embed_lsb_replacement: payload rate must be finite and satisfy 0 < rate <= 1");
    }
    const std::uint64_t total = static_cast<std::uint64_t>(cover.pixels.size());
    if (total > kMaxSamplesPerImage) {
        throw std::invalid_argument("embed_lsb_replacement: image is too large");
    }
    const std::uint64_t count =
        static_cast<std::uint64_t>(std::llround(payload_rate * static_cast<double>(total)));
    if (count == 0) {
        throw std::invalid_argument(
            "embed_lsb_replacement: payload rate selects zero samples for this image size");
    }

    SplitMix64 rng(seed);

    // Partial Fisher-Yates: the first `count` entries are a uniform sample without replacement.
    std::vector<std::uint32_t> order(static_cast<std::size_t>(total));
    for (std::uint64_t i = 0; i < total; ++i) order[i] = static_cast<std::uint32_t>(i);

    LsbEmbeddingResult result;
    result.stego = cover; // copy; the cover is never touched
    result.positions.reserve(count);
    result.bits.reserve(count);

    for (std::uint64_t i = 0; i < count; ++i) {
        const std::uint64_t j = i + rng.bounded(total - i);
        std::swap(order[i], order[j]);
        const std::uint64_t pos = order[i];
        const std::uint8_t bit = static_cast<std::uint8_t>(rng.next() >> 63);
        result.stego.pixels[pos] = static_cast<std::uint8_t>((cover.pixels[pos] & 0xFE) | bit);
        result.positions.push_back(pos);
        result.bits.push_back(bit);
    }
    return result;
}

void generate_synthetic_dataset(const SyntheticDatasetConfig& config) {
    validate_config(config);

    namespace fs = std::filesystem;
    const fs::path clean_dir = config.output_directory / "clean";
    const fs::path stego_dir = config.output_directory / "stego";
    std::error_code ec;
    fs::create_directories(clean_dir, ec);
    if (ec) {
        throw std::runtime_error("Cannot create directory " + clean_dir.string() + ": " + ec.message());
    }
    fs::create_directories(stego_dir, ec);
    if (ec) {
        throw std::runtime_error("Cannot create directory " + stego_dir.string() + ": " + ec.message());
    }

    const Pattern patterns[] = {Pattern::Gradient, Pattern::Checker, Pattern::Noise};
    std::vector<DatasetManifestEntry> entries;

    std::uint64_t cover_index = 0;
    for (Pattern pattern : patterns) {
        for (std::uint8_t channels : {static_cast<std::uint8_t>(1), static_cast<std::uint8_t>(3)}) {
            const std::string stem =
                std::string(pattern_name(pattern)) + (channels == 1 ? "_gray" : "_rgb");
            const std::uint64_t cover_seed = mix_seed(config.seed, cover_index);

            const Image cover = make_cover(pattern, channels, config.width, config.height, cover_seed);

            const std::string cover_rel = "clean/" + stem + ".png";
            write_png(config.output_directory / cover_rel, cover);
            entries.push_back({cover_rel, "clean", "", 0.0, config.seed, pattern_name(pattern)});

            std::uint64_t rate_index = 0;
            for (double rate : config.payload_rates) {
                const std::uint64_t embed_seed = mix_seed(config.seed, cover_index, rate_index + 1);
                const LsbEmbeddingResult stego = embed_lsb_replacement(cover, rate, embed_seed);

                const std::string stego_rel = "stego/" + stem + "_lsb_" + rate_token(rate) + ".png";
                write_png(config.output_directory / stego_rel, stego.stego);
                entries.push_back({stego_rel, "stego", cover_rel, rate, config.seed,
                                   pattern_name(pattern)});
                ++rate_index;
            }
            ++cover_index;
        }
    }

    write_manifest(config.output_directory / "manifest.csv", entries);
}

} // namespace stegoscan
