#pragma once

#include <memory>
#include <vector>
#include <string>
#include <cstring>
#include <cstddef>

namespace fluidloom {

enum class BackendType {
    MOCK,
    OPENCL
};

enum class MemoryLocation {
    HOST,
    DEVICE
};

// Forward declarations
class DeviceBuffer;  // RAII wrapper
using DeviceBufferPtr = std::unique_ptr<DeviceBuffer>;

class IBackend {
public:
    virtual ~IBackend() = default;

    // --- Device Management ---
    virtual BackendType getType() const = 0;
    virtual std::string getName() const = 0;
    virtual int getDeviceCount() const = 0;
    virtual void initialize(int device_id = 0) = 0;
    virtual void shutdown() = 0;

    // --- Memory Management ---
    virtual DeviceBufferPtr allocateBuffer(size_t size_in_bytes, const void* initial_data = nullptr) = 0;
    
    virtual void copyHostToDevice(const void* host_src, DeviceBuffer& device_dst, size_t size) = 0;
    virtual void copyDeviceToHost(const DeviceBuffer& device_src, void* host_dst, size_t size) = 0;
    virtual void copyDeviceToDevice(const DeviceBuffer& src, DeviceBuffer& dst, size_t size) = 0;

    // --- Synchronization ---
    virtual void flush() = 0;
    virtual void finish() = 0;

    // --- Kernel Management ---
    struct KernelHandle {
        void* handle;
        explicit KernelHandle(void* h) : handle(h) {}
    };

    /**
     * @brief Compile kernel from source file
     * @param source_file Path to .cl file relative to build directory
     * @param kernel_name Name of kernel function
     * @param build_options Optional build flags
     * @return Kernel handle for launch
     */
    virtual KernelHandle compileKernel(
        const std::string& source_file,
        const std::string& kernel_name,
        const std::string& build_options = ""
    ) = 0;

    /**
     * @brief Launch compiled kernel
     * @param kernel Compiled kernel handle
     * @param global_work_size Total number of work-items
     * @param local_work_size Work-group size (0 for auto)
     * @param args Kernel arguments (device pointers and scalars)
     */
    struct KernelArg {
        std::vector<uint8_t> data;
        
        // Helper constructors
        template<typename T>
        static KernelArg fromScalar(const T& val) {
            KernelArg arg;
            arg.data.resize(sizeof(T));
            std::memcpy(arg.data.data(), &val, sizeof(T));
            return arg;
        }
        
        static KernelArg fromBuffer(const void* ptr) {
            KernelArg arg;
            arg.data.resize(sizeof(void*));
            std::memcpy(arg.data.data(), &ptr, sizeof(void*));
            return arg;
        }
    };

    /**
     * @brief Launch compiled kernel
     * @param kernel Compiled kernel handle
     * @param global_work_size Total number of work-items
     * @param local_work_size Work-group size (0 for auto)
     * @param args Kernel arguments
     */
    virtual void launchKernel(
        const KernelHandle& kernel,
        size_t global_work_size,
        size_t local_work_size,
        const std::vector<KernelArg>& args
    ) = 0;

    /**
     * @brief Release kernel resources
     */
    virtual void releaseKernel(const KernelHandle& kernel) = 0;

    // --- Properties ---
    virtual size_t getMaxAllocationSize() const = 0;
    virtual size_t getTotalMemory() const = 0;
    virtual bool isInitialized() const = 0;

protected:
    IBackend() = default;
};

// Abstract base for all device buffers
class DeviceBuffer {
public:
    virtual ~DeviceBuffer() = default;
    virtual void* getDevicePointer() = 0;
    virtual const void* getDevicePointer() const = 0;
    virtual size_t getSize() const = 0;
    virtual MemoryLocation getLocation() const = 0;

protected:
    DeviceBuffer() = default;
};

} // namespace fluidloom
