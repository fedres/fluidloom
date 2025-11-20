#pragma once

#include <atomic>
#include <memory>
#include <thread>
#include <chrono>
#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif
#include "fluidloom/core/backend/IBackend.h"
#include "fluidloom/core/soa/Buffer.h"

namespace fluidloom {
namespace transport {

/**
 * @brief Memory buffer with dual accessibility: GPU device and MPI transport
 * 
 * This struct is the **canonical allocation for all inter-GPU communication**.
 * The buffer is allocated with CL_MEM_ALLOC_HOST_PTR for GPU-aware MPI,
 * or CL_MEM_USE_PERSISTENT_MEM_AMD / CL_MEM_IS_P2P_NV for peer access.
 * The same pointer is used by both OpenCL kernels and MPI_Isend/Irecv.
 * 
 * The buffer maintains a reference count and tracks whether it is currently
 * bound to an MPI request to prevent premature deallocation.
 * 
 * This layout is IMMUTABLE and must be agreed upon by all modules.
 */
struct GPUAwareBuffer {
    // OpenCL buffer handle (device-accessible)
    Buffer device_buffer;
    
    // Storage for the underlying allocation
    DeviceBufferPtr storage;
    
    // Host-visible pointer for GPU-aware MPI (NULL if not GPU-aware)
    void* host_ptr;
    
    // Size in bytes (must match pack buffer size from Module 7)
    size_t size_bytes;
    
    // Allocation flags
    bool is_gpu_aware;      // True if MPI can directly read/write device memory
    bool is_peer_accessible; // True if P2P copy between devices works
    bool is_bound_to_mpi;   // True if currently in an MPI operation
    
    // Reference counting for safe deallocation
    std::atomic<int> ref_count;
    
    // Constructor allocates based on backend capabilities
    GPUAwareBuffer(IBackend* backend, size_t size_bytes);
    
    // Non-copyable (use shared_ptr)
    GPUAwareBuffer(const GPUAwareBuffer&) = delete;
    GPUAwareBuffer& operator=(const GPUAwareBuffer&) = delete;
    
    // Move constructor
    GPUAwareBuffer(GPUAwareBuffer&&) = delete;
    
    // Destructor waits for all MPI operations to complete
    ~GPUAwareBuffer();
    
    // Get device pointer for OpenCL kernel launch
    cl_mem getCLMem() const { return static_cast<cl_mem>(device_buffer.device_ptr); }
    
    // Get host pointer for MPI (may be NULL if not GPU-aware)
    void* getHostPtr() const { return host_ptr; }
    
    // Mark buffer as bound to MPI request
    void markBound() { 
        ref_count.fetch_add(1);
        is_bound_to_mpi = (ref_count.load() > 0);
    }
    
    // Unmark after MPI completion
    void markUnbound() {
        if (ref_count.load() > 0) {
            ref_count.fetch_sub(1);
            is_bound_to_mpi = (ref_count.load() > 0);
        }
    }
    
    // Wait for all references to be released
    void waitForUnbind() {
        while (ref_count.load() > 0) {
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
    }
    
    // Validate buffer is ready for reuse
    bool isReady() const { return !is_bound_to_mpi && ref_count.load() == 0; }
    
    // For debugging
    std::string toString() const;
};

// Factory function that auto-detects best allocation strategy
std::unique_ptr<GPUAwareBuffer> createGPUAwareBuffer(
    IBackend* backend, 
    size_t size_bytes,
    bool prefer_peer_access = true
);

} // namespace transport
} // namespace fluidloom
