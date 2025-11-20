#include "fluidloom/core/backend/MockBackend.h"
#include "fluidloom/common/FluidLoomError.h"
#include "fluidloom/common/Logger.h"
#include <cstring>
#include <algorithm>

namespace fluidloom {

MockBuffer::MockBuffer(size_t size, const void* initial_data) 
    : m_data(size, 0) {
    if (initial_data) {
        std::memcpy(m_data.data(), initial_data, size);
    }
}

MockBuffer::~MockBuffer() = default;

MockBackend::MockBackend() 
    : m_initialized(false), m_allocated_bytes(0) {
    FL_LOG(INFO) << "MockBackend created (simulating " << MOCK_MEMORY_LIMIT / (1024*1024*1024) << " GB memory)";
}

MockBackend::~MockBackend() {
    if (m_initialized) {
        shutdown();
    }
}

void MockBackend::initialize(int device_id) {
    if (m_initialized) {
        FL_THROW(BackendError, "MockBackend already initialized");
    }
    
    if (device_id != 0) {
        FL_THROW(BackendError, "MockBackend only supports device_id 0");
    }
    
    m_initialized = true;
    m_allocated_bytes = 0;
    FL_LOG(INFO) << "MockBackend initialized (device " << device_id << ")";
}

void MockBackend::shutdown() {
    if (!m_initialized) {
        FL_THROW(BackendError, "MockBackend not initialized");
    }
    
    if (m_allocated_bytes > 0) {
        FL_LOG(WARN) << "MockBackend shutdown with " << m_allocated_bytes << " bytes still allocated";
    }
    
    m_initialized = false;
    FL_LOG(INFO) << "MockBackend shut down";
}

DeviceBufferPtr MockBackend::allocateBuffer(size_t size, const void* initial_data) {
    if (!m_initialized) {
        FL_THROW(BackendError, "Cannot allocate: MockBackend not initialized");
    }
    
    if (size == 0) {
        FL_THROW(BackendError, "Cannot allocate zero-sized buffer");
    }
    
    if (m_allocated_bytes + size > MOCK_MEMORY_LIMIT) {
        FL_THROW(BackendError, "MockBackend out of memory (allocated: " + 
                 std::to_string(m_allocated_bytes) + ", requested: " + 
                 std::to_string(size) + ", limit: " + std::to_string(MOCK_MEMORY_LIMIT));
    }
    
    auto buffer = std::make_unique<MockBuffer>(size, initial_data);
    m_allocated_bytes += size;
    
    FL_LOG(DEBUG) << "MockBackend allocated " << size << " bytes (total: " << m_allocated_bytes << ")";
    return buffer;
}

void MockBackend::copyHostToDevice(const void* host_src, DeviceBuffer& device_dst, size_t size) {
    if (!host_src) {
        FL_THROW(BackendError, "Host source pointer is null");
    }
    
    // In MockBackend, "device" is actually host memory
    auto& mock_dst = dynamic_cast<MockBuffer&>(device_dst);
    std::memcpy(mock_dst.getDevicePointer(), host_src, size);
    
    FL_LOG(DEBUG) << "MockBackend copied " << size << " bytes H2D";
}

void MockBackend::copyDeviceToHost(const DeviceBuffer& device_src, void* host_dst, size_t size) {
    if (!host_dst) {
        FL_THROW(BackendError, "Host destination pointer is null");
    }
    
    const auto& mock_src = dynamic_cast<const MockBuffer&>(device_src);
    std::memcpy(host_dst, mock_src.getDevicePointer(), size);
    
    FL_LOG(DEBUG) << "MockBackend copied " << size << " bytes D2H";
}

void MockBackend::copyDeviceToDevice(const DeviceBuffer& src, DeviceBuffer& dst, size_t size) {
    const auto& mock_src = dynamic_cast<const MockBuffer&>(src);
    auto& mock_dst = dynamic_cast<MockBuffer&>(dst);
    
    std::memcpy(mock_dst.getDevicePointer(), mock_src.getDevicePointer(), size);
    
    FL_LOG(DEBUG) << "MockBackend copied " << size << " bytes D2D";
}

size_t MockBackend::getMaxAllocationSize() const {
    return MOCK_MEMORY_LIMIT / 4;  // Simulate driver limitation
}

size_t MockBackend::getTotalMemory() const {
    return MOCK_MEMORY_LIMIT;
}

// Kernel management stubs
IBackend::KernelHandle MockBackend::compileKernel(
    const std::string& source_file,
    const std::string& kernel_name,
    const std::string& build_options
) {
    (void)source_file;
    (void)kernel_name;
    (void)build_options;
    
    FL_LOG(INFO) << "MockBackend: Simulating kernel compilation for " << kernel_name;
    return KernelHandle(reinterpret_cast<void*>(0x12345678));  // Dummy handle
}

void MockBackend::launchKernel(
    const KernelHandle& kernel,
    size_t global_work_size,
    size_t local_work_size,
    const std::vector<KernelArg>& args
) {
    (void)kernel;
    (void)global_work_size;
    (void)local_work_size;
    (void)args;
    
    FL_LOG(INFO) << "MockBackend: Simulating kernel launch (global_size=" << global_work_size << ")";
}

void MockBackend::releaseKernel(const KernelHandle& kernel) {
    (void)kernel;
    FL_LOG(INFO) << "MockBackend: Simulating kernel release";
}

} // namespace fluidloom
