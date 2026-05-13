cmake_minimum_required(VERSION 3.20)

project(AdaptiveRadixTree VERSION 0.1.0 LANGUAGES CXX)

include(CTest)

# Toggles for development
option(ART_BUILD_TESTS "Build unit tests" ON)
option(ART_BUILD_BENCHMARKS "Build benchmarks" ON)

# The Core Library
add_library(AdaptiveRadixTree INTERFACE)
add_library(ART::AdaptiveRadixTree ALIAS AdaptiveRadixTree)

# Requirements for anyone using the tree
target_compile_features(AdaptiveRadixTree INTERFACE cxx_std_23)
target_include_directories(AdaptiveRadixTree
    INTERFACE 
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

# main.cpp demo
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")
    add_executable(AdaptiveRadixTree_demo src/main.cpp)
    target_link_libraries(AdaptiveRadixTree_demo PRIVATE ART::AdaptiveRadixTree)
endif()

# Unit Tests Block
if(ART_BUILD_TESTS)
    find_package(GTest CONFIG QUIET)
    if(GTest_FOUND)
        file(GLOB_RECURSE ART_TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp")
        if(ART_TEST_SOURCES)
            add_executable(art_tests ${ART_TEST_SOURCES})
            target_link_libraries(art_tests PRIVATE ART::AdaptiveRadixTree GTest::gtest_main)
            include(GoogleTest)
            gtest_discover_tests(art_tests)
        endif()
    endif()
endif()

# Benchmarks Block
if(ART_BUILD_BENCHMARKS)
    find_package(benchmark CONFIG QUIET)
    if(benchmark_FOUND)
        file(GLOB_RECURSE ART_BENCHMARK_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/benchmarks/*.cpp")
        if(ART_BENCHMARK_SOURCES)
            add_executable(art_benchmarks ${ART_BENCHMARK_SOURCES})
            target_link_libraries(art_benchmarks PRIVATE ART::AdaptiveRadixTree benchmark::benchmark_main)
        endif()
    endif()
endif()