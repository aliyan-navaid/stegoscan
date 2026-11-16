# AGENTS.md — LSB Steganalysis (C++ / OpenMP)

## 1. Project goal

Build a small, command-line **LSB steganalysis** application that analyzes PNG images for statistical signs of **LSB replacement** steganography.

The project has two histogram implementations that must produce identical results:

1. A sequential implementation.
2. An OpenMP implementation that distributes rows of one image across threads and merges thread-local histograms.

The detector consumes the histogram produced by either implementation. The statistical detector and classifier must not need to know whether the histogram was produced sequentially or in parallel.

The main academic objective is to demonstrate **correct parallel work distribution and race-free aggregation**, then measure the performance difference. Prefer a small, understandable, testable implementation over a feature-rich application.

## 2. Instructions to the coding agent

- Implement the project **one module at a time** in the order specified in Section 9.
- Do not start the next module until the current module builds and its independent tests pass.
- Every module must expose a small, documented interface and be testable without running the complete application wherever practical.
- Write tests alongside each module; do not postpone all testing until the end.
- Keep sequential and parallel histogram implementations behind the same result contract.
- Do not duplicate the chi-square or classification logic in the sequential and parallel paths.
- Do not add a GUI, machine-learning model, message extraction, network functionality, or RS analysis unless the project owner explicitly requests it.
- Treat every decision in Section 3 as confirmed. Do not reopen those decisions or silently change them; ask only about an unexpected ambiguity that materially changes scope or blocks implementation.
- Do not claim a module is complete until it has been built and its tests have actually passed.
- Keep dependencies minimal. Do not add a large framework for a task that can be handled with a small interface.
- Use clear names, const-correct interfaces, checked dimensions, and useful error messages. Avoid global mutable state.
- Never use OpenMP for image decoding or final classification. The intended parallel work is histogram construction.

## 3. Confirmed project decisions

The project owner has confirmed the following requirements. Treat them as requirements, not open questions.

| Decision | Confirmed requirement |
|---|---|
| Programming language | C++ |
| C++ standard | C++17 |
| Build system | CMake |
| Input format | PNG only; reject other formats clearly |
| PNG bit depth | 8-bit only; reject 16-bit PNG and lower-bit-depth palette PNG rather than silently converting bit depth |
| PNG image types | Support grayscale, RGB, RGBA, and palette/indexed PNG. Expand palette/indexed images to RGB. Ignore alpha for RGBA. Also accept grayscale+alpha and discard its alpha channel. |
| Detection method | Pairwise chi-square analysis only in the first version. Do not implement RS analysis. |
| Parallel work unit | Rows of one image, using OpenMP `schedule(static)` |
| Parallel aggregation | One private histogram per OpenMP thread, followed by a merge |
| Channel analysis | Analyze each color channel separately; grayscale images have one channel |
| Classification | Flag the image if at least one valid channel has `p_value >= threshold` |
| Initial threshold | `0.95`, configurable through the CLI; this is a starting heuristic, not a validated universal threshold |
| Existing dataset | The owner does not have a dataset. Implement a deterministic synthetic dataset generator that creates clean test images, LSB-replacement versions, and a labeled manifest. |
| User interface | Command-line application |

The synthetic dataset is for repeatable development, demonstration, and basic pipeline evaluation. It is not a substitute for a representative dataset of real photographs. The README and evaluation output must disclose this limitation. Do not block implementation by asking the owner to supply a dataset.

## 4. Scope and non-goals

### In scope

- Load PNG images.
- Validate and normalize their pixel representation.
- Build 256-bin intensity histograms for each analyzed channel.
- Build histograms sequentially and in parallel using OpenMP.
- Compute pairwise chi-square statistics and p-values.
- Produce a cautious image-level assessment and per-channel results.
- Compare sequential and parallel histogram results.
- Benchmark execution with different thread counts.
- Generate a small, reproducible synthetic dataset containing known-clean synthetic PNGs and corresponding LSB-replacement PNGs, plus a CSV manifest.
- Evaluate predictions against the generated synthetic labels and also accept an external labeled manifest later.

### Out of scope for the first version

- Recovering or extracting the hidden payload.
- Detecting every possible steganography algorithm.
- Claiming that an image is definitely clean or definitely steganographic.
- JPEG analysis. JPEG compression changes pixel statistics and requires a different analysis approach.
- RS analysis, machine learning, a GUI, and multi-image parallelism.

