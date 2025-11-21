add_library(fluidloom_profiling
    Profiler.cpp
)

target_include_directories(fluidloom_profiling
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(fluidloom_profiling
    PUBLIC
    fluidloom_core
    fluidloom_core_objects
)
