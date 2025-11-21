#pragma once

#include "fluidloom/core/backend/IBackend.h"

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include <vector>
#include <string>
#include <memory>
#include <map>

namespace fluidloom {

class OpenCLBuffer : public DeviceBuffer {
public:
    OpenCLBuffer(cl_mem buffer, size_t size, cl_context context);
    ~OpenCLBuffer() override;

    void* getDevicePointer() override { return m_buffer; }
    const void* getDevicePointer() const override { return m_buffer; }
    size_t getSize() const override { return m_size; }
    MemoryLocation getLocation() const override { return MemoryLocation::DEVICE; }

    cl_mem getCLMem() const { return m_buffer; }

private:
    cl_mem m_buffer;
    size_t m_size;
};

class OpenCLBackend : public IBackend {
public:
    OpenCLBackend();
    ~OpenCLBackend() override;

    // IBackend implementation
    BackendType getType() const override { return BackendType::OPENCL; }
    std::string getName() const override;
    int getDeviceCount() const override;
    void initialize(int device_id = 0) override;
    void shutdown() override;

    DeviceBufferPtr allocateBuffer(size_t size, const void* initial_data = nullptr) override;
    
    void copyHostToDevice(const void* host_src, DeviceBuffer& device_dst, size_t size) override;
    void copyDeviceToHost(const DeviceBuffer& device_src, void* host_dst, size_t size) override;
    void copyDeviceToDevice(const DeviceBuffer& src, DeviceBuffer& dst, size_t size) override;

    void flush() override;
    void finish() override;

    size_t getMaxAllocationSize() const override;
    size_t getTotalMemory() const override;
    bool isInitialized() const override { return m_initialized; }

    // Kernel management
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

    // OpenCL-specific methods
    cl_context getContext() const { return m_context; }
    cl_command_queue getQueue() const { return m_queue; }
    cl_device_id getDevice() const { return m_device; }

private:
    bool m_initialized;
    cl_platform_id m_platform;
    cl_device_id m_device;
    cl_context m_context;
    cl_command_queue m_queue;
    
    std::vector<cl_device_id> m_all_devices;
    
    // Compiled programs and kernels
    std::map<std::string, cl_program> m_programs;
    std::map<void*, cl_kernel> m_kernels;
    std::map<void*, std::string> m_kernel_names;
    
    void checkError(cl_int error, const std::string& operation);
    void queryDeviceInfo();
};

} // namespace fluidloom
