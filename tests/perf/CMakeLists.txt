cmake_minimum_required(VERSION 3.20)

# Performance benchmarks require Google Benchmark
find_package(benchmark QUIET)

if(NOT benchmark_FOUND)
    message(STATUS "Google Benchmark not found, performance benchmarks will be skipped")
    message(STATUS "To enable benchmarks: sudo apt-get install libbenchmark-dev")
    return()
endif()

# Rebuild benchmark
add_executable(rebuild_benchmark
    rebuild/rebuild_benchmark.cpp
)

target_link_libraries(rebuild_benchmark
    PRIVATE
        benchmark::benchmark
        benchmark::benchmark_main
)

# Halo benchmark
add_executable(halo_benchmark
    halo/halo_benchmark.cpp
)

target_link_libraries(halo_benchmark
    PRIVATE
        benchmark::benchmark
        benchmark::benchmark_main
)

# Scaling test (uses GTest, not benchmark)
add_executable(scaling_test
    scaling/scaling_test.cpp
)

target_link_libraries(scaling_test
    PRIVATE
        fluidloom_test_harness
        gtest_main
)

target_include_directories(scaling_test
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/include
)

gtest_discover_tests(scaling_test)

# Add custom target to run all benchmarks
add_custom_target(run_benchmarks
    COMMAND rebuild_benchmark --benchmark_format=json --benchmark_out=rebuild_results.json
    COMMAND halo_benchmark --benchmark_format=json --benchmark_out=halo_results.json
    DEPENDS rebuild_benchmark halo_benchmark
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running all performance benchmarks"
)