The detector is a statistical indicator for potential **LSB replacement**, not a universal steganography detector. Explain this limitation in program output or documentation.

## 5. Proposed source layout

Use this layout unless the existing repository already has a sensible equivalent. Preserve a simple structure; do not create extra layers without a concrete need.

```text
project-root/
├── AGENTS.md
├── CMakeLists.txt
├── README.md
├── include/
│   ├── image/
│   │   ├── decoded_image.h
│   │   ├── image.h
│   │   ├── image_loader.h
│   │   ├── png_writer.h
│   │   └── preprocessor.h
│   ├── analysis/
│   │   ├── histogram.h
│   │   ├── sequential_analyzer.h
│   │   ├── parallel_analyzer.h
│   │   └── chi_square.h
│   ├── detection/
│   │   └── classifier.h
│   └── evaluation/
│       ├── evaluation.h
│       └── synthetic_dataset_generator.h
├── src/
│   ├── main.cpp
│   ├── image/
│   │   ├── image_loader.cpp
│   │   ├── png_writer.cpp
│   │   └── preprocessor.cpp
│   ├── analysis/
│   │   ├── sequential_analyzer.cpp
│   │   ├── parallel_analyzer.cpp
│   │   └── chi_square.cpp
│   ├── detection/
│   │   └── classifier.cpp
│   └── evaluation/
│       ├── evaluation.cpp
│       └── synthetic_dataset_generator.cpp
├── tests/
│   ├── test_main.cpp
│   ├── test_image.cpp
│   ├── test_png_loader.cpp
│   ├── test_png_writer.cpp
│   ├── test_preprocessor.cpp
│   ├── test_sequential_analyzer.cpp
│   ├── test_parallel_analyzer.cpp
│   ├── test_chi_square.cpp
│   ├── test_classifier.cpp
│   ├── test_synthetic_dataset_generator.cpp
│   └── test_pipeline.cpp
├── tools/
│   └── generate_test_dataset.cpp
├── test_images/
│   ├── clean/
│   └── stego/
└── results/
```

A small test harness using assertions is acceptable and avoids requiring a test framework download. If the repository already uses GoogleTest, Catch2, or another test framework, use the existing framework instead of adding a second one.

Use a small, well-established PNG library (for example, LodePNG or system libpng) for both decoding and writing. The dataset generator must save generated PNGs, so ensure the selected dependency supports encoding as well as decoding. Do not write a PNG decoder or encoder from scratch. Document the chosen dependency and its build instructions. The application must reject non-PNG input even if the library supports other formats. Enforce the original PNG bit depth requirement before normalizing decoder output; do not silently down-convert 16-bit or lower-bit-depth PNGs.

## 6. Module contracts

Keep the contracts below stable. Types and filenames can be adjusted slightly to fit the repository, but modules must remain independently testable.

### Module A — Core image data types

**Purpose:** Define validated, dependency-free image data structures used by later modules.

Suggested types:

```cpp
struct DecodedImage {
    std::uint32_t width;
    std::uint32_t height;
    std::uint8_t channels; // decoder output: 1, 2, 3, or 4
    std::vector<std::uint8_t> pixels;
};

struct Image {
    std::uint32_t width;
    std::uint32_t height;
    std::uint8_t channels; // normalized output: 1 (gray) or 3 (RGB)
    std::vector<std::uint8_t> pixels; // row-major, interleaved channels
};
```

Contract:

- Pixel order is row-major and interleaved: for RGB, `R, G, B, R, G, B, ...`.
- Pixel values are unsigned 8-bit integers in the range 0–255.
- `Image` has either one channel (grayscale) or three channels (RGB).
- Validate that width and height are nonzero.
- Validate `pixels.size() == width * height * channels` using overflow-safe size calculations.
- Never assume that untrusted image dimensions can safely be multiplied in a 32-bit integer.

Independent tests:

- Valid grayscale and RGB image objects are accepted.
- Zero width or height is rejected.
- Unsupported channel counts are rejected by the normalized `Image` validator.
- A buffer whose size does not match its dimensions is rejected.
- Large dimensions that overflow a size calculation are rejected safely.

**Completion gate:** data types and validation compile and all tests above pass without needing the PNG loader or OpenMP.

### Module B — PNG image loader

**Purpose:** Read a PNG file and return a `DecodedImage` or a useful error.

