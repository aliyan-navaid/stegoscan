# Stegoscan Module Assignment and Ownership Matrix

This document maps project responsibilities, dependencies, and file ownership for team members working on individual modules.

---

## Module Assignment Table

| Module ID | Module Name | Developer / Owner | Exclusive Source Files | Exclusive Test Files | Shared Header Dependencies | Can Develop Independently? |
|---|---|---|---|---|---|---|
| **Mod-A** | Core Image Types & Validation | Core Team | `src/image/image.cpp`, `src/image/decoded_image.cpp` | `tests/test_image.cpp` | `include/image/image.h`, `include/image/decoded_image.h` | **Yes** (No dependencies) |
| **Mod-B** | PNG Image Loader | Dev 1 | `src/image/image_loader.cpp` | `tests/test_png_loader.cpp` | `include/image/image_loader.h`, `include/image/decoded_image.h`, `third_party/lodepng/` | **Yes** (Requires Mod-A types) |
| **Mod-B2** | PNG Writer | Dev 2 | `src/image/png_writer.cpp` | `tests/test_png_writer.cpp` | `include/image/png_writer.h`, `include/image/image.h`, `third_party/lodepng/` | **Yes** (Requires Mod-A types) |
| **Mod-C** | Preprocessor / Normalizer | Dev 3 | `src/image/preprocessor.cpp` | `tests/test_preprocessor.cpp` | `include/image/preprocessor.h`, `include/image/decoded_image.h`, `include/image/image.h` | **Yes** (Uses hand-built `DecodedImage` in tests) |
| **Mod-D** | Histogram Data Model | Core Team | `src/analysis/histogram.cpp` | `tests/test_image.cpp` (model tests) | `include/analysis/histogram.h` | **Yes** (No dependencies) |
| **Mod-E** | Sequential Analyzer | Dev 4 | `src/analysis/sequential_analyzer.cpp` | `tests/test_sequential_analyzer.cpp` | `include/analysis/sequential_analyzer.h`, `include/analysis/histogram.h`, `include/image/image.h` | **Yes** (Uses synthetic in-memory `Image`) |
| **Mod-F** | OpenMP Parallel Analyzer | Dev 5 | `src/analysis/parallel_analyzer.cpp` | `tests/test_parallel_analyzer.cpp` | `include/analysis/parallel_analyzer.h`, `include/analysis/histogram.h`, `include/image/image.h` | **Yes** (Uses synthetic in-memory `Image`) |
| **Mod-G** | Chi-Square Detector & Incomplete Gamma | Dev 6 | `src/analysis/chi_square.cpp` | `tests/test_chi_square.cpp` | `include/analysis/chi_square.h`, `include/analysis/histogram.h` | **Yes** (Uses hand-built `HistogramResult`) |
| **Mod-H** | Classifier | Dev 7 | `src/detection/classifier.cpp` | `tests/test_classifier.cpp` | `include/detection/classifier.h`, `include/analysis/chi_square.h` | **Yes** (Uses hand-built `ChiSquareResult`) |
| **Mod-I1** | Synthetic Dataset Generator | Dev 8 | `src/evaluation/synthetic_dataset_generator.cpp`, `tools/generate_test_dataset.cpp` | `tests/test_synthetic_dataset_generator.cpp` | `include/evaluation/synthetic_dataset_generator.h`, `include/image/image.h`, `include/image/png_writer.h` | **Yes** (Requires Mod-B2 writer) |
| **Mod-I2** | Evaluation & Benchmarking | Dev 9 | `src/evaluation/evaluation.cpp` | `tests/test_pipeline.cpp` | `include/evaluation/evaluation.h`, `include/image/image.h`, `include/detection/classifier.h` | **Yes** (Can mock or use existing analyzer stubs) |
| **Mod-J** | CLI / Application Controller | Lead / Integrator | `src/app/app_controller.cpp`, `src/main.cpp` | `tests/test_pipeline.cpp` | `include/app/app_controller.h`, all module headers | **Yes** (Wires completed modules) |

