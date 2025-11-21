cmake_minimum_required(VERSION 3.20)

# Test harness library
add_library(fluidloom_test_harness
    TestHarness.cpp
    GoldDataManager.cpp
)

target_include_directories(fluidloom_test_harness
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(fluidloom_test_harness
    PUBLIC
        fluidloom_core_objects
        fluidloom_parsing
)

# Platform-specific libraries for memory tracking
if(APPLE)
    # macOS uses mach for memory info
elseif(UNIX)
    # Linux uses rusage
endif()
