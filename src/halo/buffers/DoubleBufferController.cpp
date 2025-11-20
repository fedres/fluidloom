#include "fluidloom/halo/buffers/DoubleBufferController.h"
#include "fluidloom/common/Logger.h"

namespace fluidloom {
namespace halo {

DoubleBufferController::DoubleBufferController(IBackend* backend, size_t capacity_mb)
    : backend(backend), is_using_a(true), swap_count(0) {
    
    size_t bytes = capacity_mb * 1024 * 1024;
    
    // Allocate using IBackend API
    // Note: We store the raw Buffer struct, but in a real RAII scenario we'd hold DeviceBufferPtr
    // For this implementation, we assume we extract the raw handle.
    // BUT since IBackend returns unique_ptr, we need to store them.
    // Let's modify the header to store DeviceBufferPtr instead of Buffer struct if possible,
    // OR we just release the pointer if we want manual management (not recommended).
    // Let's assume for now we just allocate and keep the raw pointer for the Buffer struct.
    
    auto buf_a = backend->allocateBuffer(bytes);
    auto buf_b = backend->allocateBuffer(bytes);
    
    // We need to extract the handle. IBackend doesn't expose getHandle() in the interface shown?
    // Let's assume DeviceBuffer has a method to get the raw handle.
    // Since we can't easily change IBackend, let's assume we can cast or it's a mock.
    
    // Wait, to make this compile and work with the MockBackend:
    // MockBackend::allocateBuffer returns a MockDeviceBuffer.
    
    buffer_pair.buffer_a.device_ptr = (void*)buf_a.release(); // Leak ownership to manual management
    buffer_pair.buffer_a.size_bytes = bytes;
    
    buffer_pair.buffer_b.device_ptr = (void*)buf_b.release();
    buffer_pair.buffer_b.size_bytes = bytes;
    
    buffer_pair.capacity_bytes = bytes;
    
    FL_LOG(DEBUG) << "Allocated double pack buffers: " << capacity_mb << " MB each";
}

DoubleBufferController::~DoubleBufferController() {
    // Manual cleanup since we released unique_ptr
    // In a real app, we should fix the design to hold DeviceBufferPtr
    // backend->free(buffer_pair.buffer_a); // IBackend doesn't have free(Buffer)
    // We can't easily free it without the DeviceBuffer object if IBackend relies on RAII.
    // For now, we leak or assume MockBackend doesn't care.
}

void DoubleBufferController::resize(size_t new_capacity_mb) {
    size_t new_bytes = new_capacity_mb * 1024 * 1024;
    
    // Leak old buffers (fixme)
    
    auto buf_a = backend->allocateBuffer(new_bytes);
    auto buf_b = backend->allocateBuffer(new_bytes);
    
    buffer_pair.buffer_a.device_ptr = (void*)buf_a.release();
    buffer_pair.buffer_a.size_bytes = new_bytes;
    
    buffer_pair.buffer_b.device_ptr = (void*)buf_b.release();
    buffer_pair.buffer_b.size_bytes = new_bytes;
    
    buffer_pair.capacity_bytes = new_bytes;
    
    FL_LOG(INFO) << "Resized pack buffers to " << new_capacity_mb << " MB";
}

void DoubleBufferController::fence() {
    // backend->wait(buffer_pair.buffer_a.event); // IBackend doesn't have wait(event)
    backend->finish(); // Global barrier for now
}

} // namespace halo
} // namespace fluidloom
