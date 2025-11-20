#pragma once

#include "fluidloom/core/hashmap/HashTable.h"
#include "fluidloom/core/hashmap/RadixSort.h"
#include "fluidloom/core/backend/IBackend.h"
#include <memory>
#include <vector>

namespace fluidloom {
namespace hashmap {

/**
 * @brief Result of compaction operation
 */
struct CompactionResult {
    size_t num_valid_cells;                  // Number of active cells after compaction
    std::vector<uint32_t> permutation;       // Maps old index → new index
    double compaction_time_ms;               // Time taken
};

/**
 * @brief Compacts cell lists and generates permutation vectors
 * 
 * Removes empty/invalid entries from cell lists and produces dense arrays.
 * Generates permutation vectors for reordering SOA fields.
 */
class CompactionEngine {
public:
    explicit CompactionEngine(IBackend* backend);
    ~CompactionEngine();
    
    // Delete copy/move
    CompactionEngine(const CompactionEngine&) = delete;
    CompactionEngine& operator=(const CompactionEngine&) = delete;
    
    /**
     * @brief Compact active cells and generate permutation
     * 
     * @param hilbert_indices Hilbert indices (may contain HASH_EMPTY_KEY)
     * @param cell_state 0=INACTIVE, 1=ACTIVE
     * @return Compaction result with dense list and permutation
     */
    CompactionResult compactAndPermute(
        const std::vector<uint64_t>& hilbert_indices,
        const std::vector<uint32_t>& cell_state
    );
    
    /**
     * @brief Apply permutation to SOA fields
     * 
     * Reorders field data according to permutation vector.
     * Launches scatter kernel on GPU.
     */
    void applyPermutationToFields(
        const std::vector<void*>& field_device_ptrs,
        size_t element_size,
        size_t old_count,
        const std::vector<uint32_t>& permutation
    );
    
    // Get peak memory usage
    size_t getPeakMemoryUsage() const { return peak_memory_usage_; }
    
private:
    IBackend* backend_;
    std::unique_ptr<RadixSort> sorter_;
    
    // Scratch buffers
    DeviceBufferPtr scratch_keys_;
    DeviceBufferPtr scratch_values_;
    
    size_t peak_memory_usage_;
    
    // Ensure scratch buffers are allocated
    void ensureScratchAllocated(size_t count);
};

} // namespace hashmap
} // namespace fluidloom
