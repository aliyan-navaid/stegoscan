# Stegoscan Interface Contracts and API Reference

This document defines the formal interfaces, data types, parameter contracts, and error-handling conventions for every module in `stegoscan`.

---

## 1. Global Conventions

- **Language Standard:** C++17.
- **Namespace:** All public types and functions reside in the `stegoscan` namespace.
- **Exception Policy:**
  - Exceptional failures (I/O failures, corrupt files, unexpected memory allocations) throw standard exceptions or module-specific derivations of `std::runtime_error`.
  - Invalid caller arguments (null ranges, out-of-bounds parameters) throw `std::invalid_argument`.
  - Invariant validation helpers return boolean status with an optional error output string for non-throwing validation checks.
- **Const Correctness:** All input parameters representing buffers, models, or configurations are passed by `const &`.
- **Memory Management:** Zero raw memory ownership; pixel buffers use `std::vector<std::uint8_t>`.

---

## 2. Core Image Interfaces (`include/image/`)

### 2.1 `decoded_image.h`

```cpp
namespace stegoscan {

struct DecodedImage {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint8_t channels = 0; // 1, 2, 3, or 4
    std::vector<std::uint8_t> pixels;
};

bool validate_decoded_image(const DecodedImage& image, std::string* error_msg = nullptr);

} // namespace stegoscan
```

- **Invariants:**
  - `width > 0` and `height > 0`
  - `channels` $\in \{1, 2, 3, 4\}$
  - `pixels.size() == static_cast<uint64_t>(width) * height * channels` calculated safely against 32-bit overflow.

---

### 2.2 `image.h`

```cpp
namespace stegoscan {

struct Image {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint8_t channels = 0; // 1 (Grayscale) or 3 (RGB)
    std::vector<std::uint8_t> pixels;
};

bool validate_image(const Image& image, std::string* error_msg = nullptr);

} // namespace stegoscan
```

- **Invariants:**
  - `channels` $\in \{1, 3\}$ (Normalized image never contains alpha).
  - Storage is interleaved row-major: $R, G, B, R, G, B, \dots$

---

### 2.3 `image_loader.h`

```cpp
namespace stegoscan {

class PngLoadError : public std::runtime_error;

DecodedImage load_png(const std::filesystem::path& path);

} // namespace stegoscan
```

- **Inputs:** Filesystem path to PNG.
- **Outputs:** `DecodedImage` with 1, 2, 3, or 4 channels.
- **Contract:**
  - Rejects non-PNG files by checking signature.
  - Rejects bit depths other than 8 (rejects 16-bit PNG, 1/2/4-bit indexed/gray).
  - Automatically expands indexed/palette images to RGB/RGBA.
  - Preserves alpha channel in `DecodedImage` (leaves removal to preprocessor).
- **Errors:** Throws `PngLoadError` on file missing, unreadable, corrupt, or unsupported bit depth.

---

### 2.4 `png_writer.h`

```cpp
namespace stegoscan {

class PngWriteError : public std::runtime_error;

void write_png(const std::filesystem::path& path, const Image& image);

} // namespace stegoscan
```

- **Inputs:** Destination filesystem path, valid normalized `Image` (1 or 3 channels).
- **Outputs:** None (`void`). Creates 8-bit PNG file on disk.
- **Contract:**
  - Accepts only 1 or 3 channel images. Does not add alpha.
- **Errors:** Throws `PngWriteError` if image is invalid, output directory is invalid, or encoding fails.

---

### 2.5 `preprocessor.h`

```cpp
namespace stegoscan {

class PreprocessingError : public std::runtime_error;

Image preprocess_image(const DecodedImage& decoded);

} // namespace stegoscan
```

- **Inputs:** Valid `DecodedImage`.
- **Outputs:** Canonical `Image` (1 or 3 channels).
- **Normalization Policy:**
  - 1 channel (Grayscale) $\to$ 1 channel `Image`.
  - 2 channels (Grayscale + Alpha) $\to$ 1 channel `Image` (alpha discarded).
  - 3 channels (RGB) $\to$ 3 channels `Image`.
  - 4 channels (RGBA) $\to$ 3 channels `Image` (alpha discarded).
