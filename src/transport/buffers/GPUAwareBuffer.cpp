#include "fluidloom/transport/GPUAwareBuffer.h"
#include "fluidloom/common/Logger.h"

namespace fluidloom {
namespace transport {

GPUAwareBuffer::GPUAwareBuffer(IBackend* backend, size_t size_bytes)
    : size_bytes(size_bytes), is_gpu_aware(false), is_peer_accessible(false), is_bound_to_mpi(false), ref_count(0) {
    
    // For now, we just allocate a standard buffer.
    // In a real implementation, we would check backend capabilities and use
    // CL_MEM_ALLOC_HOST_PTR or other flags if GPU-aware MPI is detected.
    // Since IBackend doesn't expose flags directly in allocateBuffer, we rely on
    // the backend's default behavior or need to extend IBackend.
    // For this module, we assume the backend allocates device memory.
    
    // TODO: Extend IBackend to support allocation flags for GPU-aware MPI
    
    // Mock implementation for now:
    // We allocate a device buffer.
    // host_ptr is null unless we map it (which is expensive).
    // For GPU-aware MPI, we assume the driver can handle the device pointer directly.
    
    // In a real scenario:
    // cl_mem_flags flags = CL_MEM_READ_WRITE;
    // if (gpu_aware) flags |= CL_MEM_ALLOC_HOST_PTR;
    // device_buffer = backend->allocateBuffer(size_bytes, flags);
    
    // Using existing API:
    storage = backend->allocateBuffer(size_bytes);
    device_buffer = Buffer{storage->getDevicePointer(), storage->getSize(), nullptr};
    
    // We need to keep the DeviceBufferPtr alive? 
    // The current Buffer struct is just a raw pointer wrapper.
    // This is a potential issue if the underlying allocation is freed.
    // However, Module 7 established that Buffer is just a view.
    // Here, GPUAwareBuffer owns the memory?
    // The plan says "Canonical allocation".
    // So GPUAwareBuffer should probably hold the DeviceBufferPtr.
    // But the struct definition has `Buffer device_buffer`.
    // Let's assume we need to hold the smart pointer too, but it wasn't in the struct.
    // Wait, the struct has `Buffer device_buffer`. `Buffer` in IBackend.h is `struct Buffer { void* mem; size_t size; void* event; };`
    // It's a raw handle.
    // We need to manage the lifetime.
    // The `createGPUAwareBuffer` factory should probably return a wrapper that holds the `DeviceBufferPtr`.
    // But `GPUAwareBuffer` struct definition in the plan doesn't have `DeviceBufferPtr`.
    // This implies `GPUAwareBuffer` IS the manager.
    // But `IBackend::allocateBuffer` returns `DeviceBufferPtr`.
    // We need to store that `DeviceBufferPtr` to keep it alive.
    // I will add a `std::shared_ptr<DeviceBuffer>` member to `GPUAwareBuffer` implementation detail, 
    // or just leak it if we can't change the header (but I can change the header since I just wrote it).
    // Actually, I should update the header to include `DeviceBufferPtr` or similar.
    // But `DeviceBufferPtr` is `std::unique_ptr<DeviceBuffer>`.
    // I'll add `std::shared_ptr<void>` opaque handle to keep it alive, or just modify the header to include `DeviceBufferPtr`.
    // Let's modify the header in the next step if needed. For now, I'll just use a raw pointer and assume the backend keeps it? No, that's unsafe.
    // I will modify `GPUAwareBuffer.h` to include `DeviceBufferPtr` storage.
}

GPUAwareBuffer::~GPUAwareBuffer() {
    waitForUnbind();
    // DeviceBufferPtr will be destroyed here if I add it.
}

std::string GPUAwareBuffer::toString() const {
    std::stringstream ss;
    ss << "GPUAwareBuffer(size=" << size_bytes 
       << ", gpu_aware=" << is_gpu_aware 
       << ", bound=" << is_bound_to_mpi << ")";
    return ss.str();
}

std::unique_ptr<GPUAwareBuffer> createGPUAwareBuffer(
    IBackend* backend, 
    size_t size_bytes,
    bool prefer_peer_access
) {
    (void)prefer_peer_access; // Suppress unused warning
    auto buffer = std::make_unique<GPUAwareBuffer>(backend, size_bytes);
    
    // Here we would set flags based on detection
    #ifdef FLUIDLOOM_GPU_AWARE_MPI_CUDA
    buffer->is_gpu_aware = true;
    #endif
    
    return buffer;
}

} // namespace transport
} // namespace fluidloom
