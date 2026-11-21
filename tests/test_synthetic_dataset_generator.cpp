#include "test_harness.h"

#include "evaluation/lsb_embedding.h"
#include "evaluation/synthetic_dataset_generator.h"
#include "image/image_loader.h"
#include "image/preprocessor.h"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace {

namespace fs = std::filesystem;

struct FixtureDirectory {
    fs::path dir;
    explicit FixtureDirectory(const std::string& name) {
        dir = fs::temp_directory_path() / name;
        std::error_code ec;
        fs::remove_all(dir, ec);
        fs::create_directories(dir);
    }
    ~FixtureDirectory() {
        std::error_code ec;
        fs::remove_all(dir, ec);
    }
};

struct ManifestRow {
    std::string path, label, source_cover, payload_rate, seed, pattern;
};

std::vector<ManifestRow> read_manifest(const fs::path& manifest) {
    std::ifstream in(manifest);
    std::string line;
    std::getline(in, line); // header
    std::vector<ManifestRow> rows;
    while (std::getline(in, line)) {
        // Split on every comma so an empty source_cover cell is preserved.
        std::vector<std::string> cells;
        std::size_t start = 0;
        for (std::size_t i = 0; i <= line.size(); ++i) {
            if (i == line.size() || line[i] == ',') {
                cells.push_back(line.substr(start, i - start));
                start = i + 1;
            }
        }
        rows.push_back({cells.at(0), cells.at(1), cells.at(2), cells.at(3), cells.at(4), cells.at(5)});
    }
    return rows;
}

std::string slurp(const fs::path& p) {
    std::ifstream in(p, std::ios::binary);
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

stegoscan::Image load_normalized(const fs::path& p) {
    return stegoscan::preprocess_image(stegoscan::load_png(p));
}

stegoscan::Image make_test_cover(std::uint32_t w, std::uint32_t h, std::uint8_t channels) {
    stegoscan::Image img;
    img.width = w;
    img.height = h;
    img.channels = channels;
    img.pixels.resize(static_cast<std::size_t>(w) * h * channels);
    for (std::size_t i = 0; i < img.pixels.size(); ++i) {
        img.pixels[i] = static_cast<std::uint8_t>((i * 37 + 11) % 256);
    }
    return img;
}

stegoscan::SyntheticDatasetConfig small_config(const fs::path& out) {
    stegoscan::SyntheticDatasetConfig c;
    c.output_directory = out;
    c.width = 48;
    c.height = 40;
    c.seed = 777;
    c.payload_rates = {0.25, 1.0};
    return c;
}

} // namespace

// ---------------------------------------------------------------------------------------
// embed_lsb_replacement
// ---------------------------------------------------------------------------------------

TEST_CASE(LsbEmbedding, SelectedCountMatchesRoundedRate) {
    const auto cover = make_test_cover(13, 7, 3); // 273 samples
    const double rates[] = {0.10, 0.25, 0.50, 0.99, 1.00};
    for (double rate : rates) {
        const auto r = stegoscan::embed_lsb_replacement(cover, rate, 42);
        const auto expected = static_cast<std::uint64_t>(std::llround(rate * 273.0));
        ASSERT_EQ(r.positions.size(), expected);
        ASSERT_EQ(r.bits.size(), expected);
    }
}

TEST_CASE(LsbEmbedding, PositionsDistinctAndInRange) {
    const auto cover = make_test_cover(16, 16, 3);
    const auto r = stegoscan::embed_lsb_replacement(cover, 0.5, 9);
    std::set<std::uint64_t> seen(r.positions.begin(), r.positions.end());
    ASSERT_EQ(seen.size(), r.positions.size());
    for (auto p : r.positions) ASSERT_TRUE(p < cover.pixels.size());
}

