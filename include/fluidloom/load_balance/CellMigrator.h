#pragma once

#include "fluidloom/load_balance/MigrationPlan.h"
#include "fluidloom/transport/MPITransport.h"
#include "fluidloom/transport/GPUAwareBuffer.h"

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include <vector>
#include <memory>
#include <cstdint>

namespace fluidloom {
namespace load_balance {

/**
 * @brief Executes cell migration between GPUs according to a migration plan
 * 
 * Uses existing pack/unpack kernels from Module 7 and MPITransport from Module 8
 * to transfer cells while preserving Hilbert order and field data.
 */
class CellMigrator {
public:
    /**
     * @brief Initialize cell migrator
     * @param transport MPI transport layer
     * @param context OpenCL context
     * @param queue OpenCL command queue
     */
    CellMigrator(transport::MPITransport* transport, cl_context context, cl_command_queue queue);
    
    ~CellMigrator();
    
    // Non-copyable
    CellMigrator(const CellMigrator&) = delete;
    CellMigrator& operator=(const CellMigrator&) = delete;
    
    /**
     * @brief Execute migration plan
     * 
     * @param plan Migration plan describing all transfers
     * @param coord_x X coordinates buffer (in/out)
     * @param coord_y Y coordinates buffer (in/out)
     * @param coord_z Z coordinates buffer (in/out)
     * @param levels Refinement levels buffer (in/out)
     * @param cell_states Cell states buffer (in/out)
     * @param fields Field data buffers (in/out)
     * @param num_field_components Number of components per field
     * @param num_cells Current number of cells (in/out)
     * @param capacity Buffer capacity (in/out)
     */
    void migrate(
        const MigrationPlan& plan,
        cl_mem* coord_x, cl_mem* coord_y, cl_mem* coord_z,
        cl_mem* levels, cl_mem* cell_states,
        cl_mem* fields, uint32_t num_field_components,
        size_t* num_cells, size_t* capacity
    );
    
private:
    transport::MPITransport* m_transport;
    cl_context m_context;
    cl_command_queue m_queue;
    
    // Temporary buffers for migration
    std::unique_ptr<transport::GPUAwareBuffer> m_send_buffer;
    std::unique_ptr<transport::GPUAwareBuffer> m_recv_buffer;
    
    /**
     * @brief Pack cells to migrate into send buffer
     * 
     * @param transfer Transfer descriptor
     * @param coord_x, coord_y, coord_z Coordinate buffers
     * @param levels Level buffer
     * @param fields Field data buffer
     * @param num_field_components Number of field components
     * @return Number of cells packed
     */
    size_t packCells(
        const MigrationPlan::Transfer& transfer,
        cl_mem coord_x, cl_mem coord_y, cl_mem coord_z,
        cl_mem levels, cl_mem fields,
        uint32_t num_field_components
    );
    
    /**
     * @brief Unpack received cells and insert in Hilbert order
     * 
     * @param num_cells_received Number of cells in receive buffer
     * @param coord_x, coord_y, coord_z Coordinate buffers (out)
     * @param levels Level buffer (out)
     * @param fields Field data buffer (out)
     * @param num_field_components Number of field components
     * @param total_cells Current total cell count (in/out)
     */
    void unpackCells(
        size_t num_cells_received,
        cl_mem* coord_x, cl_mem* coord_y, cl_mem* coord_z,
        cl_mem* levels, cl_mem* fields,
        uint32_t num_field_components,
        size_t* total_cells
    );
    
    /**
     * @brief Remove migrated cells from local arrays
     * 
     * @param migrated_indices Indices of cells that were migrated
     * @param coord_x, coord_y, coord_z Coordinate buffers (in/out)
     * @param levels Level buffer (in/out)
     * @param fields Field data buffer (in/out)
     * @param num_field_components Number of field components
     * @param num_cells Current cell count (in/out)
     */
    void compactAfterMigration(
        const std::vector<uint32_t>& migrated_indices,
        cl_mem* coord_x, cl_mem* coord_y, cl_mem* coord_z,
        cl_mem* levels, cl_mem* fields,
        uint32_t num_field_components,
        size_t* num_cells
    );
    
    /**
     * @brief Ensure buffers have sufficient capacity
     * 
     * @param required_capacity Required capacity
     * @param coord_x, coord_y, coord_z Coordinate buffers (in/out)
     * @param levels Level buffer (in/out)
     * @param cell_states Cell states buffer (in/out)
     * @param fields Field data buffer (in/out)
     * @param num_field_components Number of field components
     * @param capacity Current capacity (in/out)
     */
    void ensureCapacity(
        size_t required_capacity,
        cl_mem* coord_x, cl_mem* coord_y, cl_mem* coord_z,
        cl_mem* levels, cl_mem* cell_states,
        cl_mem* fields, uint32_t num_field_components,
        size_t* capacity
    );
};

} // namespace load_balance
} // namespace fluidloom
