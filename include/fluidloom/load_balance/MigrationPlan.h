#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <sstream>

namespace fluidloom {
namespace load_balance {

/**
 * @brief Describes a complete cell migration plan across GPUs
 * 
 * A migration plan consists of multiple transfers, each moving a contiguous
 * Hilbert range from one GPU to another. The plan is optimized to minimize
 * network traffic and preserve spatial locality.
 */
struct MigrationPlan {
    /**
     * @brief Single cell transfer between two GPUs
     */
    struct Transfer {
        int source_rank;          // MPI rank of source GPU
        int dest_rank;            // MPI rank of destination GPU
        uint64_t hilbert_start;   // Start of Hilbert range (inclusive)
        uint64_t hilbert_end;     // End of Hilbert range (exclusive)
        size_t num_cells;         // Number of cells to transfer
        
        Transfer() : source_rank(-1), dest_rank(-1), 
                    hilbert_start(0), hilbert_end(0), num_cells(0) {}
        
        Transfer(int src, int dst, uint64_t h_start, uint64_t h_end, size_t n)
            : source_rank(src), dest_rank(dst), 
              hilbert_start(h_start), hilbert_end(h_end), num_cells(n) {}
        
        bool isValid() const {
            return source_rank >= 0 && dest_rank >= 0 && 
                   source_rank != dest_rank &&
                   hilbert_end > hilbert_start && num_cells > 0;
        }
    };
    
    std::vector<Transfer> transfers;
    size_t total_cells_to_migrate = 0;
    float estimated_time_ms = 0.0f;
    
    /**
     * @brief Validate the entire migration plan
     * @return true if all transfers are valid and plan is consistent
     */
    bool isValid() const {
        if (transfers.empty()) return true;  // Empty plan is valid (no migration needed)
        
        size_t computed_total = 0;
        for (const auto& transfer : transfers) {
            if (!transfer.isValid()) return false;
            computed_total += transfer.num_cells;
        }
        
        return computed_total == total_cells_to_migrate;
    }
    
    /**
     * @brief Optimize the migration plan
     * 
     * Performs the following optimizations:
     * - Merge adjacent transfers with same source/dest
     * - Remove zero-cell transfers
     * - Sort by source rank for better network utilization
     */
    void optimize() {
        // Remove invalid transfers
        transfers.erase(
            std::remove_if(transfers.begin(), transfers.end(),
                          [](const Transfer& t) { return !t.isValid() || t.num_cells == 0; }),
            transfers.end()
        );
        
        // Sort by source rank, then dest rank
        std::sort(transfers.begin(), transfers.end(),
                 [](const Transfer& a, const Transfer& b) {
                     if (a.source_rank != b.source_rank) 
                         return a.source_rank < b.source_rank;
                     return a.dest_rank < b.dest_rank;
                 });
        
        // Merge adjacent transfers with same source/dest
        std::vector<Transfer> merged;
        for (const auto& transfer : transfers) {
            if (!merged.empty() && 
                merged.back().source_rank == transfer.source_rank &&
                merged.back().dest_rank == transfer.dest_rank &&
                merged.back().hilbert_end == transfer.hilbert_start) {
                // Merge with previous transfer
                merged.back().hilbert_end = transfer.hilbert_end;
                merged.back().num_cells += transfer.num_cells;
            } else {
                merged.push_back(transfer);
            }
        }
        
        transfers = std::move(merged);
        
        // Recompute total
        total_cells_to_migrate = 0;
        for (const auto& transfer : transfers) {
            total_cells_to_migrate += transfer.num_cells;
        }
    }
    
    /**
     * @brief Get a human-readable summary of the migration plan
     */
    std::string toString() const {
        std::ostringstream oss;
        oss << "MigrationPlan[" << transfers.size() << " transfers, "
            << total_cells_to_migrate << " cells, "
            << estimated_time_ms << " ms]\n";
        
        for (size_t i = 0; i < transfers.size(); ++i) {
            const auto& t = transfers[i];
            oss << "  Transfer " << i << ": GPU " << t.source_rank 
                << " -> GPU " << t.dest_rank << ", "
                << t.num_cells << " cells, "
                << "Hilbert [" << t.hilbert_start << ", " << t.hilbert_end << ")\n";
        }
        
        return oss.str();
    }
};

} // namespace load_balance
} // namespace fluidloom
