#include "fluidloom/load_balance/CellMigrator.h"
#include "fluidloom/common/Logger.h"
#include <algorithm>
#include <cstring>

namespace fluidloom {
namespace load_balance {

CellMigrator::CellMigrator(transport::MPITransport* transport, cl_context context, cl_command_queue queue)
    : m_transport(transport), m_context(context), m_queue(queue) {
    
    if (!transport) {
        throw std::invalid_argument("MPITransport cannot be null");
    }
    if (!context || !queue) {
        throw std::invalid_argument("OpenCL context and queue cannot be null");
    }
    
    FL_LOG(INFO) << "CellMigrator initialized";
}

CellMigrator::~CellMigrator() {
    // GPUAwareBuffer handles cleanup automatically
}

void CellMigrator::migrate(
    const MigrationPlan& plan,
    cl_mem* coord_x, cl_mem* coord_y, cl_mem* coord_z,
    cl_mem* levels, cl_mem* cell_states,
    cl_mem* fields, uint32_t num_field_components,
    size_t* num_cells, size_t* capacity
) {
    if (!plan.isValid()) {
        FL_LOG(ERROR) << "Invalid migration plan";
        return;
    }
    
    if (plan.transfers.empty()) {
        FL_LOG(INFO) << "No transfers in migration plan, skipping";
        return;
    }
    
    FL_LOG(INFO) << "Executing migration plan: " << plan.transfers.size() << " transfers, "
                 << plan.total_cells_to_migrate << " cells";
    
    int my_rank = m_transport->getRank();
    std::vector<uint32_t> cells_to_remove;
    size_t cells_to_receive = 0;
    
    // Phase 1: Initiate all sends and receives
    std::vector<std::unique_ptr<transport::MPIRequestWrapper>> requests;
    
    for (const auto& transfer : plan.transfers) {
        if (transfer.source_rank == my_rank) {
            // We are sending cells
            FL_LOG(INFO) << "Sending " << transfer.num_cells << " cells to GPU " << transfer.dest_rank;
            
            // Pack cells into send buffer
            size_t packed_cells = packCells(
                transfer, *coord_x, *coord_y, *coord_z, *levels,
                fields ? *fields : nullptr, num_field_components
            );
            
            if (packed_cells != transfer.num_cells) {
                FL_LOG(WARN) << "Packed " << packed_cells << " cells, expected " << transfer.num_cells;
            }
            
            // Send via MPI (non-blocking)
            size_t buffer_size = packed_cells * (sizeof(int) * 3 + sizeof(uint8_t) + 
                                                 num_field_components * sizeof(float));
            auto req = m_transport->send_async(
                transfer.dest_rank, m_send_buffer.get(), 0, buffer_size, 0
            );
            requests.push_back(std::move(req));
            
            // Mark cells for removal after send completes
            // (In real implementation, would track exact indices)
            cells_to_remove.push_back(static_cast<uint32_t>(packed_cells));
        }
        
        if (transfer.dest_rank == my_rank) {
            // We are receiving cells
            FL_LOG(INFO) << "Receiving " << transfer.num_cells << " cells from GPU " << transfer.source_rank;
            
            cells_to_receive += transfer.num_cells;
            
            // Receive via MPI (non-blocking)
            size_t buffer_size = transfer.num_cells * (sizeof(int) * 3 + sizeof(uint8_t) + 
                                                       num_field_components * sizeof(float));
            auto req = m_transport->recv_async(
                transfer.source_rank, m_recv_buffer.get(), 0, buffer_size, 0
            );
            requests.push_back(std::move(req));
        }
    }
    
    // Phase 2: Wait for all transfers to complete
    FL_LOG(INFO) << "Waiting for " << requests.size() << " MPI operations to complete";
    m_transport->wait_all();
    
    // Phase 3: Unpack received cells
    if (cells_to_receive > 0) {
        FL_LOG(INFO) << "Unpacking " << cells_to_receive << " received cells";
        
        // Ensure capacity for new cells
        size_t required_capacity = *num_cells + cells_to_receive;
        if (required_capacity > *capacity) {
            ensureCapacity(required_capacity, coord_x, coord_y, coord_z, levels, cell_states,
                          fields, num_field_components, capacity);
        }
        
        unpackCells(cells_to_receive, coord_x, coord_y, coord_z, levels, fields,
                   num_field_components, num_cells);
    }
    
    // Phase 4: Remove migrated cells
    if (!cells_to_remove.empty()) {
        FL_LOG(INFO) << "Compacting after removing " << cells_to_remove.size() << " migrated cells";
        compactAfterMigration(cells_to_remove, coord_x, coord_y, coord_z, levels, fields,
                             num_field_components, num_cells);
    }
    
    FL_LOG(INFO) << "Migration complete: final cell count = " << *num_cells;
}

size_t CellMigrator::packCells(
    const MigrationPlan::Transfer& transfer,
    cl_mem coord_x, cl_mem coord_y, cl_mem coord_z,
    cl_mem levels, cl_mem fields,
    uint32_t num_field_components
) {
    // Simplified packing: read cells from GPU and pack into send buffer
    // Real implementation would use pack kernels from Module 7
    
    size_t num_cells = transfer.num_cells;
    size_t bytes_per_cell = sizeof(int) * 3 + sizeof(uint8_t) + 
                           num_field_components * sizeof(float);
    size_t total_bytes = num_cells * bytes_per_cell;
    
    // Allocate send buffer if needed
    if (!m_send_buffer || m_send_buffer->getSize() < total_bytes) {
        m_send_buffer = std::make_unique<transport::GPUAwareBuffer>(
            m_context, m_queue, total_bytes
        );
    }
    
    // For now, just allocate the buffer
    // Real implementation would:
    // 1. Identify cells in Hilbert range [transfer.hilbert_start, transfer.hilbert_end)
    // 2. Pack coordinates, levels, and field data
    // 3. Write to send buffer
    
    FL_LOG(INFO) << "Packed " << num_cells << " cells (" << total_bytes << " bytes)";
    
    return num_cells;
}

void CellMigrator::unpackCells(
    size_t num_cells_received,
    cl_mem* coord_x, cl_mem* coord_y, cl_mem* coord_z,
    cl_mem* levels, cl_mem* fields,
    uint32_t num_field_components,
    size_t* total_cells
) {
    // Simplified unpacking: append received cells to arrays
    // Real implementation would:
    // 1. Read from receive buffer
    // 2. Insert cells in Hilbert order (maintain sorted order)
    // 3. Update field data
    
    FL_LOG(INFO) << "Unpacked " << num_cells_received << " cells";
    
    *total_cells += num_cells_received;
}

void CellMigrator::compactAfterMigration(
    const std::vector<uint32_t>& migrated_indices,
    cl_mem* coord_x, cl_mem* coord_y, cl_mem* coord_z,
    cl_mem* levels, cl_mem* fields,
    uint32_t num_field_components,
    size_t* num_cells
) {
    // Simplified compaction: reduce cell count
    // Real implementation would:
    // 1. Mark migrated cells as invalid
    // 2. Run compaction kernel (similar to Module 11's compaction)
    // 3. Update cell count
    
    size_t cells_removed = 0;
    for (uint32_t count : migrated_indices) {
        cells_removed += count;
    }
    
    if (cells_removed > *num_cells) {
        FL_LOG(ERROR) << "Attempting to remove more cells than exist";
        cells_removed = *num_cells;
    }
    
    *num_cells -= cells_removed;
    
    FL_LOG(INFO) << "Removed " << cells_removed << " cells, new count = " << *num_cells;
}

void CellMigrator::ensureCapacity(
    size_t required_capacity,
    cl_mem* coord_x, cl_mem* coord_y, cl_mem* coord_z,
    cl_mem* levels, cl_mem* cell_states,
    cl_mem* fields, uint32_t num_field_components,
    size_t* capacity
) {
    if (required_capacity <= *capacity) {
        return;  // Already have enough capacity
    }
    
    // Grow by 1.5x
    size_t new_capacity = static_cast<size_t>(required_capacity * 1.5);
    
    FL_LOG(INFO) << "Growing capacity from " << *capacity << " to " << new_capacity;
    
    // Reallocate buffers
    // Real implementation would:
    // 1. Allocate new larger buffers
    // 2. Copy existing data
    // 3. Release old buffers
    // 4. Update pointers
    
    *capacity = new_capacity;
}

} // namespace load_balance
} // namespace fluidloom
