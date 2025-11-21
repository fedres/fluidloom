add_library(fluidloom_geometry
    implicit/ImplicitEvaluator.cpp
    voxelizer/SimpleSTLVoxelizer.cpp
    placer/GeometryPlacer.cpp
)

target_include_directories(fluidloom_geometry
    PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(fluidloom_geometry
    PUBLIC
    fluidloom_core
    fluidloom_core_objects
)
