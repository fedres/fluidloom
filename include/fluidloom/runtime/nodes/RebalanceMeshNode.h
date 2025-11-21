#pragma once

#include "fluidloom/runtime/ExecutionNode.h"
#include "fluidloom/load_balance/LoadBalancer.h"
#include "fluidloom/load_balance/CellMigrator.h"

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include <string>
#include <memory>

namespace fluidloom {
namespace runtime {
namespace nodes {

/**
 * @brief Execution node for dynamic load rebalancing
 * 
 * Integrates LoadBalancer and CellMigrator to redistribute cells across GPUs
 * when load imbalance exceeds configured threshold.
 */
class RebalanceMeshNode : public ExecutionNode {
public:
    /**
     * @brief Create rebalance mesh node
     * @param name Node name
     * @param balancer Load balancer instance
     * @param migrator Cell migrator instance
     */
    RebalanceMeshNode(
        std::string name,
        load_balance::LoadBalancer* balancer,
        load_balance::CellMigrator* migrator
    );
    
    ~RebalanceMeshNode() override = default;
    
    /**
     * @brief Bind mesh buffers
     */
    void bindMesh(
        cl_mem* coord_x, cl_mem* coord_y, cl_mem* coord_z,
        cl_mem* levels, cl_mem* cell_states,
        cl_mem* fields, uint32_t num_field_components,
        size_t* num_cells, size_t* capacity
    );
    
    /**
     * @brief Set current Hilbert range owned by this GPU
     */
    void setHilbertRange(uint64_t min, uint64_t max) {
        m_hilbert_min = min;
        m_hilbert_max = max;
    }
    
    /**
     * @brief Execute rebalancing if needed
     * @param wait_event Event to wait on before execution
     * @return Event signaling completion
     */
    cl_event execute(cl_event wait_event) override;
    
private:
    load_balance::LoadBalancer* m_balancer;
    load_balance::CellMigrator* m_migrator;
    
    // Mesh buffer pointers
    cl_mem* m_coord_x = nullptr;
    cl_mem* m_coord_y = nullptr;
    cl_mem* m_coord_z = nullptr;
    cl_mem* m_levels = nullptr;
    cl_mem* m_cell_states = nullptr;
    cl_mem* m_fields = nullptr;
    uint32_t m_num_field_components = 0;
    size_t* m_num_cells = nullptr;
    size_t* m_capacity = nullptr;
    
    // Hilbert range
    uint64_t m_hilbert_min = 0;
    uint64_t m_hilbert_max = UINT64_MAX;
    
    // Current split points (shared across all GPUs)
    std::vector<uint64_t> m_current_splits;
};

} // namespace nodes
} // namespace runtime
} // namespace fluidloom
