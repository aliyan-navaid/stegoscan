# Stegoscan Developer Guide

## 1. Environment Setup and Tooling

### Prerequisites
- **Compiler:** GCC 9+, Clang 10+, or MSVC 2019+ with C++17 support.
- **Build System:** CMake 3.16 or higher.
- **OpenMP:** Runtime and compiler support (`-fopenmp`).
- **Dependencies:** LodePNG is vendored in `third_party/lodepng/` and managed automatically by CMake.

---

## 2. Building the Project

### Debug Build
A debug build includes symbols and disables optimizations for clear stack traces and debugging:
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
```

### Release Build
A release build is required for performance benchmarking and timing evaluations:
```bash
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --parallel
```

---

## 3. Running Unit Tests

### Via CTest
```bash
ctest --test-dir build --output-on-failure
```

### Direct Execution
Running the test binary directly displays individual test names and detailed assertions:
```bash
./build/stegoscan_tests
```

---

## 4. How to Implement an Individual Module

Developers assigned to a specific module can work entirely inside their assigned `.cpp` file and its dedicated unit test file.

### Step-by-Step Workflow:
1. **Review Contract:** Read the corresponding header file in `include/` and the detailed specification in `AGENTS.md` (Section 6).
2. **Review Existing Tests:** Inspect the corresponding test file in `tests/test_<module>.cpp`.
3. **Replace Stub in `.cpp`:**
   - Locate the source file in `src/<domain>/<module>.cpp`.
   - Remove the placeholder `throw std::runtime_error(...)`.
   - Implement the algorithm adhering strictly to the contract.
4. **Implement Unit Tests:**
   - Add comprehensive tests covering:
     - Normal valid inputs.
     - Boundary conditions (e.g. 0-values, 255-values, single-row images).
     - Error handling and invalid input rejection.
5. **Build and Test:**
   ```bash
   cmake --build build --parallel
   ./build/stegoscan_tests
   ```
6. **Verify Independent Header Compilation:**
   Ensure no new undeclared headers were assumed:
   ```bash
   g++ -std=c++17 -Iinclude -Ithird_party -c -x c++ include/<domain>/<module>.h -o /dev/null
   ```

---

## 5. Rules for Modifying Shared Interfaces

The shared interfaces defined in `include/` represent stable contracts between teams.

> [!CAUTION]
> **Interface Immutability Rule:**
> Developers must **NOT** unilaterally modify public function signatures, struct layouts, or return types in `include/` without team coordination.

If an architectural change is strictly necessary:
1. Propose the change with written rationale.
2. Confirm the change does not break contracts in `AGENTS.md`.
3. Update `include/<header>.h` and `docs/INTERFACES.md`.
4. Ensure all modules and tests compiling against the updated interface continue to build cleanly.

---

## 6. Integration and Completion Gates

Before submitting a module implementation for integration into the main branch:
- [ ] Module compiles cleanly without warnings (`-Wall -Wextra -Wpedantic`).
- [ ] Independent unit tests for the module pass 100%.
- [ ] The module does not introduce circular dependencies.
- [ ] OpenMP is **never** used in modules other than `parallel_analyzer.cpp`.
- [ ] Both Debug and Release configurations build successfully.
