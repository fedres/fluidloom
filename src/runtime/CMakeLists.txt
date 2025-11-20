# Runtime library - core execution engine
set(RUNTIME_SOURCES
    dependency/FieldVersionTracker.cpp
    dependency/HazardAnalyzer.cpp
    dependency/DependencyGraphBuilder.cpp
    scheduler/TopologicalScheduler.cpp
    scheduler/LevelAwareSorter.cpp
    scheduler/HaloInserter.cpp
    executor/EventChainIntegrator.cpp
    nodes/ExecutionNode.cpp
    nodes/KernelNode.cpp
    nodes/HaloExchangeNode.cpp
    nodes/BarrierNode.cpp
    nodes/AdaptMeshNode.cpp
)

add_library(fluidloom_runtime_objects OBJECT ${RUNTIME_SOURCES})

target_link_libraries(fluidloom_runtime_objects PUBLIC
    fluidloom_core_objects
    fluidloom_halo_objects
    fluidloom_transport_objects
)

target_include_directories(fluidloom_runtime_objects PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# Apply compiler flags conditionally
if(STRICT_WARNINGS)
    target_compile_options(fluidloom_runtime_objects PRIVATE -Wall -Wextra -Werror)
else()
    target_compile_options(fluidloom_runtime_objects PRIVATE -Wall -Wextra)
endif()
