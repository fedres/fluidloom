# Halo exchange module

# Find MPI (optional but required for multi-GPU)
find_package(MPI)
if(MPI_FOUND)
    add_definitions(-DFLUIDLOOM_MPI_ENABLED)
    message(STATUS "MPI enabled for multi-GPU halo transport")
else()
    message(WARNING "MPI not found. Single-GPU builds only.")
endif()

# Halo library
add_library(fluidloom_halo_objects OBJECT
    GhostRange.cpp
    buffers/DoubleBufferController.cpp
    packers/HaloPackKernel.cpp
    packers/HaloUnpackKernel.cpp
    interpolation/TrilinearInterpolator.cpp
    interpolation/VolumeWeightedAverager.cpp
    events/EventChain.cpp
    communicators/HaloExchangeManager.cpp
)

target_link_libraries(fluidloom_halo_objects PUBLIC 
    fluidloom_core_objects
)

# Add OpenCL dependency (needed for transport integration)
find_package(OpenCL REQUIRED)
target_include_directories(fluidloom_halo_objects PRIVATE ${OpenCL_INCLUDE_DIRS})
target_link_libraries(fluidloom_halo_objects PUBLIC ${OpenCL_LIBRARIES})

# Include MPI headers
if(MPI_FOUND)
    target_include_directories(fluidloom_halo_objects PUBLIC ${MPI_CXX_INCLUDE_DIRS})
    target_link_libraries(fluidloom_halo_objects PUBLIC ${MPI_CXX_LIBRARIES})
endif()

target_include_directories(fluidloom_halo_objects PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)
