#pragma once

#include <cstdint>
#include <cstddef>

namespace fluidloom {
namespace hashmap {

// Empty marker: MUST match HILBERT_EMPTY from Module 2
static constexpr uint64_t HASH_EMPTY_KEY = 0xFFFFFFFFFFFFFFFFULL;

// Invalid index marker for queries
static constexpr uint32_t HASH_INVALID_VALUE = 0xFFFFFFFFU;

// Maximum load factor: rebuild when size > capacity * LOAD_FACTOR
static constexpr float MAX_LOAD_FACTOR = 0.6f;

// Initial capacity power-of-two for new tables
static constexpr uint32_t INITIAL_CAPACITY_POW2 = 20;  // 2^20 = 1,048,576 slots

// Alignment for cache line efficiency
static constexpr size_t HASH_TABLE_ALIGNMENT = 64;

/**
 * @brief GPU-resident hash table (read-only after build)
 * 
 * This struct is passed to kernels for device-side queries.
 * Open addressing with linear probing.
 */
struct alignas(HASH_TABLE_ALIGNMENT) HashTable {
    uint64_t capacity;          // Power of two, >= num_cells * (1/LOAD_FACTOR)
    uint64_t size;              // Current number of valid entries
    uint32_t log2_capacity;     // Precomputed log2(capacity) for fast mask
    
    // Device buffer pointers (managed by backend)
    void* keys_device_ptr;      // uint64_t* - Hilbert indices
    void* values_device_ptr;    // uint32_t* - SOA array indices
    
    // Fast hash function: Fibonacci hashing
    static inline uint64_t hashKey(uint64_t key) {
        // Golden ratio multiplicative hash for good distribution
        return key * 0x9e3779b97f4a7c15ULL;
    }
    
    // Get slot index from key (branchless)
    inline uint64_t getSlot(uint64_t key) const {
        return hashKey(key) & (capacity - 1);
    }
};

/**
 * @brief CPU-side metadata for managing hash table lifecycle
 */
struct HashTableMetadata {
    size_t num_cells;           // Number of active cells
    size_t active_capacity;     // capacity * MAX_LOAD_FACTOR (rebuild threshold)
    size_t bytes_allocated;     // Total device memory used
    uint32_t build_time_ms;     // Last rebuild duration
    float average_probe_count;  // From last validation pass
    uint32_t max_probe_count;   // Worst-case probe count
};

// Probe strategy configuration
struct HashBuildConfig {
    static constexpr uint32_t MAX_PROBE_LIMIT = 32;  // Safety limit
    static constexpr bool ENABLE_STATS = true;       // Track probe counts
};

} // namespace hashmap
} // namespace fluidloom
