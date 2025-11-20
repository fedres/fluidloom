// Hash Table Query Functions
// Device-side lookups for neighbor finding

#define HASH_EMPTY_KEY 0xFFFFFFFFFFFFFFFFULL
#define HASH_INVALID_VALUE 0xFFFFFFFFU
#define MAX_PROBE_LIMIT 32

// Fibonacci hashing
inline ulong hash_key(ulong key) {
    return key * 0x9e3779b97f4a7c15ULL;
}

/**
 * @brief Device-side query function (inlined for performance)
 * 
 * @param table_keys Device array of Hilbert indices
 * @param table_values Device array of SOA indices
 * @param capacity Hash table capacity (power of 2)
 * @param query_key Hilbert index to look up
 * @return SOA array index or HASH_INVALID_VALUE if not found
 */
inline uint hash_query(
    __global const ulong* table_keys,
    __global const uint* table_values,
    ulong capacity,
    ulong query_key
) {
    // Fast path for invalid keys
    if (query_key == HASH_EMPTY_KEY) {
        return HASH_INVALID_VALUE;
    }
    
    ulong slot = hash_key(query_key) & (capacity - 1);
    uint probe_count = 0;
    
    while (probe_count < MAX_PROBE_LIMIT) {
        ulong current_key = table_keys[slot];
        
        if (current_key == query_key) {
            // Found it
            return table_values[slot];
        }
        
        if (current_key == HASH_EMPTY_KEY) {
            // Key not in table
            return HASH_INVALID_VALUE;
        }
        
        // Continue probing
        slot = (slot + 1) & (capacity - 1);
        probe_count++;
    }
    
    // Probe limit exceeded
    return HASH_INVALID_VALUE;
}

/**
 * @brief Validation kernel: Test queries and collect statistics
 */
__kernel void hash_validate(
    __global const ulong* table_keys,
    __global const uint* table_values,
    ulong capacity,
    __global const ulong* test_keys,
    __global uint* results,
    __global uint* probe_counts,
    uint num_queries
) {
    uint gid = get_global_id(0);
    if (gid >= num_queries) return;
    
    ulong query_key = test_keys[gid];
    ulong slot = hash_key(query_key) & (capacity - 1);
    uint local_probes = 0;
    
    while (local_probes < MAX_PROBE_LIMIT) {
        ulong current_key = table_keys[slot];
        
        if (current_key == query_key) {
            results[gid] = table_values[slot];
            probe_counts[gid] = local_probes;
            return;
        }
        
        if (current_key == HASH_EMPTY_KEY) {
            results[gid] = HASH_INVALID_VALUE;
            probe_counts[gid] = local_probes;
            return;
        }
        
        slot = (slot + 1) & (capacity - 1);
        local_probes++;
    }
    
    results[gid] = HASH_INVALID_VALUE;
    probe_counts[gid] = MAX_PROBE_LIMIT;
}
