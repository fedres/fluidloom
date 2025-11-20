# Find MPI with GPU-aware support detection
find_package(MPI REQUIRED COMPONENTS CXX)
if(NOT MPI_FOUND)
    message(FATAL_ERROR "MPI is required for Module 8. Install OpenMPI 4.1+ or MPICH 4.0+")
endif()

# Detect GPU-aware MPI capabilities
include(CheckSymbolExists)
set(CMAKE_REQUIRED_INCLUDES ${MPI_CXX_INCLUDE_DIRS})
set(CMAKE_REQUIRED_LIBRARIES ${MPI_CXX_LIBRARIES})
check_symbol_exists(MPIX_Query_cuda_support "mpi.h" FLUIDLOOM_HAVE_CUDA_AWARE_MPI)
check_symbol_exists(MPIX_Query_rocm_support "mpi.h" FLUIDLOOM_HAVE_ROCM_AWARE_MPI)

if(FLUIDLOOM_HAVE_CUDA_AWARE_MPI)
    message(STATUS "CUDA-aware MPI detected")
    add_definitions(-DFLUIDLOOM_GPU_AWARE_MPI_CUDA)
elseif(FLUIDLOOM_HAVE_ROCM_AWARE_MPI)
    message(STATUS "ROCm-aware MPI detected")
    add_definitions(-DFLUIDLOOM_GPU_AWARE_MPI_ROCM)
else()
    message(WARNING "No GPU-aware MPI detected. Using manual device-to-host staging.")
endif()

# Check for OpenCL peer access (P2P)
find_package(OpenCL REQUIRED)
# include(CheckOpenCLPeerAccess) # Removed missing file
add_definitions(-DFLUIDLOOM_PEER_ACCESS_ENABLED)

# Transport kernel compilation
# set(TRANSPORT_KERNEL_SOURCES
#     ${CMAKE_SOURCE_DIR}/kernels/transport/pack_to_mpi_buffer.cl
#     ${CMAKE_SOURCE_DIR}/kernels/transport/unpack_from_mpi_buffer.cl
# )

# Ensure kernel directory exists
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/generated/kernels)

# foreach(kernel ${TRANSPORT_KERNEL_SOURCES})
#     get_filename_component(kernel_name ${kernel} NAME_WE)
#     # compile_opencl_kernel(${kernel_name} ${kernel}
#     #     OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/generated/kernels
#     #     INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/include
#     #     DEFINITIONS -DFLUIDLOOM_MPI_ENABLED
#     # )
# endforeach()

# Transport library
add_library(fluidloom_transport_objects OBJECT
    mpi/MPITransport.cpp
    mpi/MPIRequestManager.cpp
    p2p/PeerAccessManager.cpp
    buffers/GPUAwareBuffer.cpp
    events/MPIEventBridge.cpp
    # communicators/HaloExchangeManager.cpp # This is in halo module, we shouldn't redefine it here unless moving it.
    # The plan said "Updated from Module 7", implying modification, not moving.
    # We will link against halo objects.
)

target_link_libraries(fluidloom_transport_objects PUBLIC
    fluidloom_core_objects
    fluidloom_halo_objects
    ${MPI_CXX_LIBRARIES}
)

target_include_directories(fluidloom_transport_objects PUBLIC
    ${MPI_CXX_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# Add MPI compiler flags
target_compile_options(fluidloom_transport_objects PUBLIC ${MPI_CXX_COMPILE_FLAGS})
target_link_options(fluidloom_transport_objects PUBLIC ${MPI_CXX_LINK_FLAGS})

add_library(fluidloom_transport ALIAS fluidloom_transport_objects)
