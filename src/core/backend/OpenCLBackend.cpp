#include "fluidloom/core/backend/OpenCLBackend.h"
#include "fluidloom/common/FluidLoomError.h"
#include "fluidloom/common/Logger.h"
#include <algorithm>
#include <cstring>
#include <fstream>

namespace fluidloom {

OpenCLBuffer::OpenCLBuffer(cl_mem buffer, size_t size, cl_context context)
    : m_buffer(buffer), m_size(size) {
    (void)context; // Unused
}

OpenCLBuffer::~OpenCLBuffer() {
    if (m_buffer) {
        cl_int err = clReleaseMemObject(m_buffer);
        if (err != CL_SUCCESS) {
            // Cannot throw in destructor, log instead
            FL_LOG(ERROR) << "Failed to release OpenCL buffer: " << err;
        }
    }
}

OpenCLBackend::OpenCLBackend() 
    : m_initialized(false), m_platform(nullptr), m_device(nullptr), 
      m_context(nullptr), m_queue(nullptr) {
    FL_LOG(INFO) << "OpenCLBackend created";
}

OpenCLBackend::~OpenCLBackend() {
    if (m_initialized) {
        shutdown();
    }
}

void OpenCLBackend::initialize(int device_id) {
    if (m_initialized) {
        FL_THROW(BackendError, "OpenCLBackend already initialized");
    }
    
    cl_int err;
    
    // Get platform
    err = clGetPlatformIDs(1, &m_platform, nullptr);
    checkError(err, "Failed to get OpenCL platform");
    
    // Get all GPU devices
    cl_uint device_count;
    err = clGetDeviceIDs(m_platform, CL_DEVICE_TYPE_GPU, 0, nullptr, &device_count);
    checkError(err, "Failed to get device count");
    
    if (device_count == 0) {
        FL_THROW(BackendError, "No OpenCL GPU devices found");
    }
    
    if (device_id >= static_cast<int>(device_count)) {
        FL_THROW(BackendError, "Device ID " + std::to_string(device_id) + 
                 " out of range (0-" + std::to_string(device_count - 1) + ")");
    }
    
    m_all_devices.resize(device_count);
    err = clGetDeviceIDs(m_platform, CL_DEVICE_TYPE_GPU, device_count, 
                         m_all_devices.data(), nullptr);
    checkError(err, "Failed to get device list");
    
    m_device = m_all_devices[device_id];
    
    // Create context
    cl_context_properties props[] = {
        CL_CONTEXT_PLATFORM, (cl_context_properties)m_platform,
        0
    };
    m_context = clCreateContext(props, 1, &m_device, nullptr, nullptr, &err);
    checkError(err, "Failed to create OpenCL context");
    
    // Create command queue
    m_queue = clCreateCommandQueue(m_context, m_device, 0, &err);
    checkError(err, "Failed to create command queue");
    
    m_initialized = true;
    FL_LOG(INFO) << "OpenCLBackend initialized (device " << device_id << ")";
    
    queryDeviceInfo();
}

void OpenCLBackend::shutdown() {
    if (!m_initialized) {
        FL_THROW(BackendError, "OpenCLBackend not initialized");
    }
    
    if (m_queue) {
        clReleaseCommandQueue(m_queue);
        m_queue = nullptr;
    }
    
    if (m_context) {
        clReleaseContext(m_context);
        m_context = nullptr;
    }
    
    m_initialized = false;
    FL_LOG(INFO) << "OpenCLBackend shut down";
}

DeviceBufferPtr OpenCLBackend::allocateBuffer(size_t size, const void* initial_data) {
    if (!m_initialized) {
        FL_THROW(BackendError, "Cannot allocate: OpenCLBackend not initialized");
    }
    
    if (size == 0) {
        FL_THROW(BackendError, "Cannot allocate zero-sized buffer");
    }
    
    cl_int err;
    cl_mem_flags flags = CL_MEM_READ_WRITE;
    if (initial_data) {
        flags |= CL_MEM_COPY_HOST_PTR;
    }
    
    cl_mem buffer = clCreateBuffer(m_context, flags, size, 
                                    const_cast<void*>(initial_data), &err);
    checkError(err, "Failed to allocate OpenCL buffer of size " + std::to_string(size));
    
    auto ptr = std::make_unique<OpenCLBuffer>(buffer, size, m_context);
    FL_LOG(DEBUG) << "OpenCLBackend allocated " << size << " bytes";
    return ptr;
}

void OpenCLBackend::copyHostToDevice(const void* host_src, DeviceBuffer& device_dst, size_t size) {
    if (!host_src) {
        FL_THROW(BackendError, "Host source pointer is null");
    }
    
    auto& cl_dst = dynamic_cast<OpenCLBuffer&>(device_dst);
    cl_int err = clEnqueueWriteBuffer(m_queue, cl_dst.getCLMem(), CL_TRUE, 0, 
                                      size, host_src, 0, nullptr, nullptr);
    checkError(err, "Failed H2D copy");
    
    FL_LOG(DEBUG) << "OpenCLBackend H2D copy: " << size << " bytes";
}

void OpenCLBackend::copyDeviceToHost(const DeviceBuffer& device_src, void* host_dst, size_t size) {
    if (!host_dst) {
        FL_THROW(BackendError, "Host destination pointer is null");
    }
    
    const auto& cl_src = dynamic_cast<const OpenCLBuffer&>(device_src);
    cl_int err = clEnqueueReadBuffer(m_queue, cl_src.getCLMem(), CL_TRUE, 0, 
                                     size, host_dst, 0, nullptr, nullptr);
    checkError(err, "Failed D2H copy");
    
    FL_LOG(DEBUG) << "OpenCLBackend D2H copy: " << size << " bytes";
}

void OpenCLBackend::copyDeviceToDevice(const DeviceBuffer& src, DeviceBuffer& dst, size_t size) {
    const auto& cl_src = dynamic_cast<const OpenCLBuffer&>(src);
    auto& cl_dst = dynamic_cast<OpenCLBuffer&>(dst);
    
    cl_int err = clEnqueueCopyBuffer(m_queue, cl_src.getCLMem(), cl_dst.getCLMem(), 
                                     0, 0, size, 0, nullptr, nullptr);
    checkError(err, "Failed D2D copy");
    
    FL_LOG(DEBUG) << "OpenCLBackend D2D copy: " << size << " bytes";
}

void OpenCLBackend::flush() {
    if (!m_initialized) return;
    clFlush(m_queue);
}

void OpenCLBackend::finish() {
    if (!m_initialized) return;
    clFinish(m_queue);
}

size_t OpenCLBackend::getMaxAllocationSize() const {
    if (!m_initialized) return 0;
    
    cl_ulong max_alloc;
    cl_int err = clGetDeviceInfo(m_device, CL_DEVICE_MAX_MEM_ALLOC_SIZE, 
                                 sizeof(max_alloc), &max_alloc, nullptr);
    if (err != CL_SUCCESS) return 0;
    return static_cast<size_t>(max_alloc);
}

size_t OpenCLBackend::getTotalMemory() const {
    if (!m_initialized) return 0;
    
    cl_ulong total_mem;
    cl_int err = clGetDeviceInfo(m_device, CL_DEVICE_GLOBAL_MEM_SIZE, 
                                 sizeof(total_mem), &total_mem, nullptr);
    if (err != CL_SUCCESS) return 0;
    return static_cast<size_t>(total_mem);
}

void OpenCLBackend::checkError(cl_int error, const std::string& operation) {
    if (error != CL_SUCCESS) {
        FL_THROW_OPENCL(error, operation);
    }
}

void OpenCLBackend::queryDeviceInfo() {
    char device_name[256];
    cl_int err = clGetDeviceInfo(m_device, CL_DEVICE_NAME, sizeof(device_name), device_name, nullptr);
    if (err == CL_SUCCESS) {
        FL_LOG(INFO) << "OpenCL device: " << device_name;
    }
    
    size_t max_work_group_size;
    err = clGetDeviceInfo(m_device, CL_DEVICE_MAX_WORK_GROUP_SIZE, 
                          sizeof(max_work_group_size), &max_work_group_size, nullptr);
    if (err == CL_SUCCESS) {
        FL_LOG(INFO) << "  Max work group size: " << max_work_group_size;
    }
    
    // Retrieve and log max allocation size and global memory
    cl_ulong max_alloc;
    err = clGetDeviceInfo(m_device, CL_DEVICE_MAX_MEM_ALLOC_SIZE, 
                                 sizeof(max_alloc), &max_alloc, nullptr);
    if (err == CL_SUCCESS) {
        FL_LOG(INFO) << "  Max allocation size: " << (max_alloc / (1024 * 1024)) << " MB";
    }

    cl_ulong global_mem;
    err = clGetDeviceInfo(m_device, CL_DEVICE_GLOBAL_MEM_SIZE, 
                                 sizeof(global_mem), &global_mem, nullptr);
    if (err == CL_SUCCESS) {
        FL_LOG(INFO) << "  Global memory: " << (global_mem / (1024 * 1024 * 1024)) << " GB";
    }
}

// Kernel management implementation
IBackend::KernelHandle OpenCLBackend::compileKernel(
    const std::string& source_file,
    const std::string& kernel_name,
    const std::string& build_options
) {
    if (!m_initialized) {
        throw std::runtime_error("OpenCLBackend not initialized");
    }
    
    // Check if program already compiled
    if (m_programs.find(source_file) == m_programs.end()) {
        // Load source from file
        std::ifstream file(source_file);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open kernel source: " + source_file);
        }
        
        std::string source((std::istreambuf_iterator<char>(file)),
                          std::istreambuf_iterator<char>());
        file.close();
        
        const char* source_str = source.c_str();
        size_t source_len = source.length();
        
        // Create program
        cl_int err;
        cl_program program = clCreateProgramWithSource(m_context, 1, &source_str, &source_len, &err);
        checkError(err, "Failed to create program");
        
        // Build program
        err = clBuildProgram(program, 1, &m_device, build_options.c_str(), nullptr, nullptr);
        if (err != CL_SUCCESS) {
            // Get build log
            size_t log_size;
            clGetProgramBuildInfo(program, m_device, CL_PROGRAM_BUILD_LOG, 0, nullptr, &log_size);
            std::vector<char> log(log_size);
            clGetProgramBuildInfo(program, m_device, CL_PROGRAM_BUILD_LOG, log_size, log.data(), nullptr);
            
            FL_LOG(ERROR) << "Kernel build failed for " << source_file << ":\n" << log.data();
            clReleaseProgram(program);
            throw std::runtime_error("Kernel build failed");
        }
        
        m_programs[source_file] = program;
        FL_LOG(INFO) << "Compiled kernel source: " << source_file;
    }
    