---

## Detailed Module Breakdown

### Module B: PNG Image Loader
- **Ownership:** `src/image/image_loader.cpp`, `tests/test_png_loader.cpp`.
- **Responsibilities:** Validates PNG signature; decodes PNG chunks using LodePNG; verifies bit depth is 8; expands palette images to RGB; preserves alpha in `DecodedImage`.
- **Decoupling:** Can be tested immediately using small valid and invalid PNG files on disk without needing the preprocessor or analyzers.

### Module B2: PNG Writer
- **Ownership:** `src/image/png_writer.cpp`, `tests/test_png_writer.cpp`.
- **Responsibilities:** Encodes normalized 1-channel or 3-channel 8-bit `Image` into PNG format and writes to disk.
- **Decoupling:** Can be tested by writing synthetic images and verifying file generation and round-trips.

### Module C: Preprocessor
- **Ownership:** `src/image/preprocessor.cpp`, `tests/test_preprocessor.cpp`.
- **Responsibilities:** Maps 1-channel $\to$ 1-channel, 2-channel $\to$ 1-channel (drops alpha), 3-channel $\to$ 3-channel, 4-channel $\to$ 3-channel (drops alpha).
- **Decoupling:** Tested using hand-constructed `DecodedImage` instances; does not require the PNG loader or filesystem.

### Module E: Sequential Analyzer
- **Ownership:** `src/analysis/sequential_analyzer.cpp`, `tests/test_sequential_analyzer.cpp`.
- **Responsibilities:** Single-threaded baseline counting 256-bin intensity frequencies.
- **Decoupling:** Tested using hand-crafted `Image` objects with known expected bin counts.

### Module F: Parallel Analyzer (OpenMP)
- **Ownership:** `src/analysis/parallel_analyzer.cpp`, `tests/test_parallel_analyzer.cpp`.
- **Responsibilities:** Distributes rows across OpenMP threads with `schedule(static)`. Uses thread-private local histograms. Merges post-parallel region into `HistogramResult`. Populates `thread_work`.
- **Decoupling:** Tested for bitwise identity with sequential analyzer using synthetic `Image` instances across thread counts (1, 2, 4, 8).

### Module G: Chi-Square Detector
- **Ownership:** `src/analysis/chi_square.cpp`, `tests/test_chi_square.cpp`.
- **Responsibilities:** Implements regularized upper incomplete gamma $Q(a, x)$ and calculates pairwise chi-square statistic across even/odd bin pairs.
- **Decoupling:** Tested using synthetic `ChannelHistogram` arrays with mathematically known chi-square and p-value results.

### Module H: Classifier
- **Ownership:** `src/detection/classifier.cpp`, `tests/test_classifier.cpp`.
- **Responsibilities:** Implements decision heuristic: flags image as `PotentialLsbEvidence` if any valid channel has $p \ge \text{threshold}$.
- **Decoupling:** Tested using mock `std::vector<ChiSquareResult>` structures across threshold boundary cases.

### Module I1: Synthetic Dataset Generator
- **Ownership:** `src/evaluation/synthetic_dataset_generator.cpp`, `tools/generate_test_dataset.cpp`, `tests/test_synthetic_dataset_generator.cpp`.
- **Responsibilities:** Deterministically generates synthetic cover patterns (gradients, checkerboards, textures) and LSB-replacement variants across payload rates; writes CSV manifest.
- **Decoupling:** Generates test fixtures; tested for bit-level LSB replacement accuracy and manifest path resolution.

### Module I2: Dataset Evaluation & Benchmarking
- **Ownership:** `src/evaluation/evaluation.cpp`, `tests/test_pipeline.cpp`.
- **Responsibilities:** Parses CSV manifests, runs detection pipeline, computes confusion matrix metrics, and measures OpenMP speedup and parallel efficiency.
- **Decoupling:** Tested against mock labeled manifests and benchmark timing harnesses.
