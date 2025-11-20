# Core library (header-only in Module 1, will become static library later)
add_library(fluidloom_core INTERFACE)
add_library(FluidLoom::Core ALIAS fluidloom_core)

target_include_directories(fluidloom_core INTERFACE 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<INSTALL_INTERFACE:include>
)

# Backend sources (compiled as part of tests for now)
set(BACKEND_SOURCES
    backend/MockBackend.cpp
    backend/OpenCLBackend.cpp
    backend/BackendFactory.cpp
)

set(COMMON_SOURCES
    ../common/Logger.cpp
)

set(HILBERT_SOURCES
    hilbert/HilbertCodec.cpp
    hilbert/CellCoord.cpp
)

set(FIELDS_SOURCES
    fields/FieldDescriptor.cpp
    fields/SOAFieldManager.cpp
)

set(REGISTRY_SOURCES
    registry/FieldRegistry.cpp
)

set(HASHMAP_SOURCES
    hashmap/RadixSort.cpp
    hashmap/CompactionEngine.cpp
    hashmap/HashTableManager.cpp
)

set(HALO_SOURCES
    ../halo/GhostRangeBuilder.cpp
    ../halo/HaloExchanger.cpp
    ../common/mpi/MPIEnvironment.cpp
)

# Create object library for core components
add_library(fluidloom_core_objects OBJECT 
    ${BACKEND_SOURCES} 
    ${COMMON_SOURCES}
    ${HILBERT_SOURCES}
    ${FIELDS_SOURCES}
    ${REGISTRY_SOURCES}
    ${HASHMAP_SOURCES}
    ${HALO_SOURCES}
)

# Add OpenCL kernel copying
add_custom_target(fluidloom_kernels ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/kernels
    ${CMAKE_BINARY_DIR}/kernels
    COMMENT "Copying OpenCL kernels to build directory"
)
target_include_directories(fluidloom_core_objects PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_include_directories(fluidloom_core_objects PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${OpenCL_INCLUDE_DIRS}
)

# Apply compiler flags conditionally
if(STRICT_WARNINGS)
    target_compile_options(fluidloom_core_objects PRIVATE -Wall -Wextra -Werror)
else()
    target_compile_options(fluidloom_core_objects PRIVATE -Wall -Wextra)
endif()

target_link_libraries(fluidloom_core_objects PRIVATE Threads::Threads)
if(FL_ENABLE_OPENCL)
    target_link_libraries(fluidloom_core_objects PUBLIC ${OpenCL_LIBRARIES})
endif()
target_link_libraries(fluidloom_core_objects PRIVATE MPI::MPI_CXX)

# For now, tests link directly against object files
# In future modules, this will become a static library