TEST_CASE(LsbEmbedding, OnlySelectedSamplesChangeAndOnlyByLsb) {
    const auto cover = make_test_cover(20, 11, 3);
    const auto copy_before = cover;
    const auto r = stegoscan::embed_lsb_replacement(cover, 0.3, 5);

    // Cover must be untouched.
    ASSERT_TRUE(cover.pixels == copy_before.pixels);
    ASSERT_EQ(r.stego.width, cover.width);
    ASSERT_EQ(r.stego.height, cover.height);
    ASSERT_EQ(static_cast<int>(r.stego.channels), static_cast<int>(cover.channels));

    std::set<std::uint64_t> selected(r.positions.begin(), r.positions.end());
    for (std::size_t i = 0; i < r.positions.size(); ++i) {
        const auto pos = r.positions[i];
        const std::uint8_t expected = static_cast<std::uint8_t>((cover.pixels[pos] & 0xFE) | r.bits[i]);
        ASSERT_EQ(r.stego.pixels[pos], expected);
        ASSERT_TRUE(r.bits[i] <= 1);
    }
    for (std::size_t i = 0; i < cover.pixels.size(); ++i) {
        const int diff = std::abs(static_cast<int>(r.stego.pixels[i]) - static_cast<int>(cover.pixels[i]));
        ASSERT_TRUE(diff <= 1);
        if (selected.count(i) == 0) {
            ASSERT_EQ(r.stego.pixels[i], cover.pixels[i]);
        }
    }
}

TEST_CASE(LsbEmbedding, FullRateSetsEveryLsbToPayloadBit) {
    const auto cover = make_test_cover(9, 9, 1);
    const auto r = stegoscan::embed_lsb_replacement(cover, 1.0, 3);
    ASSERT_EQ(r.positions.size(), cover.pixels.size());
    for (std::size_t i = 0; i < r.positions.size(); ++i) {
        ASSERT_EQ(r.stego.pixels[r.positions[i]] & 1, static_cast<int>(r.bits[i]));
        ASSERT_EQ(r.stego.pixels[r.positions[i]] & 0xFE, cover.pixels[r.positions[i]] & 0xFE);
    }
}

TEST_CASE(LsbEmbedding, DeterministicForSameSeedAndDiffersForOtherSeed) {
    const auto cover = make_test_cover(32, 32, 1);
    const auto a = stegoscan::embed_lsb_replacement(cover, 0.4, 100);
    const auto b = stegoscan::embed_lsb_replacement(cover, 0.4, 100);
    const auto c = stegoscan::embed_lsb_replacement(cover, 0.4, 101);
    ASSERT_TRUE(a.stego.pixels == b.stego.pixels);
    ASSERT_TRUE(a.positions == b.positions);
    ASSERT_TRUE(a.bits == b.bits);
    ASSERT_TRUE(a.positions != c.positions);
}

TEST_CASE(LsbEmbedding, PayloadBitsAreRoughlyBalanced) {
    const auto cover = make_test_cover(64, 64, 1); // 4096 samples
    const auto r = stegoscan::embed_lsb_replacement(cover, 1.0, 1);
    std::size_t ones = 0;
    for (auto b : r.bits) ones += b;
    // Binomial(4096, 0.5): sigma = 32; allow 6 sigma.
    ASSERT_TRUE(ones > 2048 - 192 && ones < 2048 + 192);
}

TEST_CASE(LsbEmbedding, RejectsInvalidRates) {
    const auto cover = make_test_cover(8, 8, 1);
    const double bad[] = {0.0, -0.1, 1.0000001, 2.0,
                          std::numeric_limits<double>::quiet_NaN(),
                          std::numeric_limits<double>::infinity(),
                          -std::numeric_limits<double>::infinity()};
    for (double rate : bad) {
        ASSERT_THROW(stegoscan::embed_lsb_replacement(cover, rate, 1), std::invalid_argument);
    }
}

TEST_CASE(LsbEmbedding, RejectsRateThatSelectsNothingAndInvalidCover) {
    const auto cover = make_test_cover(4, 4, 1); // 16 samples
    ASSERT_THROW(stegoscan::embed_lsb_replacement(cover, 0.001, 1), std::invalid_argument);

    stegoscan::Image bad;
    bad.width = 4;
    bad.height = 4;
    bad.channels = 1;
    bad.pixels.resize(5); // wrong size
    ASSERT_THROW(stegoscan::embed_lsb_replacement(bad, 0.5, 1), std::invalid_argument);
}

// ---------------------------------------------------------------------------------------
// generate_synthetic_dataset
// ---------------------------------------------------------------------------------------

