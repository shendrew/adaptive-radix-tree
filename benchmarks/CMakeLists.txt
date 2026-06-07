option(ART_REQUIRE_BENCHMARK_PFM "Require libpfm for benchmark perf counters" ON)
option(ART_USE_BUNDLED_BENCHMARK "Build google-benchmark from source" ON)
# Zen 4-friendly core PMU counters that we know the benchmark can read cleanly.
set(ART_BENCHMARK_PERF_COUNTERS
    "branch-instructions,branch-misses,cache-references,cache-misses"
    CACHE STRING "Comma-separated libpfm perf counters for run_benchmarks")
set(ART_BENCHMARK_MIN_TIME "5s" CACHE STRING "Google Benchmark minimum run time for run_benchmarks")

# 1. Performance Tuning Function (Zen 4 Optimization)
function(apply_zen4_optimizations target_name)
    target_compile_options(${target_name} PRIVATE 
        -O3 
        -march=native 
        -flto 
        -fno-omit-frame-pointer
    )
    target_link_options(${target_name} PRIVATE -flto)
endfunction()

find_library(PFM_LIB pfm)

if(ART_USE_BUNDLED_BENCHMARK)
    include(FetchContent)
    # enables hardware performance counters
    set(BENCHMARK_ENABLE_LIBPFM ON CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        benchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG v1.8.5
    )
    FetchContent_MakeAvailable(benchmark)
else()
    find_package(benchmark CONFIG QUIET)
endif()

include(FetchContent)
set(ABSL_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(ABSL_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    abseil
    GIT_REPOSITORY https://github.com/abseil/abseil-cpp.git
    GIT_TAG 20240116.2
)
FetchContent_MakeAvailable(abseil)

if(ART_REQUIRE_BENCHMARK_PFM AND NOT PFM_LIB)
    message(FATAL_ERROR "libpfm not found. Install libpfm or set ART_REQUIRE_BENCHMARK_PFM=OFF")
endif()

if(NOT ART_USE_BUNDLED_BENCHMARK AND NOT benchmark_FOUND)
    message(STATUS "benchmark library not found - skipping benchmarks")
else()
    file(GLOB_RECURSE ART_BENCHMARK_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
    if(ART_BENCHMARK_SOURCES)
        add_executable(art_benchmarks ${ART_BENCHMARK_SOURCES})

        set_target_properties(art_benchmarks PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
        )

        apply_zen4_optimizations(art_benchmarks)

        target_include_directories(art_benchmarks PRIVATE 
            "${abseil_SOURCE_DIR}"
            "${CMAKE_CURRENT_SOURCE_DIR}"
        )
        target_link_libraries(art_benchmarks PRIVATE ART::AdaptiveRadixTree benchmark::benchmark_main)
        if(PFM_LIB)
            target_link_libraries(art_benchmarks PRIVATE ${PFM_LIB})
            target_compile_definitions(art_benchmarks PRIVATE BENCHMARK_ENABLE_LIBPFM=1)
            message(STATUS "libpfm found: perf counters enabled")
        else()
            message(WARNING "libpfm not found: perf counters disabled")
        endif()

        add_custom_target(run_benchmarks
            COMMAND $<TARGET_FILE:art_benchmarks>
                --benchmark_min_time=${ART_BENCHMARK_MIN_TIME}
                --benchmark_perf_counters=${ART_BENCHMARK_PERF_COUNTERS}
            DEPENDS art_benchmarks
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
        )
        
        message(STATUS "Abseil btree benchmarks enabled")
    endif()
endif()