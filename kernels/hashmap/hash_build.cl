// Hash Table Build and Query Kernels
// Open addressing with linear probing

#define HASH_EMPTY_KEY 0xFFFFFFFFFFFFFFFFULL
#define HASH_INVALID_VALUE 0xFFFFFFFFU
#define MAX_PROBE_LIMIT 32

// Fibonacci hashing for 64-bit keys
inline ulong hash_key(ulong key) {
    return key * 0x9e3779b97f4a7c15ULL;
}

/**
 * @brief Build kernel: Insert (key, value) pairs into hash table
 * 
 * Uses atomic compare-and-swap for thread-safe concurrent insertion.
 * Linear probing for collision resolution.
 */
__kernel void hash_build(
    __global ulong* table_keys,
    __global uint* table_values,
    __global const ulong* input_keys,
    __global const uint* input_values,
    ulong capacity,
    uint num_cells,
    __global uint* probe_counts  // Optional stats
) {
    uint gid = get_global_id(0);
    if (gid >= num_cells) return;
    
    ulong key = input_keys[gid];
    uint value = input_values[gid];
    
    // Skip empty keys
    if (key == HASH_EMPTY_KEY) return;
    
    // Initial slot
    ulong slot = hash_key(key) & (capacity - 1);
    uint probe_count = 0;
    
    // Linear probing with atomic CAS
    while (probe_count < MAX_PROBE_LIMIT) {
        // Attempt to claim slot
        ulong prev = atom_cmpxchg(&table_keys[slot], HASH_EMPTY_KEY, key);
        
        if (prev == HASH_EMPTY_KEY) {
            // Successfully inserted
            table_values[slot] = value;
            if (probe_counts) probe_counts[gid] = probe_count;
            return;
        }
        
        if (prev == key) {
            // Key already exists - overwrite value (idempotent)
            table_values[slot] = value;
            if (probe_counts) probe_counts[gid] = probe_count;
            return;
        }
        
        // Collision: try next slot
        slot = (slot + 1) & (capacity - 1);
        probe_count++;
    }
    
    // Probe limit exceeded - should not happen with 0.6 load factor
    if (probe_counts) probe_counts[gid] = MAX_PROBE_LIMIT;
}

/**
 * @brief Clear kernel: Initialize empty hash table
 */
__kernel void hash_clear(
    __global ulong* table_keys,
    __global uint* table_values,
    ulong capacity
) {
    uint gid = get_global_id(0);
    if (gid >= capacity) return;
    
    table_keys[gid] = HASH_EMPTY_KEY;
    table_values[gid] = HASH_INVALID_VALUE;
}