TEST_CASE(DatasetGenerator, ProducesLabeledManifestAndLoadablePngs) {
    FixtureDirectory fx("stegoscan_gen_basic");
    const auto cfg = small_config(fx.dir / "ds");
    ASSERT_NO_THROW(stegoscan::generate_synthetic_dataset(cfg));

    const auto manifest = cfg.output_directory / "manifest.csv";
    ASSERT_TRUE(fs::exists(manifest));
    const auto rows = read_manifest(manifest);

    // 3 patterns x {gray, rgb} = 6 covers, each with 2 payload rates.
    const std::size_t covers = 6;
    ASSERT_EQ(rows.size(), covers * (1 + cfg.payload_rates.size()));

    std::size_t clean = 0, stego = 0, gray = 0, rgb = 0;
    std::set<std::string> patterns;
    for (const auto& row : rows) {
        // Relative paths resolve against the manifest directory.
        ASSERT_FALSE(fs::path(row.path).is_absolute());
        const auto full = manifest.parent_path() / row.path;
        ASSERT_TRUE(fs::exists(full));

        const auto img = load_normalized(full);
        ASSERT_EQ(img.width, cfg.width);
        ASSERT_EQ(img.height, cfg.height);
        ASSERT_TRUE(img.channels == 1 || img.channels == 3);
        (img.channels == 1 ? gray : rgb)++;
        patterns.insert(row.pattern);

        if (row.label == "clean") {
            ++clean;
            ASSERT_TRUE(row.source_cover.empty());
            ASSERT_TRUE(row.path.rfind("clean/", 0) == 0);
        } else {
            ASSERT_EQ(row.label, std::string("stego"));
            ++stego;
            ASSERT_FALSE(row.source_cover.empty());
            ASSERT_TRUE(fs::exists(manifest.parent_path() / row.source_cover));
            ASSERT_TRUE(row.path.rfind("stego/", 0) == 0);
        }
        ASSERT_EQ(row.seed, std::to_string(cfg.seed));
    }
    ASSERT_EQ(clean, covers);
    ASSERT_EQ(stego, covers * cfg.payload_rates.size());
    ASSERT_EQ(gray, rgb); // equal numbers of grayscale and RGB images
    ASSERT_EQ(patterns.size(), static_cast<std::size_t>(3));
}

TEST_CASE(DatasetGenerator, StegoDiffersFromItsCoverByLsbOnly) {
    FixtureDirectory fx("stegoscan_gen_lsb");
    auto cfg = small_config(fx.dir / "ds");
    ASSERT_NO_THROW(stegoscan::generate_synthetic_dataset(cfg));
    const auto manifest = cfg.output_directory / "manifest.csv";

    for (const auto& row : read_manifest(manifest)) {
        if (row.label != "stego") continue;
        const auto stego = load_normalized(manifest.parent_path() / row.path);
        const auto cover = load_normalized(manifest.parent_path() / row.source_cover);
        ASSERT_EQ(stego.pixels.size(), cover.pixels.size());

        const double rate = std::stod(row.payload_rate);
        const double selected = std::llround(rate * static_cast<double>(cover.pixels.size()));
        std::size_t changed = 0;
        for (std::size_t i = 0; i < cover.pixels.size(); ++i) {
            const int d = std::abs(static_cast<int>(stego.pixels[i]) - static_cast<int>(cover.pixels[i]));
            ASSERT_TRUE(d <= 1);
            changed += (d == 1);
        }
        // About half of the selected samples already had the payload bit; allow 6 sigma.
        const double sigma = std::sqrt(selected * 0.25);
        ASSERT_TRUE(static_cast<double>(changed) <= selected);
        ASSERT_TRUE(std::abs(static_cast<double>(changed) - selected / 2.0) <= 6.0 * sigma + 1.0);
    }
}

