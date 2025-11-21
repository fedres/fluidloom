#include "fluidloom/runtime/nodes/RebalanceMeshNode.h"
#include "fluidloom/common/Logger.h"

namespace fluidloom {
namespace runtime {
namespace nodes {

RebalanceMeshNode::RebalanceMeshNode(
    std::string name,
    load_balance::LoadBalancer* balancer,
    load_balance::CellMigrator* migrator
) : ExecutionNode(std::move(name)), m_balancer(balancer), m_migrator(migrator) {
    
    if (!balancer || !migrator) {
        throw std::invalid_argument("LoadBalancer and CellMigrator cannot be null");
    }
    
    FL_LOG(INFO) << "RebalanceMeshNode created: " << node_name;
}

void RebalanceMeshNode::bindMesh(
    cl_mem* coord_x, cl_mem* coord_y, cl_mem* coord_z,
    cl_mem* levels, cl_mem* cell_states,
    cl_mem* fields, uint32_t num_field_components,
    size_t* num_cells, size_t* capacity
) {
    m_coord_x = coord_x;
    m_coord_y = coord_y;
    m_coord_z = coord_z;
    m_levels = levels;
    m_cell_states = cell_states;
    m_fields = fields;
    m_num_field_components = num_field_components;
    m_num_cells = num_cells;
    m_capacity = capacity;
    
    FL_LOG(INFO) << "RebalanceMeshNode bound to mesh with " << *num_cells << " cells";
}

cl_event RebalanceMeshNode::execute(cl_event wait_event) {
    if (!m_num_cells) {
        FL_LOG(ERROR) << "RebalanceMeshNode: mesh not bound";
        return nullptr;
    }
    
    // Increment timestep counter
    m_balancer->incrementTimestep();
    
    // Gather cell counts from all GPUs
    auto cell_counts = m_balancer->gatherCellCounts(*m_num_cells);
    
    // Check if rebalancing is needed
    if (!m_balancer->shouldRebalance(cell_counts)) {
        FL_LOG(INFO) << "Rebalancing not needed (imbalance below threshold or interval not met)";
        return nullptr;  // Skip rebalancing
    }
    
    FL_LOG(INFO) << "Rebalancing triggered";
    
    // Compute new split points
    auto new_splits = m_balancer->computeSplitPoints(
        cell_counts, m_current_splits,
        0, UINT64_MAX  // Global Hilbert range
    );
    
    // Create migration plan
    auto plan = m_balancer->createMigrationPlan(
        new_splits, m_current_splits,
        m_hilbert_min, m_hilbert_max,
        *m_num_cells
    );
    
    // Execute migration
    if (plan.isValid() && !plan.transfers.empty()) {
        FL_LOG(INFO) << "Executing migration plan:\n" << plan.toString();
        
        m_migrator->migrate(
            plan,
            m_coord_x, m_coord_y, m_coord_z,
            m_levels, m_cell_states,
            m_fields, m_num_field_components,
            m_num_cells, m_capacity
        );
        
        // Update current splits
        m_current_splits = new_splits;
        
        // Reset timestep counter
        m_balancer->resetTimestep();
        
        FL_LOG(INFO) << "Rebalancing complete: new cell count = " << *m_num_cells;
    } else {
        FL_LOG(INFO) << "No migration needed (empty plan)";
    }
    
    // TODO: Rebuild hash table and ghost ranges (Phase 4)
    // This would call:
    // - hash_manager->rebuildPartial(...)
    // - ghost_builder->buildGhostRanges(...)
    // - MPI_Alltoall for ghost range exchange
    
    return nullptr;  // No event for now
}

} // namespace nodes
} // namespace runtime
} // namespace fluidloom