- **Errors:** Throws `PreprocessingError` if buffer sizes or channel counts are invalid.

---

## 3. Analysis Interfaces (`include/analysis/`)

### 3.1 `histogram.h`

```cpp
namespace stegoscan {

constexpr std::size_t kHistogramBins = 256;
constexpr std::size_t kMaxColorChannels = 3;

using ChannelHistogram = std::array<std::uint64_t, kHistogramBins>;

struct ThreadWorkInfo {
    int thread_id = 0;
    std::uint64_t rows_processed = 0;
    std::uint32_t first_row = 0;
    std::uint32_t last_row = 0;
};

struct HistogramResult {
    std::array<ChannelHistogram, kMaxColorChannels> channels{};
    std::uint8_t channel_count = 0;
    std::uint64_t samples_per_channel = 0;
    int actual_threads = 1;
    std::vector<ThreadWorkInfo> thread_work;
};

bool are_histograms_equal(const HistogramResult& a, const HistogramResult& b);

} // namespace stegoscan
```

- **Comparison Rule:** Compares `channel_count`, `samples_per_channel`, and all 256 bins across active channels. Ignores execution diagnostics (`actual_threads`, `thread_work`).

---

### 3.2 `sequential_analyzer.h`

```cpp
namespace stegoscan {

HistogramResult compute_histogram_sequential(const Image& image);

} // namespace stegoscan
```

- **Contract:** Strictly single-threaded. Counts every pixel sample into the matching bin. Sets `actual_threads = 1`.
- **Errors:** Throws `std::invalid_argument` if image validation fails.

---

### 3.3 `parallel_analyzer.h`

```cpp
namespace stegoscan {

HistogramResult compute_histogram_parallel(const Image& image, int requested_threads);

} // namespace stegoscan
```

- **Contract:**
  - OpenMP parallel row distribution with `schedule(static)`.
  - Thread-local histograms merged post-join.
  - Returns `HistogramResult` mathematically equal to `compute_histogram_sequential(image)`.
  - Records per-thread work in `thread_work`.
- **Errors:** Throws `std::invalid_argument` if image is invalid or `requested_threads < 1`.

---

### 3.4 `chi_square.h`

```cpp
namespace stegoscan {

struct ChiSquareResult {
    double statistic = 0.0;
    std::uint32_t degrees_of_freedom = 0;
    double p_value = 0.0;
    bool is_valid = false;
    std::string note;
};

double regularized_upper_incomplete_gamma(double a, double x);

ChiSquareResult compute_channel_chi_square(const ChannelHistogram& histogram);

std::vector<ChiSquareResult> compute_chi_square(const HistogramResult& histogram);

} // namespace stegoscan
```

- **Pairwise Chi-Square Formula:**
  - Adjacent pairs $(2k, 2k+1)$ for $k \in [0, 127]$.
  - For $a + b > 0$: $\Delta \chi^2 = \frac{(a-b)^2}{a+b}$, $df \mathrel{+}= 1$. Empty pairs ($a+b=0$) are skipped.
  - $p = Q(df / 2.0, \chi^2 / 2.0)$ via regularized upper incomplete gamma.

---

## 4. Detection & Classifier Interfaces (`include/detection/`)

### 4.1 `classifier.h`

```cpp
namespace stegoscan {

enum class DetectionStatus {
    PotentialLsbEvidence,
    NoEvidenceDetected,
    InsufficientData
};

struct ClassifierConfig {
    double p_value_threshold = 0.95;
};

struct ChannelDetectionResult {
    std::size_t channel_index = 0;
    ChiSquareResult chi_square{};
    bool is_suspicious = false;
};

struct DetectionResult {
    DetectionStatus status = DetectionStatus::InsufficientData;
    double p_value_threshold = 0.95;
    std::vector<ChannelDetectionResult> channels;
    std::string explanation;
};

DetectionResult classify(
    const std::vector<ChiSquareResult>& channel_results,
    const ClassifierConfig& config = ClassifierConfig{});

std::string detection_status_to_string(DetectionStatus status);

} // namespace stegoscan
```