Suggested interface:

```cpp
// Adapt the error/result type to the project's style.
DecodedImage load_png(const std::filesystem::path& path);
```

Contract:

- Accept PNG files only. Check the file extension for user feedback, but also rely on the decoder to validate the actual file contents/signature.
- Return decoded 8-bit pixel data and its dimensions/channel information.
- Reject unsupported bit depths or malformed files with a descriptive error.
- Do not perform steganalysis in the loader.
- Do not silently resize, sharpen, denoise, or otherwise alter pixel values.
- Reject any input whose original PNG bit depth is not exactly 8, including 16-bit PNGs and 1-, 2-, or 4-bit indexed/grayscale PNGs. Do not let decoder transformations hide the original bit depth.
- Support 8-bit grayscale, RGB, RGBA, and palette/indexed PNGs. Expand palette/indexed images to RGB (or RGBA when the PNG has palette transparency); leave alpha removal to the preprocessor. Also accept grayscale+alpha and leave alpha removal to the preprocessor.
- Keep the decoder behind this interface so the rest of the code does not depend on a particular PNG library.

Independent tests:

- Load a tiny known grayscale PNG and verify dimensions, channel count, and selected pixel values.
- Load a tiny known RGB PNG and verify dimensions, channel count, and selected pixel values.
- Test all required input types: grayscale, RGB, RGBA, and 8-bit palette/indexed PNG. Also test grayscale+alpha.
- Verify palette/indexed input is expanded to RGB (or RGBA when transparency metadata exists) without changing its represented color samples.
- For RGBA, grayscale+alpha, or palette transparency, preserve alpha in `DecodedImage`; the preprocessor is responsible for discarding it.
- Missing file produces a useful error.
- Corrupt/non-PNG input is rejected.
- Reject 16-bit PNGs and lower-bit-depth (1-, 2-, or 4-bit) PNGs, including indexed PNGs, before any decoder transformation silently expands or down-converts them.

**Completion gate:** loader tests pass without invoking the analyzer or classifier.

### Module B2 — PNG writer

**Purpose:** Write normalized 8-bit grayscale or RGB images to PNG files. This module is needed by the synthetic dataset generator and is independently testable.

Suggested interface:

```cpp
void write_png(const std::filesystem::path& path, const Image& image);
```

Contract:

- Accept only valid normalized `Image` objects with one grayscale channel or three RGB channels.
- Write 8-bit grayscale or RGB PNG data without changing pixel values.
- Do not add an alpha channel.
- Report filesystem and encoder failures as useful errors.
- Keep the PNG library behind this interface.

Independent tests:

- Write a small grayscale image, load it again, and compare dimensions and every pixel.
- Write a small RGB image, load it again, and compare dimensions and every pixel.
- Invalid image buffers are rejected.
- An unwritable output path produces a useful error.

**Completion gate:** round-trip writer/loader tests pass. The writer can be tested without the statistical analyzer or OpenMP.

### Module C — Preprocessor / normalizer

**Purpose:** Convert decoder output to the normalized `Image` format used by the analysis modules.

Suggested interface:

```cpp
Image preprocess_image(const DecodedImage& decoded);
```

Initial default behavior:

- Grayscale (1 channel): preserve as one channel.
- Grayscale+alpha (2 channels): preserve grayscale and discard alpha.
- RGB (3 channels): preserve RGB.
- RGBA (4 channels): preserve RGB and discard alpha.
- Palette/indexed PNG: the loader must expand it to RGB (or RGBA when palette transparency is present) before returning decoder output; do not make this support optional.
- Reject unsupported/invalid data and unsupported bit depth. Do not invent pixel values to repair invalid buffers.

Alpha is ignored because this first version analyzes color intensity samples, not transparency. Document that choice.

Independent tests:

- Grayscale data remains unchanged.
- RGB data remains unchanged.
- Alpha is removed without changing the color/grayscale samples.
- Invalid dimensions, channel counts, and buffer lengths are rejected.

**Completion gate:** preprocessor tests pass using hand-built `DecodedImage` objects; the PNG loader is not required.

### Module D — Histogram data model

**Purpose:** Define the common result returned by both analyzer implementations.

Suggested types:

