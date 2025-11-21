#pragma once

#include "fluidloom/load_balance/LoadBalanceConfig.h"
#include "fluidloom/load_balance/MigrationPlan.h"
#include "fluidloom/transport/MPITransport.h"
#include <vector>
#include <memory>
#include <cstdint>

namespace fluidloom {
namespace load_balance {

/**
 * @brief Hilbert SFC Load Balancer for multi-GPU simulations
 * 
 * Redistributes cells across GPUs to equalize load using Hilbert space-filling
 * curve partitioning. Computes optimal split points and creates migration plans.
 */
class LoadBalancer {
public:
    /**
     * @brief Initialize load balancer
     * @param transport MPI transport layer for communication
     * @param config Load balancing configuration
     */
    LoadBalancer(transport::MPITransport* transport, const LoadBalanceConfig& config);
    
    ~LoadBalancer() = default;
    
    // Non-copyable
    LoadBalancer(const LoadBalancer&) = delete;
    LoadBalancer& operator=(const LoadBalancer&) = delete;
    
    /**
     * @brief Gather cell counts from all GPUs
     * @param local_cell_count Number of cells on this GPU
     * @return Vector of cell counts, one per GPU
     */
    std::vector<size_t> gatherCellCounts(size_t local_cell_count);
    
    /**
     * @brief Calculate current load imbalance
     * @return Imbalance metric: (max - min) / avg
     */
    float calculateCurrentImbalance();
    
    /**
     * @brief Check if rebalancing should occur
     * @param cell_counts Current cell counts per GPU
     * @return true if rebalancing is needed
     */
    bool shouldRebalance(const std::vector<size_t>& cell_counts);
    
    /**
     * @brief Compute new Hilbert split points to equalize load
     * @param cell_counts Current cell counts per GPU
     * @param current_splits Current Hilbert split points (N-1 for N GPUs)
     * @param global_hilbert_min Minimum Hilbert index in simulation
     * @param global_hilbert_max Maximum Hilbert index in simulation
     * @return New split points that equalize load
     */
    std::vector<uint64_t> computeSplitPoints(
        const std::vector<size_t>& cell_counts,
        const std::vector<uint64_t>& current_splits,
        uint64_t global_hilbert_min,
        uint64_t global_hilbert_max
    );
    
    /**
     * @brief Create migration plan from old to new split points
     * @param new_splits New Hilbert split points
     * @param current_splits Current Hilbert split points
     * @param local_hilbert_min Minimum Hilbert index on this GPU
     * @param local_hilbert_max Maximum Hilbert index on this GPU
     * @param local_cell_count Number of cells on this GPU
     * @return Migration plan describing all transfers
     */
    MigrationPlan createMigrationPlan(
        const std::vector<uint64_t>& new_splits,
        const std::vector<uint64_t>& current_splits,
        uint64_t local_hilbert_min,
        uint64_t local_hilbert_max,
        size_t local_cell_count
    );
    
    /**
     * @brief Increment timestep counter
     */
    void incrementTimestep() { m_steps_since_last_balance++; }
    
    /**
     * @brief Reset timestep counter (called after rebalancing)
     */
    void resetTimestep() { m_steps_since_last_balance = 0; }
    
    /**
     * @brief Get number of timesteps since last rebalance
     */
    uint32_t getStepsSinceLastBalance() const { return m_steps_since_last_balance; }
    
private:
    transport::MPITransport* m_transport;
    LoadBalanceConfig m_config;
    uint32_t m_steps_since_last_balance = 0;
    std::vector<size_t> m_cached_cell_counts;
    
    /**
     * @brief Estimate Hilbert index at given cumulative cell position
     * @param cumulative_cells Target cumulative cell count
     * @param cell_counts Cell counts per GPU
     * @param current_splits Current split points
     * @return Estimated Hilbert index
     */
    uint64_t estimateSplitPoint(
        size_t cumulative_cells,
        const std::vector<size_t>& cell_counts,
        const std::vector<uint64_t>& current_splits
    ) const;
};

} // namespace load_balance
} // namespace fluidloom
