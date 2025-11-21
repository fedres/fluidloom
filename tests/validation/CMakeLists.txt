cmake_minimum_required(VERSION 3.20)

# Analytical validation library
add_library(fluidloom_validation INTERFACE)

target_include_directories(fluidloom_validation
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Header-only library, no sources to compile