- **Decision Rule:**
  - Channel is suspicious if `is_valid && p_value >= p_value_threshold`.
  - Any suspicious channel $\to$ `PotentialLsbEvidence`.
  - No suspicious channels but valid channels exist $\to$ `NoEvidenceDetected`.
  - No valid channels $\to$ `InsufficientData`.
- **Errors:** Throws `std::invalid_argument` if `config.p_value_threshold` $\notin [0.0, 1.0]$.

---

## 5. Evaluation & Dataset Generation (`include/evaluation/`)

### 5.1 `synthetic_dataset_generator.h`

```cpp
namespace stegoscan {

struct SyntheticDatasetConfig {
    std::filesystem::path output_directory;
    std::uint32_t width = 512;
    std::uint32_t height = 512;
    std::uint64_t seed = 12345;
    std::vector<double> payload_rates{0.10, 0.25, 0.50, 1.00};
};

struct DatasetManifestEntry {
    std::string relative_path;
    std::string label; // "clean" or "stego"
    std::string source_cover;
    double payload_rate = 0.0;
    std::uint64_t seed = 0;
    std::string pattern;
};

void generate_synthetic_dataset(const SyntheticDatasetConfig& config);

} // namespace stegoscan
```

- **Contract:** Strictly deterministic; no OpenMP. Generates cover images and LSB-replacement variants, writing images and relative-path `manifest.csv`.

---

### 5.2 `evaluation.h`

```cpp
namespace stegoscan {

struct ConfusionMatrix {
    std::uint64_t true_positives = 0;
    std::uint64_t true_negatives = 0;
    std::uint64_t false_positives = 0;
    std::uint64_t false_negatives = 0;

    double accuracy() const;
    double false_positive_rate() const;
    double precision() const;
    double recall() const;
};

struct EvaluationConfig {
    double threshold = 0.95;
};

struct EvaluationReport {
    ConfusionMatrix metrics;
    double threshold = 0.95;
    std::size_t total_images_evaluated = 0;
    std::size_t failed_images = 0;
    std::vector<std::string> error_messages;
};

EvaluationReport evaluate_dataset(
    const std::filesystem::path& manifest_path,
    const EvaluationConfig& config = EvaluationConfig{});

struct BenchmarkRun {
    int requested_threads = 1;
    int actual_threads = 1;
    double elapsed_seconds = 0.0;
    double speedup = 0.0;
    double efficiency = 0.0;
};

struct BenchmarkResult {
    double sequential_time_seconds = 0.0;
    std::vector<BenchmarkRun> parallel_runs;
};

struct BenchmarkConfig {
    std::vector<int> thread_counts{1, 2, 4, 8};
    int warmup_repetitions = 2;
    int repetitions = 5;
};

BenchmarkResult benchmark_analyzers(
    const Image& image,
    const BenchmarkConfig& config = BenchmarkConfig{});

} // namespace stegoscan
```

---

## 6. Application Controller (`include/app/`)

### 6.1 `app_controller.h`

```cpp
namespace stegoscan {

enum class ExecutionMode {
    Sequential,
    Parallel,
    Compare,
    Evaluate
};

struct CliOptions {
    ExecutionMode mode = ExecutionMode::Sequential;
    std::filesystem::path image_path;
    std::filesystem::path manifest_path;
    int threads = 1;
    double threshold = 0.95;
    bool show_help = false;
    bool show_version = false;
    std::string error_message;
};

CliOptions parse_cli_args(int argc, char* argv[]);
int run_application(const CliOptions& options);
void print_help(const std::string& program_name);

} // namespace stegoscan
```
