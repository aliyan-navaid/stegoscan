cmake_minimum_required(VERSION 3.16)
project(stegoscan VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings
if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Find OpenMP explicitly as required by AGENTS.md
find_package(OpenMP REQUIRED)

# Include directories
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/third_party
)

# Third-party LodePNG library (kept separate from project source files)
add_library(lodepng STATIC
    third_party/lodepng/lodepng.cpp
    third_party/lodepng/lodepng.h
)
target_include_directories(lodepng PUBLIC third_party/lodepng)

# Core Stegoscan static library
set(STEGOSCAN_CORE_SOURCES
    src/image/decoded_image.cpp
    src/image/image.cpp
    src/image/image_loader.cpp
    src/image/png_writer.cpp
    src/image/preprocessor.cpp
    src/analysis/histogram.cpp
    src/analysis/sequential_analyzer.cpp
    src/analysis/parallel_analyzer.cpp
    src/analysis/chi_square.cpp
    src/detection/classifier.cpp
    src/evaluation/synthetic_dataset_generator.cpp
    src/evaluation/evaluation.cpp
    src/app/app_controller.cpp
)

add_library(stegoscan_core STATIC ${STEGOSCAN_CORE_SOURCES})
target_link_libraries(stegoscan_core PUBLIC
    lodepng
    OpenMP::OpenMP_CXX
)

# Main command-line application
add_executable(stegdetector src/main.cpp)
target_link_libraries(stegdetector PRIVATE stegoscan_core)

# Standalone dataset generation tool
add_executable(generate_test_dataset tools/generate_test_dataset.cpp)
target_link_libraries(generate_test_dataset PRIVATE stegoscan_core)

# Independent test suite
enable_testing()

set(STEGOSCAN_TEST_SOURCES
    tests/test_main.cpp
    tests/test_image.cpp
    tests/test_png_loader.cpp
    tests/test_png_writer.cpp
    tests/test_preprocessor.cpp
    tests/test_sequential_analyzer.cpp
    tests/test_parallel_analyzer.cpp
    tests/test_chi_square.cpp
    tests/test_classifier.cpp
    tests/test_synthetic_dataset_generator.cpp
    tests/test_pipeline.cpp
)

add_executable(stegoscan_tests ${STEGOSCAN_TEST_SOURCES})
target_include_directories(stegoscan_tests PRIVATE tests)
target_link_libraries(stegoscan_tests PRIVATE stegoscan_core)

add_test(NAME stegoscan_all_tests COMMAND stegoscan_tests)
