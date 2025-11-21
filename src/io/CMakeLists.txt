add_library(fluidloom_io
    VTKWriter.cpp
)

target_include_directories(fluidloom_io
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(fluidloom_io
    PUBLIC
    fluidloom_core
    fluidloom_core_objects
)