```cpp
constexpr std::size_t kHistogramBins = 256;
constexpr std::size_t kMaxColorChannels = 3;
using ChannelHistogram = std::array<std::uint64_t, kHistogramBins>;

struct ThreadWorkInfo {
    int thread_id;
    std::uint64_t rows_processed;
    std::uint32_t first_row; // optional diagnostic; define clearly when no rows were assigned
    std::uint32_t last_row;  // optional diagnostic; inclusive if used
};

struct HistogramResult {
    std::array<ChannelHistogram, kMaxColorChannels> channels{};
    std::uint8_t channel_count = 0;
    std::uint64_t samples_per_channel = 0;
    int actual_threads = 1;
    std::vector<ThreadWorkInfo> thread_work; // populated by parallel mode when requested
};
```

A histogram count must use a 64-bit unsigned integer to avoid overflow on large images. `samples_per_channel` is the number of samples counted in each channel and should equal `width * height` for every present channel.

The comparison helper used in tests should compare the histogram bins, channel count, and sample counts. It should intentionally ignore execution metadata such as thread IDs and work counts.

Independent tests:

- Empty histogram starts with all bins equal to zero.
- Histogram comparison reports equal bins as equal and detects a one-bin mismatch.

**Completion gate:** common histogram types compile and their tests pass without OpenMP.

### Module E — Sequential histogram analyzer

**Purpose:** Build channel histograms as the correctness baseline.

Suggested interface:

```cpp
HistogramResult compute_histogram_sequential(const Image& image);
```

Algorithm:

1. Validate the normalized image.
2. Iterate through every row and pixel.
3. For each present channel, increment the bin matching that pixel's 8-bit value.
4. Return the completed `HistogramResult`.

Contract:

- Every image sample is counted exactly once.
- Do not use OpenMP in this implementation.
- Do not apply any statistical test here.
- Results must be deterministic.

Independent tests:

- A tiny grayscale image with hand-calculated expected bin counts.
- A tiny RGB image with different expected counts in each channel.
- An image containing only 0 and 255 values.
- A deterministic pseudo-random image whose bin totals equal its pixel count per channel.
- Invalid input is rejected.

**Completion gate:** all sequential analyzer tests pass and the sum of all 256 bins equals `width * height` for every present channel.

### Module F — OpenMP parallel histogram analyzer

**Purpose:** Perform the same histogram calculation while distributing rows of a single image among OpenMP threads.

Suggested interface:

```cpp
HistogramResult compute_histogram_parallel(const Image& image, int requested_threads);
```

Required design:

1. Validate the image and require `requested_threads >= 1`.
2. Allocate one zero-initialized local histogram per requested thread (or otherwise size storage safely for the actual OpenMP team).
3. Enter an OpenMP parallel region with the requested thread count.
4. Use `omp for schedule(static)` over image rows.
5. Each thread updates only its own local histogram. Do not let threads increment shared histogram bins.
6. Track per-thread row counts for diagnostic output without introducing a data race. Each thread must write only to its own metadata slot.
7. After the parallel region, merge local histograms into one `HistogramResult`.
8. Record the actual team size, which may be lower than the requested count if the runtime does not create the full team.

Pseudocode (adapt to the final types):

```cpp
std::vector<LocalHistogram> local_histograms(requested_threads);
std::vector<ThreadWorkInfo> work(requested_threads);
int actual_threads = 1;

#pragma omp parallel num_threads(requested_threads) shared(actual_threads)
{
    const int tid = omp_get_thread_num();

    #pragma omp single
    actual_threads = omp_get_num_threads();

    #pragma omp for schedule(static)
    for (std::int64_t y = 0; y < image.height; ++y) {
        // Process every x in this row.
        // Update local_histograms[tid], never a shared histogram.
        // Update work[tid].
    }
}

// Merge only the slots used by the actual team.
```

Implementation notes:

- Include `<omp.h>` only in the parallel implementation or its private implementation details.
- Use a signed loop variable type accepted by the OpenMP compiler for the row loop; avoid narrowing a large image height to `int`.
- Do not use a critical section, mutex, or atomic increment for every pixel. Thread-local histograms are the intended solution.
- It is acceptable for a thread to receive zero rows when there are more threads than rows.
- Do not assert that every thread must receive exactly the same number of rows. Static scheduling distributes the iteration space, but exact team behavior can depend on runtime settings.
- `requested_threads == 1` must work and match sequential output.
- Keep histogram merging outside the pixel-processing loop.

Independent tests (these are critical project tests):

