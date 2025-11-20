#include <thread>
#include "fluidloom/halo/communicators/HaloExchangeManager.h"
#include "fluidloom/common/Logger.h"
#include "fluidloom/core/registry/FieldRegistry.h"

namespace fluidloom {
namespace halo {

HaloExchangeManager::HaloExchangeManager(IBackend* backend, const registry::FieldRegistry& registry)
    : backend(backend), field_registry(registry), using_buffer_a(true) {
    (void)field_registry; // Suppress unused warning
    
    // Initialize transport
    mpi_transport = std::make_unique<transport::MPITransport>(backend);
    mpi_bridge = std::make_unique<transport::MPIEventBridge>(backend);
    
    // Keep existing components for now, but we might bypass DoubleBufferController for pack buffers
    buffer_controller = std::make_unique<DoubleBufferController>(backend, 64);
    pack_kernel = std::make_unique<HaloPackKernel>(backend);
    unpack_kernel = std::make_unique<HaloUnpackKernel>(backend);
    event_chain = std::make_unique<EventChain>();
    interpolator = std::make_unique<TrilinearInterpolator>(backend);
}

void HaloExchangeManager::initialize(size_t buffer_capacity_mb) {
    // Initialize GPU-aware buffers
    size_t bytes = buffer_capacity_mb * 1024 * 1024;
    gpu_pack_buffer_a = transport::createGPUAwareBuffer(backend, bytes);
    gpu_pack_buffer_b = transport::createGPUAwareBuffer(backend, bytes);
    
    pack_kernel->initialize();
    unpack_kernel->initialize();
    interpolator->initializeLookupTable();
    
    FL_LOG(INFO) << "HaloExchangeManager initialized with " << buffer_capacity_mb << "MB GPU-aware buffers";
}

void HaloExchangeManager::addGhostRange(const GhostRange& range) {
    ghost_ranges.push_back(range);
}

void HaloExchangeManager::exchangeAsync() {
    stats.num_exchanges++;
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Choose which pack buffer to use
    auto* pack_buffer = using_buffer_a ? gpu_pack_buffer_a.get() : gpu_pack_buffer_b.get();
    
    // 1. Launch Pack Kernels
    // We need to iterate over fields. For now, assume we have a list or query registry.
    // Since FieldRegistry is passed, we should iterate over registered fields.
    // But for this implementation, we'll assume a "halo_fields" list or similar if available,
    // or just use a placeholder loop if the registry doesn't expose iteration easily.
    // Module 7 tests used manual field addition.
    // Let's assume we pack all fields in the registry that are marked for halo exchange.
    // For now, we'll just use a dummy field loop or assume the user added fields to a list.
    // The plan code used `halo_fields`. I'll assume it's a member I missed or I should add it.
    // I'll add `std::vector<std::string> halo_fields;` to the class in a separate step if needed.
    // For now, I'll just comment the field iteration.
    
    // Mock field iteration
    std::vector<std::string> fields = {"density", "velocity"}; 
    
    for (const auto& field_name : fields) {
        (void)field_name; // Suppress unused warning
        // Get field descriptor...
        // pack_kernel->execute(...)
        // We need to adapt pack_kernel to accept GPUAwareBuffer's device buffer.
        // pack_buffer->device_buffer is a Buffer struct.
        
        for (const auto& range : ghost_ranges) {
            (void)range; // Suppress unused warning
             // pack_kernel->execute(..., pack_buffer->device_buffer, ...);
        }
    }
    
    // Record pack completion event
    // cl_event pack_event = backend->getLastEvent(); // Assuming backend tracks it
    // For now, we just wait for queue to finish packing before sending (simple synchronization)
    // or use events if backend supports it.
    backend->finish(); // Simple sync for now to ensure packing is done
    
    auto pack_end = std::chrono::high_resolution_clock::now();
    stats.pack_time_ms = std::chrono::duration<double, std::milli>(pack_end - start_time).count();
    
    // 2. Post MPI Operations (Send/Recv)
    for (const auto& range : ghost_ranges) {
        // Send to target_gpu
        if (range.pack_size_bytes > 0) {
            auto send_req = mpi_transport->send_async(
                range.target_gpu,
                pack_buffer,
                range.pack_offset,
                range.pack_size_bytes,
                range.hilbert_start & 0xFFFFFFFF // Tag
            );
            active_requests.push_back(std::move(send_req));
        }
        
        // Receive from target_gpu (use other buffer for recv)
        auto* recv_buffer = using_buffer_a ? gpu_pack_buffer_b.get() : gpu_pack_buffer_a.get();
        auto recv_req = mpi_transport->recv_async(
            range.target_gpu,
            recv_buffer,
            range.pack_offset,
            range.pack_size_bytes,
            range.hilbert_start & 0xFFFFFFFF // Tag
        );
        active_requests.push_back(std::move(recv_req));
    }
}

void HaloExchangeManager::waitCompletion() {
    // Wait for all MPI requests
    // We iterate and wait on wrappers
    for (auto& req : active_requests) {
        req->wait();
    }
    active_requests.clear();
    
    // Launch Unpack Kernels
    auto* recv_buffer = using_buffer_a ? gpu_pack_buffer_b.get() : gpu_pack_buffer_a.get();
    (void)recv_buffer; // Suppress unused warning
    
    // Iterate fields and unpack
    // for (const auto& field : fields) ...
    // unpack_kernel->execute(..., recv_buffer->device_buffer, ...);
    
    backend->finish();
}

void HaloExchangeManager::swapBuffers() {
    using_buffer_a = !using_buffer_a;
}

// Stub for postMpiOperations if needed by interface, but we inlined it
void HaloExchangeManager::postMpiOperations() {}

} // namespace halo
} // namespace fluidloom