    // Create kernel
    cl_int err;
    cl_kernel kernel = clCreateKernel(m_programs[source_file], kernel_name.c_str(), &err);
    checkError(err, "Failed to create kernel: " + kernel_name);
    
    void* handle = static_cast<void*>(kernel);
    m_kernels[handle] = kernel;
    
    FL_LOG(INFO) << "Created kernel: " << kernel_name;
    
    return KernelHandle(handle);
}

void OpenCLBackend::launchKernel(
    const KernelHandle& kernel,
    size_t global_work_size,
    size_t local_work_size,
    const std::vector<KernelArg>& args
) {
    if (!m_initialized) {
        throw std::runtime_error("OpenCLBackend not initialized");
    }
    
    auto it = m_kernels.find(kernel.handle);
    if (it == m_kernels.end()) {
        throw std::runtime_error("Invalid kernel handle");
    }
    
    cl_kernel cl_kern = it->second;
    
    // Set arguments
    for (size_t i = 0; i < args.size(); ++i) {
        cl_int err = clSetKernelArg(cl_kern, static_cast<cl_uint>(i), args[i].data.size(), args[i].data.data());
        checkError(err, "Failed to set kernel arg " + std::to_string(i));
    }
    
    // Launch kernel
    // Launch kernel
    const size_t* local_ptr = (local_work_size == 0) ? nullptr : &local_work_size;
    cl_int err = clEnqueueNDRangeKernel(m_queue, cl_kern, 1, nullptr, &global_work_size, local_ptr, 0, nullptr, nullptr);
    checkError(err, "Failed to launch kernel");
}