- For the same grayscale image, parallel bins exactly equal sequential bins with 1, 2, and 4 threads.
- Repeat for RGB images.
- Test odd image dimensions and dimensions smaller than the requested thread count.
- Test uniform, alternating, and deterministic pseudo-random pixel values.
- Verify that total counts equal `width * height` per channel.
- Verify that the reported row counts across active threads sum to image height.
- Verify that no thread reports more rows than image height and that the actual team size is valid.
- Run the tests repeatedly to help expose races.

**Completion gate:** parallel output exactly matches sequential output for every bin across all test images and thread counts. No statistical module is needed to run these tests.

### Module G — Chi-square detector

**Purpose:** Calculate a pairwise chi-square statistic from a completed histogram. This module must be independent of image loading and OpenMP.

For each channel, pair the 256 intensity bins as:

`(0,1), (2,3), ..., (254,255)`.

For a pair, let:

- `a` be the count of the even-valued bin.
- `b` be the count of the adjacent odd-valued bin.
- `E = (a + b) / 2` be the expected count in each bin under the pair-equality hypothesis.

The pair contribution is:

\[
\frac{(a-E)^2}{E} + \frac{(b-E)^2}{E}
\]

For numerical simplicity, when `a + b > 0`, this is equivalent to:

\[
\frac{(a-b)^2}{a+b}
\]

Skip pairs where `a + b == 0`. Each non-empty pair contributes one degree of freedom. Sum the contributions to get the channel's chi-square statistic.

Suggested result:

```cpp
struct ChiSquareResult {
    double statistic;
    std::uint32_t degrees_of_freedom;
    double p_value;
};
```

Calculate the upper-tail probability:

\[
p = Q\left(\frac{df}{2}, \frac{\chi^2}{2}\right)
\]

where `Q` is the regularized upper incomplete gamma function. Use a well-tested mathematical library if already available. Otherwise, implement the numerical routine in a small, isolated helper and test it against known values. Do not substitute an unverified rough approximation.

Interpretation for this LSB-replacement test: unusually strong equalization within adjacent even/odd pairs can produce a **high** upper-tail p-value. A high p-value is an indicator consistent with LSB replacement, not proof that payload data exists. A low p-value does not prove an image is clean.

Independent tests:

- Equal counts in every non-empty adjacent pair produce statistic 0 and p-value 1 (within floating-point tolerance).
- A histogram with one unequal pair produces the expected statistic for that pair.
- Empty pairs are skipped and do not increase degrees of freedom.
- A histogram with no usable pairs returns a clearly represented invalid/insufficient result rather than dividing by zero.
- Test p-value calculation against known cases, including `df=2, statistic=2`, whose upper-tail probability is `exp(-1)`.
- Verify p-values stay in `[0,1]` for valid inputs.
- Test all channels independently.

**Completion gate:** chi-square tests pass on hand-calculated histograms without loading an image or invoking OpenMP.

### Module H — Classifier

**Purpose:** Convert per-channel chi-square results into a cautious image-level assessment.

Suggested interface:

```cpp
enum class DetectionStatus {
    PotentialLsbEvidence,
    NoEvidenceDetected,
    InsufficientData
};

struct ClassifierConfig {
    double p_value_threshold = 0.95;
};

struct DetectionResult {
    DetectionStatus status;
    // Include per-channel result/status and a concise explanation.
};

DetectionResult classify(
    const std::vector<ChiSquareResult>& channel_results,
    const ClassifierConfig& config);
```

