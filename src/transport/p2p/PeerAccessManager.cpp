#include "fluidloom/transport/PeerAccessManager.h"
#include "fluidloom/common/Logger.h"
// #include <CL/cl_ext.h> // Not available on all platforms

namespace fluidloom {
namespace transport {

PeerAccessManager::PeerAccessManager(const std::vector<cl_device_id>& gpu_devices) 
    : devices(gpu_devices) {
    
    // Build peer matrix
    for (auto src : devices) {
        for (auto dst : devices) {
            if (src == dst) {
                peer_matrix[{src, dst}] = false; // No self-P2P
                continue;
            }
            
            // Query if devices share the same platform
            cl_platform_id src_platform, dst_platform;
            clGetDeviceInfo(src, CL_DEVICE_PLATFORM, sizeof(cl_platform_id), &src_platform, nullptr);
            clGetDeviceInfo(dst, CL_DEVICE_PLATFORM, sizeof(cl_platform_id), &dst_platform, nullptr);
            
            if (src_platform != dst_platform) {
                peer_matrix[{src, dst}] = false;
                continue;
            }
            
            // Try to create context with both devices and test copy
            cl_int err;
            cl_context_properties props[] = {
                CL_CONTEXT_PLATFORM, (cl_context_properties)src_platform,
                0
            };
            cl_device_id devices_to_test[] = {src, dst};
            cl_context context = clCreateContext(props, 2, devices_to_test, nullptr, nullptr, &err);
            
            if (err != CL_SUCCESS) {
                peer_matrix[{src, dst}] = false;
                continue;
            }
            
            // Allocate test buffers
            cl_mem buf_src = clCreateBuffer(context, CL_MEM_READ_WRITE, 4096, nullptr, &err);
            cl_mem buf_dst = clCreateBuffer(context, CL_MEM_READ_WRITE, 4096, nullptr, &err);
            
            cl_command_queue queue;
            #ifdef __APPLE__
            queue = clCreateCommandQueueWithPropertiesAPPLE(context, src, 0, &err);
            #else
            queue = clCreateCommandQueueWithProperties(context, src, 0, &err);
            #endif
            
            // Try P2P copy
            err = clEnqueueCopyBuffer(queue, buf_src, buf_dst, 0, 0, 4096, 0, nullptr, nullptr);
            clFinish(queue);
            
            peer_matrix[{src, dst}] = (err == CL_SUCCESS);
            
            // Cleanup
            clReleaseMemObject(buf_src);
            clReleaseMemObject(buf_dst);
            clReleaseCommandQueue(queue);
            clReleaseContext(context);
            
            FL_LOG(DEBUG) << "P2P " << src << " -> " << dst << ": " 
                         << (peer_matrix[{src, dst}] ? "YES" : "NO");
        }
    }
}

bool PeerAccessManager::isPeerAccessible(cl_device_id src, cl_device_id dst) const {
    auto it = peer_matrix.find({src, dst});
    return (it != peer_matrix.end()) && it->second;
}

PeerAccessManager::CopyMethod PeerAccessManager::getOptimalMethod(
    cl_device_id src, cl_device_id dst) const {
    
    if (isPeerAccessible(src, dst)) {
        return CopyMethod::P2P;
    }
    
    #ifdef FLUIDLOOM_GPU_AWARE_MPI
    return CopyMethod::GPU_AWARE_MPI;
    #else
    return CopyMethod::STAGING_HOST;
    #endif
}

cl_int PeerAccessManager::enablePeerAccess(cl_device_id src, cl_device_id dst) {
    (void)src; (void)dst; // Suppress unused warnings
    // Platform-specific enablement (may require AMD extension)
    #ifdef CL_MEM_USE_PERSISTENT_MEM_AMD
    if (isPeerAccessible(src, dst)) {
        FL_LOG(INFO) << "Enabling peer access for AMD devices";
        return CL_SUCCESS;
    }
    #endif
    return CL_SUCCESS;
}

void PeerAccessManager::disablePeerAccess(cl_device_id src, cl_device_id dst) {
    (void)src; (void)dst; // Suppress unused warnings
    // No-op for now
}

void PeerAccessManager::printPeerMatrix() const {
    FL_LOG(INFO) << "Peer Access Matrix:";
    for (auto src : devices) {
        for (auto dst : devices) {
            if (src != dst) {
                FL_LOG(INFO) << src << " -> " << dst << ": " 
                             << (isPeerAccessible(src, dst) ? "YES" : "NO");
            }
        }
    }
}

} // namespace transport
} // namespace fluidloom
