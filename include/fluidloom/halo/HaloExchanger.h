#pragma once

#include "fluidloom/halo/GhostRangeBuilder.h"
#include "fluidloom/halo/PackBufferLayout.h"
#include "fluidloom/core/fields/SOAFieldManager.h"
#include "fluidloom/core/backend/IBackend.h"
#include "fluidloom/common/mpi/MPIEnvironment.h"
#include <vector>
#include <map>
#include <memory>

namespace fluidloom {
namespace halo {

class HaloExchanger {
public:
    HaloExchanger(
        std::shared_ptr<IBackend> backend,
        std::shared_ptr<fields::SOAFieldManager> field_manager,
        std::shared_ptr<GhostRangeBuilder> ghost_builder
    );
    
    ~HaloExchanger();

    // Initialize exchange buffers based on topology and fields
    void initialize();
    
    // Start async exchange of halo data
    void startExchange();
    
    // Wait for exchange to complete and unpack data
    void finishExchange();
    
protected:
    std::shared_ptr<IBackend> m_backend;
    std::shared_ptr<fields::SOAFieldManager> m_field_manager;
    std::shared_ptr<GhostRangeBuilder> m_ghost_builder;
    
    // MPI Requests
    std::vector<MPI_Request> m_requests;
    
    // Buffers
    struct NeighborBuffers {
        std::vector<uint8_t> send_buffer_host;
        std::vector<uint8_t> recv_buffer_host;
        std::unique_ptr<DeviceBuffer> send_buffer_device;
        std::unique_ptr<DeviceBuffer> recv_buffer_device;
        PackBufferLayout layout;
    };
    
    std::map<int, NeighborBuffers> m_neighbor_buffers; // rank -> buffers
    
    // Kernels
    IBackend::KernelHandle m_pack_kernel;
    IBackend::KernelHandle m_unpack_kernel;
    
    void compileKernels();
    void packData(int rank);
    void unpackData(int rank);
};

} // namespace halo
} // namespace fluidloom