Confirmed initial rule (keep configurable; later calibration may inform a documented threshold change only with the owner's approval):

- A valid channel with `p_value >= p_value_threshold` is flagged as suspicious.
- If at least one valid channel is flagged, return `PotentialLsbEvidence`.
- If valid channel results exist and none is flagged, return `NoEvidenceDetected`.
- If there are no usable channel results, return `InsufficientData`.

Requirements:

- Validate threshold is between 0 and 1 inclusive.
- Preserve per-channel values in the final result; do not hide which channel caused a flag.
- Do not describe `NoEvidenceDetected` as “clean” or “no hidden message.”
- Keep the decision rule in one place and make the threshold configurable.
- Do not claim that `0.95` is scientifically optimal. It is only an initial default to be calibrated on labeled data.

Independent tests:

- A channel exactly at the threshold follows the documented `>=` rule.
- One flagged channel causes the default any-channel rule to flag the image.
- No flagged channels produce `NoEvidenceDetected`.
- No usable channels produce `InsufficientData`.
- Invalid thresholds are rejected.

**Completion gate:** classifier tests pass using manually constructed chi-square results.

### Module I — Evaluation and benchmarking

**Purpose:** Provide reproducible correctness, detection, and performance evaluation without mixing timing code into the detector.

Split this module internally into two small responsibilities if that keeps the code clearer:

1. Dataset evaluation: compare predictions with known labels.
2. Benchmarking: time sequential and parallel histogram construction.

#### Dataset evaluation

- Accept a simple manifest containing image path and expected label (`clean` or `stego`).
- Resolve relative paths relative to the manifest location.
- Load, preprocess, analyze, run chi-square, and classify each image.
- Compute TP, TN, FP, FN, accuracy, and false-positive rate. Also report precision and recall when denominators are nonzero.
- State the selected p-value threshold in the report.
- Do not include a file in metrics if it failed to load; report the error separately rather than silently counting it as a prediction.
- An optional test-only LSB-replacement helper may generate reproducible examples, but synthetic fixtures are not a substitute for evaluating on a representative natural-image dataset.

Suggested manifest format:

```csv
path,label
clean/photo_001.png,clean
stego/photo_001_lsb.png,stego
```

#### Benchmarking

- Benchmark only the histogram analyzers by default; this isolates the part that is actually parallelized.
- Load and preprocess the image before starting the timer.
- Use `omp_get_wtime()` for elapsed time.
- Run a warm-up, then multiple repetitions; report the median or clearly state the aggregation method.
- Before recording timings, verify sequential and parallel histograms are identical.
- Test requested thread counts such as 1, 2, 4, and 8, subject to hardware availability.
- Calculate:

\[
\text{Speedup} = \frac{T_{seq}}{T_{parallel}}
\]

\[
\text{Efficiency} = \frac{\text{Speedup}}{\text{actual thread count}}
\]

- Record actual thread count as well as requested thread count.
- Do not promise that more threads will always be faster. Small images and synchronization/merge overhead can make parallel execution slower.
- Export machine-readable CSV results and print a short human-readable summary.

Independent tests:

- Confusion-matrix metrics on a tiny hand-labeled set with known TP/TN/FP/FN values.
- Correct handling of zero denominators (report undefined or zero according to one documented policy; do not divide by zero).
- Manifest parsing, invalid labels, missing files, and relative paths.
- Benchmark output has nonnegative finite timings and only records a result after histogram equality succeeds.

#### Synthetic dataset generation (required because no dataset is available)

Implement this as a separate, independently testable utility/module, not as part of the detector's normal analysis path.

Suggested interface:

```cpp
struct SyntheticDatasetConfig {
    std::filesystem::path output_directory;
    std::uint32_t width = 512;
    std::uint32_t height = 512;
    std::uint64_t seed = 12345;
    std::vector<double> payload_rates{0.10, 0.25, 0.50, 1.00};
};

void generate_synthetic_dataset(const SyntheticDatasetConfig& config);
```

Required behavior:

- Generate several deterministic synthetic *cover* images directly in memory; do not require a pre-existing image dataset or internet download.
- Include more than one simple image pattern, such as smooth gradients, structured/checker patterns, and seeded pseudo-random texture. Include grayscale and RGB examples.
- Save each generated cover as a PNG and label it `clean` in the manifest.
- For each cover, create one or more stego copies using **LSB replacement** at each configured payload rate. A payload rate is the target fraction of all channel samples selected for replacement; select exactly `round(payload_rate * total_channel_samples)` distinct sample positions.
- Use a seeded pseudo-random generator to choose sample positions without replacement and to generate payload bits, making the chosen positions and payload deterministic for a given seed/configuration.
- For a selected sample, replace only its least significant bit: `new_value = (old_value & 0xFE) | payload_bit`. Leave unselected samples unchanged. A selected sample whose original LSB already equals the payload bit may remain numerically unchanged.
- Save stego variants as PNG and label them `stego` in the manifest. Include the source cover and payload rate in filenames or manifest columns.
- Write a CSV manifest with at least `path,label`; useful additional columns include `source_cover,payload_rate,seed,pattern`.
- Use relative paths in the manifest and resolve them relative to the manifest's directory during evaluation.
- Validate dimensions, payload rates (`0 < rate <= 1`), output directory creation, and write failures.
- Do not modify the original cover image when creating a stego variant.
- Do not use OpenMP inside the dataset generator; its purpose is to create deterministic fixtures, not to benchmark generation.
- Provide a small standalone executable, for example `generate_test_dataset`, with CLI options for output directory, dimensions, seed, and payload rates. Document a one-command invocation.
- Generation must be repeatable: same configuration and seed must produce identical pixel data and manifest content (aside from any intentionally variable metadata such as absolute paths; avoid such metadata).

Independent tests:

- Same config and seed produce identical generated pixel data and labels.
- Different seeds change the generated payload/pattern data where randomness is used.
- Verify the number of selected sample positions equals `round(payload_rate * total_channel_samples)` for each generated variant.
- Verify no sample other than a selected sample changes; every changed sample differs from its source by exactly one, and every output sample differs by at most one.
- Payload rate 1.0 sets the output LSB to the generated payload bit for every sample position.
- Payload rate validation rejects zero, negative, greater-than-one, NaN, and infinity.
- Generated cover and stego PNGs can be reloaded by the PNG loader and have the expected dimensions, channel counts, and labels.
- Manifest entries resolve correctly relative to the manifest location.

Important evaluation limitation:

- These generated images are synthetic. They are suitable for repeatable functional tests and a demonstration that the pipeline processes known generated LSB-replacement examples.
- Do not describe metrics from this synthetic set as real-world detection accuracy, and do not tune or advertise `0.95` as a generally optimal threshold based only on these synthetic images.
- Report the image patterns, seeds, payload rates, and dataset size with every evaluation run. Keep support for a user-supplied real-image manifest so a natural-image dataset can be added later.

**Completion gate:** the generator creates a reproducible PNG dataset and manifest; generator tests pass independently; the evaluation module can consume that manifest and report confusion-matrix metrics.

### Module J — CLI / application controller

**Purpose:** Connect modules. Keep `main.cpp` thin; business logic belongs in the modules above.

Initial command-line behavior can be minimal:

```text
stegdetector <image.png> --mode sequential
stegdetector <image.png> --mode parallel --threads 4
stegdetector <image.png> --mode compare --threads 4
stegdetector <image.png> --mode parallel --threads 4 --threshold 0.95
stegdetector --evaluate <manifest.csv> --threshold 0.95
```

Provide the separate dataset-generation utility after its module is implemented:

```text
generate_test_dataset --output-dir test_images/generated --width 512 --height 512 --seed 12345 --payload-rates 0.10,0.25,0.50,1.00
```

Add these options only after the underlying functions and their independent tests are complete.

The controller should:

1. Parse and validate arguments.
2. Load the PNG.
3. Preprocess it.
4. Call the requested histogram analyzer.
5. Run the shared chi-square detector and classifier.
6. Print per-channel statistics and the cautious overall status.
7. In compare mode, run both analyzers, verify histogram equality, and then show timings and speedup.

The CLI must return a nonzero exit code for invalid arguments or input errors. Keep output stable and understandable. Do not put pixel loops or chi-square formulas in `main.cpp`.

Independent tests:

- Test argument parsing separately if it is nontrivial.
- Test invalid mode, missing path, invalid thread count, and invalid threshold.
- At least one end-to-end smoke test loads a known PNG and runs each mode.

**Completion gate:** all unit tests pass and the CLI works in sequential, parallel, and compare modes.

## 7. OpenMP correctness requirements

These requirements are mandatory because parallel distribution is a major evaluation criterion.

- Use OpenMP (`#pragma omp parallel`, `#pragma omp for`) rather than `std::thread` or another threading library.
- Distribute rows of one image across threads with `schedule(static)` in the initial implementation.
- Each thread writes to its own local histogram and its own work-statistics slot.
- Merge local histograms after the parallel region.
- Do not use a shared histogram incremented by all threads.
- Do not put a critical section or lock around each pixel update.
- Ensure all rows are processed exactly once. Verify this by summing per-thread row counts and by exact histogram equality against the sequential implementation.
- Report both requested and actual OpenMP team size.
- Tests must cover grayscale and RGB, several thread counts, odd dimensions, and more requested threads than rows.
- Ensure the build links OpenMP explicitly with CMake's `OpenMP::OpenMP_CXX` target (or the equivalent in an existing build system).

## 8. Build and test expectations

Preferred CMake setup:

- Require the chosen C++ standard (initially C++17).
- Find and link OpenMP explicitly.
- Build the application, dataset-generation utility, and test executable(s) as separate targets.
- Register tests with CTest.
- Keep third-party decoder code separate from the project's own source files.

Expected developer commands (adjust target names to the final repository):

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Also verify a Release build before benchmarking:

```bash
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --parallel
ctest --test-dir build-release --output-on-failure
```

Do not use Debug timings as the project's performance results. If available, run AddressSanitizer and UndefinedBehaviorSanitizer in a separate Debug configuration. Sanitizer support is a quality check, not a reason to complicate the main build.

## 9. Required implementation sequence

Implement and verify in this order. Do not generate the entire application in one untested pass. A later module may use earlier modules, but each module must also have direct unit tests that isolate its contract.

| Step | Deliverable | Must pass before proceeding |
|---:|---|---|
| 1 | CMake scaffold, test harness, core image types and validation | Configure/build succeeds; core image tests pass |
| 2 | PNG loader with 8-bit/type validation | Loader tests pass for known valid, invalid, and unsupported inputs |
| 3 | PNG writer | Grayscale and RGB round-trip tests pass |
| 4 | Preprocessor | Independent grayscale/RGB/alpha/palette-normalization tests pass |
| 5 | Histogram result type and comparison helper | Histogram type tests pass |
| 6 | Sequential analyzer | Hand-calculated histogram tests pass; bin totals are correct |
| 7 | OpenMP analyzer | Exact per-bin equality with sequential for all test images and thread counts; row counts sum to image height |
| 8 | Chi-square statistic and p-value calculation | Hand-calculated statistic and known p-value tests pass |
| 9 | Classifier | Threshold-boundary and status tests pass |
| 10 | Synthetic dataset generator | Reproducibility, LSB-only modification, PNG output, and manifest tests pass |
| 11 | Evaluation and benchmarking | Metric, manifest, and benchmark validation tests pass |
| 12 | CLI integration and documentation | Full test suite passes; sequential, parallel, compare, evaluate, and dataset-generation workflows work |

At the end of each step:

1. List files added or changed.
2. State the module contract implemented.
3. Show the exact build and test commands run.
4. Report actual test results, including failures. Do not fabricate test output.
5. Fix failures before proceeding.
6. Do not begin the next step until the current step's completion gate is satisfied.

## 10. Acceptance criteria

The project is ready for evaluation when all of the following are true:

- PNG-only input is enforced; only 8-bit PNGs are accepted, and malformed/unsupported files fail clearly.
- Grayscale, RGB, RGBA (alpha ignored), and palette/indexed PNGs (expanded to RGB) are supported; grayscale+alpha is also accepted with alpha ignored.
- The normalized image representation and alpha policy are documented.
- Sequential histograms are correct against hand-calculated fixtures.
- Parallel histograms match sequential histograms exactly in every bin.
- OpenMP distributes image rows; diagnostic data demonstrates the work performed by active threads.
- Per-thread histograms are merged without a per-pixel critical section, lock, or atomic increment.
- Chi-square results are computed once through a shared module for either execution mode.
- Classification uses the confirmed initial rule: flag if any valid channel has `p_value >= 0.95`; threshold is configurable and the output does not claim certainty.
- A deterministic synthetic dataset generator creates clean covers, LSB-replacement variants, and a relative-path CSV manifest without requiring an external dataset.
- Documentation clearly states that synthetic evaluation is not representative real-world accuracy.
- Tests exist for every module and the complete test suite passes.
- Performance measurements use Release builds, exclude PNG loading, verify correctness first, and report requested/actual thread counts.
- Accuracy and false-positive metrics are computed only from labeled data and their threshold is recorded.
- README explains build steps, CLI usage, algorithm assumptions, limitations, and how to run tests/benchmarks.

## 11. Final implementation guidance

All major project choices above have been confirmed by the owner. Do not ask the owner to reconfirm C++17, CMake, PNG-only/8-bit scope, supported PNG image types, chi-square-only detection, the initial `0.95` threshold, or the need for a synthetic dataset generator.

If a low-level library/API choice is not specified, choose a lightweight, maintainable option, document the choice, and continue. Ask the owner only if an unexpected ambiguity materially changes the agreed scope or makes implementation impossible. Prefer the simplest implementation that satisfies the module contracts and acceptance criteria.
