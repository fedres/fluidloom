#pragma once

#include <cstdint>
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <numeric>

namespace fluidloom {
namespace load_balance {

/**
 * @brief Configuration for Hilbert SFC load balancer
 * 
 * All parameters control when and how load balancing occurs.
 * These settings are validated at startup and remain immutable during execution.
 */
struct LoadBalanceConfig {
    // Enable/disable load balancing entirely
    bool enabled = true;
    
    // Trigger conditions
    uint32_t min_interval_timesteps = 10;  // Minimum steps between rebalances
    float imbalance_tolerance = 0.15f;     // Trigger if (max-min)/avg > 15%
    uint32_t max_cells_per_migration_block = 50000;  // Max cells in one MPI transfer
    
    // Split point calculation
    uint32_t num_sample_points = 1000;     // Global samples for split estimation
    bool use_exact_count = false;          // true: O(N log N), false: O(S log N)
    
    // Migration policy
    bool migrate_in_hilbert_order = true;  // Preserve spatial locality
    bool allow_cell_split_during_migration = false;  // true: split large blocks
    
    // Performance guardrails
    float max_migration_time_ms = 100.0f;  // Fail-fast if too slow
    float max_memory_overhead_percent = 5.0f;  // Extra memory allowed
    
    // Debug/validation
    bool validate_migration = true;        // Check cell counts before/after
    bool dump_migration_plan = false;      // Write migration schedule to disk
    
    /**
     * @brief Validate configuration parameters
     * @throws std::invalid_argument if any parameter is out of valid range
     */
    void validate() const {
        if (min_interval_timesteps < 10) {
            throw std::invalid_argument("min_interval_timesteps must be >= 10");
        }
        if (imbalance_tolerance < 0.05f || imbalance_tolerance > 0.5f) {
            throw std::invalid_argument("imbalance_tolerance must be in [0.05, 0.5]");
        }
        if (max_cells_per_migration_block < 1000) {
            throw std::invalid_argument("Migration blocks must be >= 1000 cells");
        }
        if (num_sample_points < 100) {
            throw std::invalid_argument("Need at least 100 sample points");
        }
    }
    
    /**
     * @brief Calculate current imbalance from per-GPU cell counts
     * @param cell_counts Vector of cell counts, one per GPU
     * @return Imbalance metric: (max - min) / avg
     */
    static float calculateImbalance(const std::vector<size_t>& cell_counts) {
        if (cell_counts.empty()) return 0.0f;
        
        size_t total = std::accumulate(cell_counts.begin(), cell_counts.end(), size_t(0));
        size_t min_cells = *std::min_element(cell_counts.begin(), cell_counts.end());
        size_t max_cells = *std::max_element(cell_counts.begin(), cell_counts.end());
        
        float avg_cells = static_cast<float>(total) / cell_counts.size();
        if (avg_cells == 0.0f) return 0.0f;
        
        return static_cast<float>(max_cells - min_cells) / avg_cells;
    }
    
    /**
     * @brief Determine if rebalancing should occur
     * @param cell_counts Current cell counts per GPU
     * @param steps_since_last Number of timesteps since last rebalance
     * @return true if rebalancing should be triggered
     */
    bool shouldRebalance(const std::vector<size_t>& cell_counts, 
                        uint32_t steps_since_last) const {
        if (!enabled) return false;
        if (steps_since_last < min_interval_timesteps) return false;
        
        float imbalance = calculateImbalance(cell_counts);
        return imbalance > imbalance_tolerance;
    }
};

} // namespace load_balance
} // namespace fluidloom
