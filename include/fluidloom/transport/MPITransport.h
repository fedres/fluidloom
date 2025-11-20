#pragma once

#include "fluidloom/transport/GPUAwareBuffer.h"
#include "fluidloom/transport/MPIRequestWrapper.h"
#include "fluidloom/transport/TransportStats.h"
#include "fluidloom/halo/GhostRange.h"
#include "fluidloom/core/backend/IBackend.h"
#include "fluidloom/transport/PeerAccessManager.h"

#ifdef FLUIDLOOM_MPI_ENABLED
#include <mpi.h>
#endif

#include <vector>
#include <memory>
#include <chrono>

namespace fluidloom {
namespace transport {

/**
 * @brief Non-blocking MPI transport with GPU-aware memory support
 * 
 * This class handles all MPI communication for halo exchange. It automatically
 * chooses between:
 * - GPU-aware MPI (zero-copy)
 * - PCIe P2P (clEnqueueCopyBuffer)
 * - Staging through host memory (fallback)
 * 
 * The transport is **context-aware**: it knows which GPUs are on the same node
 * and can use shared memory or P2P instead of MPI for intra-node transfers.
 * 
 * This is the **ONLY** module that calls MPI functions. All other modules
 * must go through this interface to maintain traceability.
 */
class MPITransport {
private:
    IBackend* backend;
    int mpi_rank;
    int mpi_size;
    bool mpi_initialized_here;
    
    // GPU-aware capabilities
    bool gpu_aware_available;
    bool p2p_available;
    std::unique_ptr<PeerAccessManager> peer_manager;
    
    // Outstanding requests (for waitall)
    std::vector<std::unique_ptr<MPIRequestWrapper>> active_requests;
    
    // Statistics
    TransportStats stats;
    
public:
    MPITransport(IBackend* backend);
    ~MPITransport();
    
    // Non-copyable
    MPITransport(const MPITransport&) = delete;
    MPITransport& operator=(const MPITransport&) = delete;
    
    // Initialize MPI (if not already initialized)
    void initialize();
    
    // Finalize MPI (only if we initialized it)
    void finalize();
    
    // Post non-blocking send
    // Returns request wrapper that can be waited on
    std::unique_ptr<MPIRequestWrapper> send_async(
        int target_rank,
        GPUAwareBuffer* buffer,
        size_t offset,
        size_t size_bytes,
        int tag = 0
    );
    
    // Post non-blocking receive
    std::unique_ptr<MPIRequestWrapper> recv_async(
        int source_rank,
        GPUAwareBuffer* buffer,
        size_t offset,
        size_t size_bytes,
        int tag = 0
    );
    
    // Post P2P copy (intra-node, no MPI)
    std::unique_ptr<MPIRequestWrapper> p2p_copy_async(
        cl_device_id src_device,
        cl_device_id dst_device,
        GPUAwareBuffer* src_buffer,
        GPUAwareBuffer* dst_buffer,
        size_t src_offset,
        size_t dst_offset,
        size_t size_bytes
    );
    
    // Wait for all outstanding requests
    void wait_all();
    
    // Test if all requests are complete (non-blocking)
    bool test_all();
    
    // Cancel all outstanding requests
    void cancel_all();
    
    // Get MPI rank and size
    int getRank() const { return mpi_rank; }
    int getSize() const { return mpi_size; }
    
    // Get statistics
    const TransportStats& getStats() const { return stats; }
    TransportStats& getStats() { return stats; }
    void resetStats() { stats.reset(); }
    
    // Barrier (for testing synchronization)
    void barrier();
    
    // All-gather utility for GhostRange exchange
    std::vector<uint8_t> allGather(const void* send_data, size_t send_size, size_t* recv_sizes);
    
private:
    // Helper: get optimal copy method for rank pair
    bool useP2P(int src_rank, int dst_rank) const;
    bool useGPUAwareMPI(int src_rank, int dst_rank) const;
    
    // Helper: create MPI datatype for GPU-aware transfer
    #ifdef FLUIDLOOM_MPI_ENABLED
    MPI_Datatype createGPUAwareDatatype(size_t size_bytes);
    #endif
};

} // namespace transport
} // namespace fluidloom
