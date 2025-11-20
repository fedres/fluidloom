#pragma once

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif
#include <vector>
#include <unordered_map>

namespace fluidloom {
namespace transport {

/**
 * @brief Detects and manages PCIe peer access capabilities between GPU devices
 * 
 * This class runs once at initialization and builds a peer matrix:
 * peer_accessible[dev_a][dev_b] = true if clEnqueueCopyBuffer works without staging.
 * 
 * The detection is CRITICAL for performance: P2P copies can be 2-3x faster than
 * device->host->device staging.
 */
class PeerAccessManager {
private:
    struct DevicePair {
        cl_device_id src;
        cl_device_id dst;
        bool operator==(const DevicePair& other) const {
            return src == other.src && dst == other.dst;
        }
    };
    
    struct DevicePairHash {
        size_t operator()(const DevicePair& dp) const noexcept {
            return std::hash<void*>{}(dp.src) ^ (std::hash<void*>{}(dp.dst) << 1);
        }
    };
    
    std::unordered_map<DevicePair, bool, DevicePairHash> peer_matrix;
    std::vector<cl_device_id> devices;
    
public:
    explicit PeerAccessManager(const std::vector<cl_device_id>& gpu_devices);
    ~PeerAccessManager() = default;
    
    // Check if direct P2P copy is possible
    bool isPeerAccessible(cl_device_id src, cl_device_id dst) const;
    
    // Get optimal copy method for a transfer
    enum class CopyMethod { P2P, GPU_AWARE_MPI, STAGING_HOST };
    CopyMethod getOptimalMethod(cl_device_id src, cl_device_id dst) const;
    
    // Initialize peer access (may require clEnqueueUnmapMemObject calls)
    cl_int enablePeerAccess(cl_device_id src, cl_device_id dst);
    
    // Disable peer access (cleanup)
    void disablePeerAccess(cl_device_id src, cl_device_id dst);
    
    // Debug output
    void printPeerMatrix() const;
};

} // namespace transport
} // namespace fluidloom