TEST_CASE(DatasetGenerator, SameConfigProducesIdenticalDataset) {
    FixtureDirectory fx("stegoscan_gen_repro");
    const auto a = small_config(fx.dir / "a");
    const auto b = small_config(fx.dir / "b");
    ASSERT_NO_THROW(stegoscan::generate_synthetic_dataset(a));
    ASSERT_NO_THROW(stegoscan::generate_synthetic_dataset(b));

    // Manifests contain only relative paths, so they must be byte-identical.
    ASSERT_EQ(slurp(a.output_directory / "manifest.csv"), slurp(b.output_directory / "manifest.csv"));

    for (const auto& row : read_manifest(a.output_directory / "manifest.csv")) {
        const auto ia = load_normalized(a.output_directory / row.path);
        const auto ib = load_normalized(b.output_directory / row.path);
        ASSERT_TRUE(ia.pixels == ib.pixels);
    }
}

TEST_CASE(DatasetGenerator, DifferentSeedChangesRandomData) {
    FixtureDirectory fx("stegoscan_gen_seed");
    auto a = small_config(fx.dir / "a");
    auto b = small_config(fx.dir / "b");
    b.seed = a.seed + 1;
    ASSERT_NO_THROW(stegoscan::generate_synthetic_dataset(a));
    ASSERT_NO_THROW(stegoscan::generate_synthetic_dataset(b));

    const auto ca = load_normalized(a.output_directory / "clean/noise_gray.png");
    const auto cb = load_normalized(b.output_directory / "clean/noise_gray.png");
    ASSERT_TRUE(ca.pixels != cb.pixels);

    const auto sa = load_normalized(a.output_directory / "stego/noise_gray_lsb_1.png");
    const auto sb = load_normalized(b.output_directory / "stego/noise_gray_lsb_1.png");
    ASSERT_TRUE(sa.pixels != sb.pixels);
}

TEST_CASE(DatasetGenerator, CoversAreNotConstantAndUseManyIntensities) {
    FixtureDirectory fx("stegoscan_gen_cover_quality");
    const auto cfg = small_config(fx.dir / "ds");
    ASSERT_NO_THROW(stegoscan::generate_synthetic_dataset(cfg));
    for (const auto& row : read_manifest(cfg.output_directory / "manifest.csv")) {
        if (row.label != "clean") continue;
        const auto img = load_normalized(cfg.output_directory / row.path);
        std::set<std::uint8_t> distinct(img.pixels.begin(), img.pixels.end());
        ASSERT_TRUE(distinct.size() > 8);
    }
}

TEST_CASE(DatasetGenerator, RejectsInvalidConfiguration) {
    FixtureDirectory fx("stegoscan_gen_invalid");
    const auto base = small_config(fx.dir / "ds");

    auto cfg = base;
    cfg.width = 0;
    ASSERT_THROW(stegoscan::generate_synthetic_dataset(cfg), std::invalid_argument);

    cfg = base;
    cfg.height = 0;
    ASSERT_THROW(stegoscan::generate_synthetic_dataset(cfg), std::invalid_argument);

    cfg = base;
    cfg.output_directory.clear();
    ASSERT_THROW(stegoscan::generate_synthetic_dataset(cfg), std::invalid_argument);

    cfg = base;
    cfg.payload_rates.clear();
    ASSERT_THROW(stegoscan::generate_synthetic_dataset(cfg), std::invalid_argument);

    cfg = base;
    cfg.payload_rates = {0.5, 0.5};
    ASSERT_THROW(stegoscan::generate_synthetic_dataset(cfg), std::invalid_argument);

    const double bad[] = {0.0, -0.5, 1.5, std::numeric_limits<double>::quiet_NaN(),
                          std::numeric_limits<double>::infinity()};
    for (double rate : bad) {
        cfg = base;
        cfg.payload_rates = {0.25, rate};
        ASSERT_THROW(stegoscan::generate_synthetic_dataset(cfg), std::invalid_argument);
    }

    // Validation must happen before anything is written.
    ASSERT_FALSE(fs::exists(base.output_directory));
}

TEST_CASE(DatasetGenerator, ReportsFilesystemFailure) {
    FixtureDirectory fx("stegoscan_gen_fs_fail");
    // Make the output "directory" a regular file so directory creation must fail.
    const auto blocker = fx.dir / "blocker";
    { std::ofstream(blocker) << "x"; }
    auto cfg = small_config(blocker);
    ASSERT_THROW(stegoscan::generate_synthetic_dataset(cfg), std::runtime_error);
}
