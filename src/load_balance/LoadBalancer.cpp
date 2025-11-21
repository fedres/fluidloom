#include "fluidloom/load_balance/LoadBalancer.h"
#include "fluidloom/common/Logger.h"
#include <algorithm>
#include <numeric>

#ifdef FLUIDLOOM_MPI_ENABLED
#include <mpi.h>
#endif

namespace fluidloom {
namespace load_balance {

LoadBalancer::LoadBalancer(transport::MPITransport* transport, const LoadBalanceConfig& config)
    : m_transport(transport), m_config(config) {
    
    if (!transport) {
        throw std::invalid_argument("MPITransport cannot be null");
    }
    
    m_config.validate();
    
    FL_LOG(INFO) << "LoadBalancer initialized for " << m_transport->getSize() << " GPUs";
    FL_LOG(INFO) << "  Imbalance tolerance: " << m_config.imbalance_tolerance;
    FL_LOG(INFO) << "  Min interval: " << m_config.min_interval_timesteps << " steps";
}

std::vector<size_t> LoadBalancer::gatherCellCounts(size_t local_cell_count) {
#ifdef FLUIDLOOM_MPI_ENABLED
    int num_gpus = m_transport->getSize();
    std::vector<size_t> all_counts(num_gpus);
    
    // Use MPI_Allgather to collect cell counts from all GPUs
    MPI_Allgather(&local_cell_count, 1, MPI_UNSIGNED_LONG,
                  all_counts.data(), 1, MPI_UNSIGNED_LONG,
                  MPI_COMM_WORLD);
    
    m_cached_cell_counts = all_counts;
    
    FL_LOG(INFO) << "Gathered cell counts from " << num_gpus << " GPUs:";
    for (int i = 0; i < num_gpus; ++i) {
        FL_LOG(INFO) << "  GPU " << i << ": " << all_counts[i] << " cells";
    }
    
    return all_counts;
#else
    // Single GPU fallback
    m_cached_cell_counts = {local_cell_count};
    return {local_cell_count};
#endif
}

float LoadBalancer::calculateCurrentImbalance() {
    if (m_cached_cell_counts.empty()) {
        FL_LOG(WARN) << "No cached cell counts, returning 0 imbalance";
        return 0.0f;
    }
    
    return LoadBalanceConfig::calculateImbalance(m_cached_cell_counts);
}

bool LoadBalancer::shouldRebalance(const std::vector<size_t>& cell_counts) {
    bool should = m_config.shouldRebalance(cell_counts, m_steps_since_last_balance);
    
    if (should) {
        float imbalance = LoadBalanceConfig::calculateImbalance(cell_counts);
        FL_LOG(INFO) << "Rebalancing triggered: imbalance=" << imbalance 
                     << ", steps_since_last=" << m_steps_since_last_balance;
    }
    
    return should;
}

std::vector<uint64_t> LoadBalancer::computeSplitPoints(
    const std::vector<size_t>& cell_counts,
    const std::vector<uint64_t>& current_splits,
    uint64_t global_hilbert_min,
    uint64_t global_hilbert_max
) {
    int num_gpus = static_cast<int>(cell_counts.size());
    if (num_gpus <= 1) {
        return {};  // No splits needed for single GPU
    }
    
    // Calculate total cells and target per GPU
    size_t total_cells = std::accumulate(cell_counts.begin(), cell_counts.end(), size_t(0));
    size_t target_per_gpu = total_cells / num_gpus;
    
    FL_LOG(INFO) << "Computing split points: total_cells=" << total_cells 
                 << ", target_per_gpu=" << target_per_gpu;
    
    std::vector<uint64_t> new_splits;
    new_splits.reserve(num_gpus - 1);
    
    size_t cumulative_cells = 0;
    uint64_t hilbert_range = global_hilbert_max - global_hilbert_min;
    
    for (int gpu = 0; gpu < num_gpus - 1; ++gpu) {
        cumulative_cells += target_per_gpu;
        
        // Estimate Hilbert index at cumulative position
        // Simple linear interpolation: hilbert = min + (cumulative / total) * range
        double fraction = static_cast<double>(cumulative_cells) / total_cells;
        uint64_t estimated_split = global_hilbert_min + 
                                   static_cast<uint64_t>(fraction * hilbert_range);
        
        new_splits.push_back(estimated_split);
        
        FL_LOG(INFO) << "  Split " << gpu << ": Hilbert=" << estimated_split 
                     << " (cumulative=" << cumulative_cells << " cells)";
    }
    
    return new_splits;
}

MigrationPlan LoadBalancer::createMigrationPlan(
    const std::vector<uint64_t>& new_splits,
    const std::vector<uint64_t>& current_splits,
    uint64_t local_hilbert_min,
    uint64_t local_hilbert_max,
    size_t local_cell_count
) {
    MigrationPlan plan;
    int my_rank = m_transport->getRank();
    int num_gpus = m_transport->getSize();
    
    FL_LOG(INFO) << "Creating migration plan for GPU " << my_rank;
    FL_LOG(INFO) << "  Local Hilbert range: [" << local_hilbert_min << ", " << local_hilbert_max << ")";
    FL_LOG(INFO) << "  Local cell count: " << local_cell_count;
    
    // Determine which GPU owns each Hilbert range under new splits
    auto getOwnerGPU = [&](uint64_t hilbert_index, const std::vector<uint64_t>& splits) -> int {
        for (size_t i = 0; i < splits.size(); ++i) {
            if (hilbert_index < splits[i]) return static_cast<int>(i);
        }
        return num_gpus - 1;  // Last GPU owns everything above last split
    };
    
    // Check if any of our cells need to migrate
    int old_owner = getOwnerGPU(local_hilbert_min, current_splits);
    int new_owner = getOwnerGPU(local_hilbert_min, new_splits);
    
    if (old_owner != new_owner) {
        // Our entire range migrates to a different GPU
        MigrationPlan::Transfer transfer(
            my_rank, new_owner,
            local_hilbert_min, local_hilbert_max,
            local_cell_count
        );
        
        plan.transfers.push_back(transfer);
        plan.total_cells_to_migrate += local_cell_count;
        
        FL_LOG(INFO) << "  Entire range migrates: GPU " << my_rank << " -> GPU " << new_owner;
    } else {
        // Check if range spans multiple new owners (split case)
        for (size_t i = 0; i < new_splits.size(); ++i) {
            uint64_t split_point = new_splits[i];
            if (split_point > local_hilbert_min && split_point < local_hilbert_max) {
                // Range is split at this point
                int dest_gpu = static_cast<int>(i) + 1;
                
                // Estimate number of cells above split point
                double fraction = static_cast<double>(split_point - local_hilbert_min) / 
                                (local_hilbert_max - local_hilbert_min);
                size_t cells_to_migrate = static_cast<size_t>((1.0 - fraction) * local_cell_count);
                
                MigrationPlan::Transfer transfer(
                    my_rank, dest_gpu,
                    split_point, local_hilbert_max,
                    cells_to_migrate
                );
                
                plan.transfers.push_back(transfer);
                plan.total_cells_to_migrate += cells_to_migrate;
                
                FL_LOG(INFO) << "  Partial migration: " << cells_to_migrate << " cells -> GPU " << dest_gpu;
            }
        }
    }
    
    // Optimize plan
    plan.optimize();
    
    // Estimate migration time (simple model: 1 μs per cell)
    plan.estimated_time_ms = plan.total_cells_to_migrate * 0.001f;
    
    FL_LOG(INFO) << "Migration plan created: " << plan.transfers.size() << " transfers, "
                 << plan.total_cells_to_migrate << " cells, "
                 << plan.estimated_time_ms << " ms estimated";
    
    return plan;
}

uint64_t LoadBalancer::estimateSplitPoint(
    size_t cumulative_cells,
    const std::vector<size_t>& cell_counts,
    const std::vector<uint64_t>& current_splits
) const {
    // Simple linear interpolation based on cumulative cell count
    // This is a placeholder - real implementation would sample Hilbert indices
    
    size_t total_cells = std::accumulate(cell_counts.begin(), cell_counts.end(), size_t(0));
    if (total_cells == 0) return 0;
    
    double fraction = static_cast<double>(cumulative_cells) / total_cells;
    
    // Assume Hilbert range is [0, UINT64_MAX)
    return static_cast<uint64_t>(fraction * UINT64_MAX);
}

} // namespace load_balance
} // namespace fluidloom