void OpenCLBackend::releaseKernel(const KernelHandle& kernel) {
    auto it = m_kernels.find(kernel.handle);
    if (it != m_kernels.end()) {
        clReleaseKernel(it->second);
        m_kernels.erase(it);
    }
}

std::string OpenCLBackend::getName() const {
    if (!m_initialized) return "OpenCLBackend (uninitialized)";
    
    char device_name[256];
    cl_int err = clGetDeviceInfo(m_device, CL_DEVICE_NAME, sizeof(device_name), device_name, nullptr);
    if (err != CL_SUCCESS) {
        return "OpenCLBackend (unknown device)";
    }
    return std::string("OpenCL: ") + device_name;
}

int OpenCLBackend::getDeviceCount() const {
    if (m_initialized) {
        return static_cast<int>(m_all_devices.size());
    }
    
    cl_uint num_platforms;
    cl_int err = clGetPlatformIDs(0, nullptr, &num_platforms);
    if (err != CL_SUCCESS || num_platforms == 0) return 0;
    
    std::vector<cl_platform_id> platforms(num_platforms);
    err = clGetPlatformIDs(num_platforms, platforms.data(), nullptr);
    if (err != CL_SUCCESS) return 0;
    
    // Use first platform as in initialize()
    cl_uint num_devices;
    err = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_GPU, 0, nullptr, &num_devices);
    if (err != CL_SUCCESS) return 0;
    
    return static_cast<int>(num_devices);
}

} // namespace fluidloom
