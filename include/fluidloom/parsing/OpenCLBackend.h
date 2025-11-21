#pragma once

#include "fluidloom/core/backend/IBackend.h"

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

namespace fluidloom {
namespace parsing {

/**
 * @brief Simple OpenCL backend wrapper for SOAFieldManager
 * 
 * Wraps existing cl_context and cl_command_queue for use with IBackend interface.
 * This is a lightweight adapter - does not own the OpenCL resources.
 */
class OpenCLBackend : public IBackend {
private:
    cl_context context_;
    cl_command_queue queue_;
    bool initialized_;
    
    class OpenCLBuffer : public DeviceBuffer {
    private:
        cl_mem buffer_;
        size_t size_;
        cl_command_queue queue_;
        
    public:
        OpenCLBuffer(cl_mem buf, size_t sz, cl_command_queue q)
            : buffer_(buf), size_(sz), queue_(q) {}
        
        ~OpenCLBuffer() override {
            if (buffer_) {
                clReleaseMemObject(buffer_);
            }
        }
        
        void* getDevicePointer() override { return buffer_; }
        const void* getDevicePointer() const override { return buffer_; }
        size_t getSize() const override { return size_; }
        MemoryLocation getLocation() const override { return MemoryLocation::DEVICE; }
    };
    
public:
    OpenCLBackend(cl_context ctx, cl_command_queue q)
        : context_(ctx), queue_(q), initialized_(true) {
        if (!ctx || !q) {
            throw std::invalid_argument("OpenCLBackend: context and queue must not be null");
        }
        // Retain to ensure they stay valid
        clRetainContext(context_);
        clRetainCommandQueue(queue_);
    }
    
    ~OpenCLBackend() override {
        if (context_) clReleaseContext(context_);
        if (queue_) clReleaseCommandQueue(queue_);
    }
    
    // Device Management
    BackendType getType() const override { return BackendType::OPENCL; }
    std::string getName() const override { return "OpenCL"; }
    int getDeviceCount() const override { return 1; }
    void initialize(int device_id = 0) override { (void)device_id; }
    void shutdown() override { initialized_ = false; }
    
    // Memory Management
    DeviceBufferPtr allocateBuffer(size_t size_in_bytes, const void* initial_data = nullptr) override {
        cl_int err;
        cl_mem_flags flags = CL_MEM_READ_WRITE;
        if (initial_data) {
            flags |= CL_MEM_COPY_HOST_PTR;
        }
        
        cl_mem buffer = clCreateBuffer(context_, flags, size_in_bytes, 
                                       const_cast<void*>(initial_data), &err);
        if (err != CL_SUCCESS || !buffer) {
            return nullptr;
        }
        
        return std::make_unique<OpenCLBuffer>(buffer, size_in_bytes, queue_);
    }
    
    void copyHostToDevice(const void* host_src, DeviceBuffer& device_dst, size_t size) override {
        cl_mem dst_buf = static_cast<cl_mem>(device_dst.getDevicePointer());
        clEnqueueWriteBuffer(queue_, dst_buf, CL_TRUE, 0, size, host_src, 0, nullptr, nullptr);
    }
    
    void copyDeviceToHost(const DeviceBuffer& device_src, void* host_dst, size_t size) override {
        cl_mem src_buf = static_cast<cl_mem>(const_cast<void*>(device_src.getDevicePointer()));
        clEnqueueReadBuffer(queue_, src_buf, CL_TRUE, 0, size, host_dst, 0, nullptr, nullptr);
    }
    
    void copyDeviceToDevice(const DeviceBuffer& src, DeviceBuffer& dst, size_t size) override {
        cl_mem src_buf = static_cast<cl_mem>(const_cast<void*>(src.getDevicePointer()));
        cl_mem dst_buf = static_cast<cl_mem>(dst.getDevicePointer());
        clEnqueueCopyBuffer(queue_, src_buf, dst_buf, 0, 0, size, 0, nullptr, nullptr);
    }
    
    // Synchronization
    void flush() override { clFlush(queue_); }
    void finish() override { clFinish(queue_); }
    
    // Kernel Management (not used by SOAFieldManager)
    KernelHandle compileKernel(const std::string&, const std::string&, const std::string& = "") override {
        throw std::runtime_error("OpenCLBackend::compileKernel not implemented");
    }
    
    void launchKernel(const KernelHandle&, size_t, size_t, const std::vector<KernelArg>&) override {
        throw std::runtime_error("OpenCLBackend::launchKernel not implemented");
    }
    
    void releaseKernel(const KernelHandle&) override {
        throw std::runtime_error("OpenCLBackend::releaseKernel not implemented");
    }
    
    // Properties
    size_t getMaxAllocationSize() const override { return 1ULL << 30; } // 1 GB default
    size_t getTotalMemory() const override { return 4ULL << 30; } // 4 GB default
    bool isInitialized() const override { return initialized_; }
};

} // namespace parsing
} // namespace fluidloom
