cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
cmake_policy(SET CMP0110 NEW)  # Required for ANTLR4 later

project(FluidLoom 
    VERSION 1.0.0
    LANGUAGES CXX C
    DESCRIPTION "Multi-GPU LBM Engine with Adaptive Mesh Refinement"
)

# C++ Standard - Hard requirement, no negotiation
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)  # No GNU extensions for portability

# Option to enable strict warnings (default ON for development, can be disabled for Docker)
option(STRICT_WARNINGS "Treat warnings as errors" ON)

# Build Type - Default to Release with debug symbols
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

# Export compile_commands.json for IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Options (Module 1 scope only)
option(FL_BUILD_TESTS "Build unit tests" ON)
option(FL_ENABLE_OPENCL "Enable OpenCL backend" ON)
option(FL_ENABLE_MOCK "Enable Mock backend (fallback)" ON)
option(FL_TREAT_WARNINGS_AS_ERRORS "Strict warning policy" ON)

# Warning Configuration (Aggressive but justified)
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

if(FL_TREAT_WARNINGS_AS_ERRORS)
    add_compile_options(
        $<$<CXX_COMPILER_ID:MSVC>:/WX>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Werror>
    )
endif()

# Dependencies - Module 1 scope
find_package(Threads REQUIRED)  # For std::thread in future modules

# Add cmake module path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Global include directory
include_directories(include)

if(FL_ENABLE_OPENCL)
    find_package(OpenCL)
    if(NOT OpenCL_FOUND)
        message(STATUS "OpenCL not found, disabling OpenCL backend")
        set(FL_ENABLE_OPENCL OFF CACHE BOOL "Enable OpenCL backend" FORCE)
    endif()
endif()

# MPI - Module 5 requirement
find_package(MPI REQUIRED)
if(MPI_FOUND)
    message(STATUS "MPI found: ${MPI_CXX_COMPILER}")
endif()

# Core library (header-only for now)
# Subdirectories
add_subdirectory(src/core)
add_subdirectory(src/parsing)
add_subdirectory(src/halo)
add_subdirectory(src/transport)
add_subdirectory(src/runtime)
add_subdirectory(src/geometry)
add_subdirectory(src/profiling)
add_subdirectory(src/io)

# Testing
if(FL_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# CPack configuration (Future packaging)
set(CPACK_PACKAGE_NAME "FluidLoom")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_GENERATOR "ZIP;TGZ")
include(CPack)

# Compiler version requirements
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "9.0")
        message(FATAL_ERROR "GCC 9.0+ required. Found ${CMAKE_CXX_COMPILER_VERSION}")
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "10.0")
        message(FATAL_ERROR "Clang 10.0+ required. Found ${CMAKE_CXX_COMPILER_VERSION}")
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    if(MSVC_TOOLSET_VERSION LESS "142")
        message(FATAL_ERROR "MSVC 2019+ (toolset 142+) required. Found ${MSVC_TOOLSET_VERSION}")
    endif()
endif()
add_subdirectory(src/adaptation)
