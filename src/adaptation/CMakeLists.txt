# Adaptation module for AMR (cell split/merge, 2:1 balance)

# Core adaptation sources
set(ADAPTATION_SOURCES
    SplitEngine.cpp
    MergeEngine.cpp
    BalanceEnforcer.cpp
    AdaptationEngine.cpp
    utils/HilbertCodec3D.cpp
)

# Create adaptation library
if(ADAPTATION_SOURCES)
    add_library(fluidloom_adaptation STATIC
        ${ADAPTATION_SOURCES}
    )
    
    target_include_directories(fluidloom_adaptation PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )
    
    target_link_libraries(fluidloom_adaptation PUBLIC
        fluidloom_core_objects
        ${OPENCL_LIBRARIES}
    )
endif()

# Placeholder message
message(STATUS "Module 11 (Adaptation) directory structure created")
message(STATUS "  Core data structures: CellDescriptor, AdaptationTypes ready")
message(STATUS "  Implementation in progress...")
