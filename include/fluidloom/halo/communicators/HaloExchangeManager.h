#pragma once

#include "fluidloom/core/backend/IBackend.h"
#include "fluidloom/halo/GhostRange.h"
#include "fluidloom/halo/PackBufferLayout.h"
#include "fluidloom/halo/buffers/DoubleBufferController.h"
#include "fluidloom/halo/packers/HaloPackKernel.h"
#include "fluidloom/halo/packers/HaloUnpackKernel.h"
#include "fluidloom/halo/events/EventChain.h"
#include "fluidloom/halo/interpolation/TrilinearInterpolator.h"
#include <memory>
#include <vector>
#include "fluidloom/transport/MPITransport.h"
#include "fluidloom/transport/MPIEventBridge.h"
#include "fluidloom/transport/GPUAwareBuffer.h"

namespace fluidloom {
namespace halo {

/**
 * @brief Orchestrates complete halo exchange cycle: pack → communicate → unpack
 * 
 * Usage pattern:
 *   1. exchange_async() → launches pack kernels, posts non-blocking sends/recvs
 *   2. Wait for compute to complete (external)
 *   3. wait_completion() → blocks until unpack finished
 *   4. swap_buffers() → prepare for next iteration
 */
class HaloExchangeManager {
public:
    HaloExchangeManager(IBackend* backend, const registry::FieldRegistry& registry);
    ~HaloExchangeManager() = default;
    
    // Initialize kernels and buffers
    void initialize(size_t buffer_capacity_mb = 64);
    
    // Register a ghost range to manage
    void addGhostRange(const GhostRange& range);
    
    // Start async exchange cycle
    void exchangeAsync();
    
    // Wait for exchange to complete (includes unpack)
    void waitCompletion();
    
    // Prepare for next cycle
    void swapBuffers();
    
    // Statistics
    struct Stats {
        size_t bytes_exchanged{0};
        double pack_time_ms{0.0};
        double comm_time_ms{0.0};
        double unpack_time_ms{0.0};
        size_t num_exchanges{0};
    };
    Stats getStats() const { return stats; }
    
private:
    IBackend* backend;
    const registry::FieldRegistry& field_registry;
    


// Add to private members
private:
    std::unique_ptr<transport::MPITransport> mpi_transport;
    std::unique_ptr<transport::MPIEventBridge> mpi_bridge;
    
    // Override pack buffers with GPU-aware versions
    std::unique_ptr<transport::GPUAwareBuffer> gpu_pack_buffer_a;
    std::unique_ptr<transport::GPUAwareBuffer> gpu_pack_buffer_b;
    
    // Current buffer index (for double buffering)
    bool using_buffer_a;
    
    // Request tracking for wait
    std::vector<std::unique_ptr<transport::MPIRequestWrapper>> active_requests;
    
    // Components
    std::unique_ptr<DoubleBufferController> buffer_controller;
    std::unique_ptr<HaloPackKernel> pack_kernel;
    std::unique_ptr<HaloUnpackKernel> unpack_kernel;
    std::unique_ptr<EventChain> event_chain;
    std::unique_ptr<TrilinearInterpolator> interpolator;
    
    // Data
    std::vector<GhostRange> ghost_ranges;
    
    // Stats
    Stats stats;
    
    // Helper to post MPI operations
    void postMpiOperations();
};

} // namespace halo
} // namespace fluidloom
