# This module defines:
# OpenCL_FOUND, OpenCL_INCLUDE_DIRS, OpenCL_LIBRARIES, OpenCL_VERSION

if(NOT OpenCL_FOUND)
    # Try modern CMake config first
    find_package(OpenCL CONFIG QUIET)
    
    if(NOT OpenCL_FOUND)
        # Fallback to manual detection
        find_path(OpenCL_INCLUDE_DIRS
            NAMES CL/cl.h OpenCL/cl.h
            PATHS
                ENV OCL_ROOT
                ENV AMDAPPSDKROOT
                ENV CUDA_PATH
                ENV INTELOCLSDKROOT
                /usr/local/cuda
                /opt/amd/app-sdk
        )
        
        find_library(OpenCL_LIBRARIES
            NAMES OpenCL
            PATHS
                ENV OCL_ROOT/lib
                ENV AMDAPPSDKROOT/lib
                ENV CUDA_PATH/lib64
                ENV INTELOCLSDKROOT/lib
                /usr/local/cuda/lib64
                /opt/amd/app-sdk/lib
        )
        
        if(OpenCL_INCLUDE_DIRS AND OpenCL_LIBRARIES)
            set(OpenCL_FOUND TRUE)
            # Extract version from header
            if(EXISTS "${OpenCL_INCLUDE_DIRS}/CL/cl.h")
                file(READ "${OpenCL_INCLUDE_DIRS}/CL/cl.h" CL_HEADER)
            elseif(EXISTS "${OpenCL_INCLUDE_DIRS}/OpenCL/cl.h")
                file(READ "${OpenCL_INCLUDE_DIRS}/OpenCL/cl.h" CL_HEADER)
            elseif(EXISTS "${OpenCL_INCLUDE_DIRS}/Headers/cl.h")
                 file(READ "${OpenCL_INCLUDE_DIRS}/Headers/cl.h" CL_HEADER)
            endif()
            
            if(CL_HEADER)
                string(REGEX MATCH "__OPENCL_VERSION__ ([0-9]+)" _ ${CL_HEADER})
                set(OpenCL_VERSION "${CMAKE_MATCH_1}")
            endif()
        endif()
    endif()
endif()

# Validation
if(OpenCL_FOUND)
    if(NOT OpenCL_VERSION VERSION_GREATER_EQUAL "200")
        message(WARNING "OpenCL ${OpenCL_VERSION} < 2.0 detected. Some features may be limited.")
    endif()
    message(STATUS "OpenCL found: ${OpenCL_INCLUDE_DIRS}, version ${OpenCL_VERSION}")

    if(NOT TARGET OpenCL::OpenCL)
        add_library(OpenCL::OpenCL UNKNOWN IMPORTED)
        set_target_properties(OpenCL::OpenCL PROPERTIES
            IMPORTED_LOCATION "${OpenCL_LIBRARIES}"
            INTERFACE_INCLUDE_DIRECTORIES "${OpenCL_INCLUDE_DIRS}"
        )
    endif()
else()
    message(STATUS "OpenCL NOT found. Only Mock backend will be available.")
endif()
