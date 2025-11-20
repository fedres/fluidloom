#include "fluidloom/halo/HaloExchanger.h"
#include "fluidloom/common/Logger.h"
#include "fluidloom/core/registry/FieldRegistry.h"
#include <cstring>

namespace fluidloom {
namespace halo {

HaloExchanger::HaloExchanger(
    std::shared_ptr<IBackend> backend,
    std::shared_ptr<fields::SOAFieldManager> field_manager,
    std::shared_ptr<GhostRangeBuilder> ghost_builder
) : m_backend(backend),
    m_field_manager(field_manager),
    m_ghost_builder(ghost_builder),
    m_pack_kernel(nullptr),
    m_unpack_kernel(nullptr)
{
    compileKernels();
}

HaloExchanger::~HaloExchanger() {
    if (m_pack_kernel.handle) m_backend->releaseKernel(m_pack_kernel);
    if (m_unpack_kernel.handle) m_backend->releaseKernel(m_unpack_kernel);
}

void HaloExchanger::compileKernels() {
    try {
        m_pack_kernel = m_backend->compileKernel("kernels/halo_pack.cl", "pack_fields");
        m_unpack_kernel = m_backend->compileKernel("kernels/halo_pack.cl", "unpack_fields");
    } catch (const std::exception& e) {
        FL_LOG(ERROR) << "Failed to compile halo kernels: " << e.what();
        throw;
    }
}

void HaloExchanger::initialize() {
    // For now, we'll assume a simple 1D decomposition where we exchange with prev/next ranks
    // In a real scenario, we'd use GhostRangeBuilder to get candidates
    
    auto& env = mpi::MPIEnvironment::getInstance();
    int rank = env.getRank();
    int size = env.getSize();
    
    m_neighbor_buffers.clear();
    
    // Define neighbors (simplified)
    std::vector<int> neighbors;
    if (rank > 0) neighbors.push_back(rank - 1);
    if (rank < size - 1) neighbors.push_back(rank + 1);
    
    for (int neighbor : neighbors) {
        NeighborBuffers buffers;
        
        // Setup layout based on registered fields
        // In reality, we'd query GhostRangeBuilder for size
        size_t num_cells = 1024; 
        
        auto& registry = registry::FieldRegistry::instance();
        auto field_names = registry.getAllNames();
        
        if (field_names.empty()) {
            // Fallback for testing if no fields registered
            buffers.layout.addField("density", 1, sizeof(float));
        } else {
            for (const auto& name : field_names) {
                auto desc = registry.lookupByName(name);
                if (desc) {
                    buffers.layout.addField(name, desc->num_components, desc->bytesPerCell() / desc->num_components);
                }
            }
        }
        
        buffers.layout.capacity_bytes = num_cells * buffers.layout.cell_size_bytes;
        size_t buffer_size = buffers.layout.capacity_bytes;
        
        // Allocate host buffers
        buffers.send_buffer_host.resize(buffer_size);
        buffers.recv_buffer_host.resize(buffer_size);
        
        // Allocate device buffers
        buffers.send_buffer_device = m_backend->allocateBuffer(buffer_size);
        buffers.recv_buffer_device = m_backend->allocateBuffer(buffer_size);
        
        m_neighbor_buffers[neighbor] = std::move(buffers);
    }
    
    FL_LOG(INFO) << "HaloExchanger initialized with " << m_neighbor_buffers.size() << " neighbors";
}

void HaloExchanger::startExchange() {
    m_requests.clear();
    
    for (auto& [rank, buffers] : m_neighbor_buffers) {
        // 1. Pack data on GPU
        packData(rank);
        
        // 2. Copy to host (async ideally, but sync for now)
        m_backend->copyDeviceToHost(*buffers.send_buffer_device, buffers.send_buffer_host.data(), buffers.layout.capacity_bytes);
        
        // 3. MPI_Isend / MPI_Irecv
        MPI_Request send_req, recv_req;
        
        MPI_Isend(buffers.send_buffer_host.data(), buffers.layout.capacity_bytes, MPI_BYTE, 
                  rank, static_cast<int>(MPITag::GHOST_EXCHANGE), MPI_COMM_WORLD, &send_req);
                  
        MPI_Irecv(buffers.recv_buffer_host.data(), buffers.layout.capacity_bytes, MPI_BYTE,
                  rank, static_cast<int>(MPITag::GHOST_EXCHANGE), MPI_COMM_WORLD, &recv_req);
                  
        m_requests.push_back(send_req);
        m_requests.push_back(recv_req);
    }
}

void HaloExchanger::finishExchange() {
    if (m_requests.empty()) return;
    
    MPI_Waitall(m_requests.size(), m_requests.data(), MPI_STATUSES_IGNORE);
    m_requests.clear();
    
    for (auto& [rank, buffers] : m_neighbor_buffers) {
        // 1. Copy from host to GPU
        m_backend->copyHostToDevice(buffers.recv_buffer_host.data(), *buffers.recv_buffer_device, buffers.layout.capacity_bytes);
        
        // 2. Unpack on GPU
        unpackData(rank);
    }
}

void HaloExchanger::packData(int rank) {
    auto& buffers = m_neighbor_buffers[rank];
    auto& registry = registry::FieldRegistry::instance();
    
    // For each field in layout
    // For each field in layout
    size_t num_cells = (buffers.layout.cell_size_bytes > 0) ? (buffers.layout.capacity_bytes / buffers.layout.cell_size_bytes) : 0;

    for (const auto& field : buffers.layout.fields) {
        auto desc = registry.lookupByName(field.field_name);
        if (!desc) continue;
        
        size_t offset = field.offset_in_cell * num_cells;

        try {
            void* device_ptr = m_field_manager->getDevicePtr(fields::FieldHandle(desc->id));
            
            // Launch pack kernel
            m_backend->launchKernel(m_pack_kernel, num_cells, 0, {
                IBackend::KernelArg::fromBuffer(device_ptr),
                IBackend::KernelArg::fromBuffer(buffers.send_buffer_device->getDevicePointer()),
                IBackend::KernelArg::fromScalar((uint64_t)num_cells),
                IBackend::KernelArg::fromScalar((uint64_t)offset)
            });
        } catch (...) {
            // Field might not be allocated, skip or log
            FL_LOG(WARN) << "Skipping pack for unallocated field: " << field.field_name;
        }
    }
}

void HaloExchanger::unpackData(int rank) {
    auto& buffers = m_neighbor_buffers[rank];
    auto& registry = registry::FieldRegistry::instance();
    
    size_t num_cells = (buffers.layout.cell_size_bytes > 0) ? (buffers.layout.capacity_bytes / buffers.layout.cell_size_bytes) : 0;

    for (const auto& field : buffers.layout.fields) {
        auto desc = registry.lookupByName(field.field_name);
        if (!desc) continue;
        
        size_t offset = field.offset_in_cell * num_cells;
        
        try {
            void* device_ptr = m_field_manager->getDevicePtr(fields::FieldHandle(desc->id));
            
            // Launch unpack kernel
            m_backend->launchKernel(m_unpack_kernel, num_cells, 0, {
                IBackend::KernelArg::fromBuffer(buffers.recv_buffer_device->getDevicePointer()),
                IBackend::KernelArg::fromBuffer(device_ptr),
                IBackend::KernelArg::fromScalar((uint64_t)num_cells),
                IBackend::KernelArg::fromScalar((uint64_t)offset)
            });
        } catch (...) {
             FL_LOG(WARN) << "Skipping unpack for unallocated field: " << field.field_name;
        }
    }
}

} // namespace halo
} // namespace fluidloom
