# Stegoscan — LSB Steganalysis in C++17 with OpenMP

`stegoscan` is a command-line LSB (Least Significant Bit) steganalysis application that analyzes PNG images for statistical signs of **LSB replacement steganography**.

The core technical objective of this project is demonstrating **correct parallel work distribution and race-free aggregation** using OpenMP, and measuring performance scaling against a ground-truth sequential baseline.

---

## Architecture and Design Overview

The application is structured into decoupled modules:
1. **PNG Loader & Normalizer:** Decodes 8-bit PNG images (grayscale, RGB, RGBA, and palette/indexed PNGs expanded to RGB; alpha channels are discarded). Rejects non-PNG formats and bit depths other than 8.
2. **Histogram Analyzers:**
   - **Sequential Analyzer:** Single-threaded baseline constructing 256-bin intensity histograms.
   - **OpenMP Parallel Analyzer:** Distributes image rows across threads using `#pragma omp for schedule(static)`. Each thread updates a private thread-local histogram and private work statistics (avoiding per-pixel critical sections or atomic locks), followed by post-parallel merging.
3. **Statistical Detector:** Shared pairwise chi-square analysis evaluating pairs of values (PoVs) $(2k, 2k+1)$ and computing upper-tail probabilities via the regularized incomplete gamma function $Q(df/2, \chi^2/2)$.
4. **Classifier:** Evaluates per-channel p-values against a configurable threshold ($p \ge 0.95$) to produce cautious assessments (`PotentialLsbEvidence`, `NoEvidenceDetected`, `InsufficientData`).
5. **Synthetic Dataset Generator:** Deterministically creates clean cover images and LSB-replacement variants across configured payload rates with a labeled manifest.
6. **Evaluation & Benchmarking:** Computes classification metrics (TP, TN, FP, FN, accuracy, FPR, precision, recall) and measures analyzer speedup and parallel efficiency.

---

## Important Limitations and Scope

> [!WARNING]
> **Statistical Indicator Only**: The chi-square detector is a statistical indicator for potential **LSB replacement**, not a universal steganography detector. It does not extract or decrypt hidden payloads, nor does it detect LSB matching (which preserves pair parity), spread-spectrum, or transform-domain steganography.
> 
> **Synthetic Dataset Notice**: The included synthetic dataset generator creates synthetic fixtures for repeatable testing and functional validation. Metrics evaluated on synthetic images do not represent real-world photographic detection accuracy.

---

## Build Requirements

- **C++ Compiler:** C++17 compliant compiler (GCC $\ge 9$, Clang $\ge 10$, MSVC $\ge 2019$)
- **Build System:** CMake $\ge 3.16$
- **Parallel Computing:** OpenMP $\ge 3.0$
- **PNG Library:** Bundled self-contained LodePNG in `third_party/lodepng/` (zero external dependencies)

---

## Building the Project

### Debug Build
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

### Release Build (Recommended for Benchmarking)
```bash
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --parallel
ctest --test-dir build-release --output-on-failure
```

---

## Running the Application

### 1. Sequential Mode
```bash
./build/stegdetector <path-to-image.png> --mode sequential
```

### 2. Parallel Mode (OpenMP)
```bash
./build/stegdetector <path-to-image.png> --mode parallel --threads 4
```

### 3. Comparison & Verification Mode
Runs both sequential and OpenMP analyzers, verifies that histograms are bitwise identical, and reports execution timing and speedup:
```bash
./build/stegdetector <path-to-image.png> --mode compare --threads 4
```

### 4. Custom Decision Threshold
```bash
./build/stegdetector <path-to-image.png> --mode parallel --threads 4 --threshold 0.90
```

### 5. Dataset Evaluation
```bash
./build/stegdetector --evaluate test_images/manifest.csv --threshold 0.95
```

### 6. Synthetic Dataset Generator Utility
```bash
./build/generate_test_dataset --output-dir test_images/generated --width 512 --height 512 --seed 12345
```

---

## Repository Structure

```text
stegoscan/
├── CMakeLists.txt              # CMake build configuration
├── README.md                   # Project overview and build guide
├── AGENTS.md                   # Complete architectural requirements specification
├── docs/                       # Developer collaboration documentation
│   ├── ARCHITECTURE.md         # System architecture, data flow, diagrams
│   ├── INTERFACES.md           # API contracts, signatures, shared data types
│   ├── DEVELOPMENT.md          # Build instructions, test execution, workflows
│   └── MODULE_ASSIGNMENTS.md   # Module ownership, dependencies, decoupling
├── include/                    # Public header interfaces
│   ├── image/                  # Core image types, loader, writer, preprocessor
│   ├── analysis/               # Histogram model, sequential/parallel analyzers, chi-square
│   ├── detection/              # Classifier and detection results
│   ├── evaluation/             # Dataset evaluation, benchmarking, dataset generator
│   └── app/                    # Application controller and CLI options
├── src/                        # Module implementations (stubs in scaffolding phase)
│   ├── image/
│   ├── analysis/
│   ├── detection/
│   ├── evaluation/
│   ├── app/
│   └── main.cpp                # CLI entry point
├── tools/                      # Standalone CLI tools
│   └── generate_test_dataset.cpp
├── tests/                      # Independent unit test suite
│   ├── test_harness.h          # Lightweight header-only testing framework
│   ├── test_main.cpp           # Test runner
│   ├── test_image.cpp          # Image types & validation tests
│   ├── test_png_loader.cpp     # PNG loader contract tests
│   ├── test_png_writer.cpp     # PNG writer contract tests
│   ├── test_preprocessor.cpp   # Preprocessor contract tests
│   ├── test_sequential_analyzer.cpp # Sequential analyzer contract tests
│   ├── test_parallel_analyzer.cpp   # Parallel analyzer contract tests
│   ├── test_chi_square.cpp     # Chi-square & incomplete gamma tests
│   ├── test_classifier.cpp     # Classification decision rule tests
│   ├── test_synthetic_dataset_generator.cpp # Generator contract tests
│   └── test_pipeline.cpp       # CLI parsing & metric calculation tests
├── third_party/
│   └── lodepng/                # Bundled LodePNG library
├── test_images/                # Test fixtures (clean & stego)
└── results/                    # Benchmark and evaluation outputs
```

---

## Documentation Links

- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)
- [docs/INTERFACES.md](docs/INTERFACES.md)
- [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md)
- [docs/MODULE_ASSIGNMENTS.md](docs/MODULE_ASSIGNMENTS.md)
