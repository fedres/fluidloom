#pragma once

#include "fluidloom/core/backend/IBackend.h"
#include <vector>
#include <unordered_map>
#include <memory>

namespace fluidloom {

class MockBuffer : public DeviceBuffer {
public:
    MockBuffer(size_t size, const void* initial_data);
    ~MockBuffer() override;

    void* getDevicePointer() override { return m_data.data(); }
    const void* getDevicePointer() const override { return m_data.data(); }
    size_t getSize() const override { return m_data.size(); }
    MemoryLocation getLocation() const override { return MemoryLocation::HOST; }

private:
    std::vector<uint8_t> m_data;
};

class MockBackend : public IBackend {
public:
    MockBackend();
    ~MockBackend() override;

    // IBackend implementation
    BackendType getType() const override { return BackendType::MOCK; }
    std::string getName() const override { return "Mock Backend"; }
    int getDeviceCount() const override { return 1; }
    void initialize(int device_id = 0) override;
    void shutdown() override;

    DeviceBufferPtr allocateBuffer(size_t size, const void* initial_data = nullptr) override;
    
    void copyHostToDevice(const void* host_src, DeviceBuffer& device_dst, size_t size) override;
    void copyDeviceToHost(const DeviceBuffer& device_src, void* host_dst, size_t size) override;
    void copyDeviceToDevice(const DeviceBuffer& src, DeviceBuffer& dst, size_t size) override;

    void flush() override {}  // No-op
    void finish() override {}  // No-op

    size_t getMaxAllocationSize() const override;
    size_t getTotalMemory() const override;
    bool isInitialized() const override { return m_initialized; }

    // Kernel management (stubs for testing)
    KernelHandle compileKernel(
        const std::string& source_file,
        const std::string& kernel_name,
        const std::string& build_options = ""
    ) override;

    void launchKernel(
        const KernelHandle& kernel,
        size_t global_work_size,
        size_t local_work_size,
        const std::vector<KernelArg>& args
    ) override;

    void releaseKernel(const KernelHandle& kernel) override;

private:
    bool m_initialized;
    size_t m_allocated_bytes;
    static constexpr size_t MOCK_MEMORY_LIMIT = size_t(16) * 1024 * 1024 * 1024; // 16GB simulated
};

} // namespace fluidloom
