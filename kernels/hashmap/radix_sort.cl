// OpenCL Radix Sort Kernels
// Implements LSB radix sort for 64-bit keys with 32-bit values
// Processes 8 bits per pass (8 passes total for 64-bit keys)

#define NUM_BINS 256
#define BITS_PER_PASS 8

/**
 * @brief Histogram kernel: Count occurrences of each radix digit
 * 
 * Each work-group processes a chunk of data and produces a local histogram.
 * Results are written to global histogram array: histograms[group_id * NUM_BINS + bin]
 */
__kernel void radix_histogram(
    __global const ulong* keys,
    __global uint* histograms,
    uint pass,              // Which byte to process (0-7)
    uint num_elements
) {
    uint gid = get_global_id(0);
    uint lid = get_local_id(0);
    uint group_id = get_group_id(0);
    uint local_size = get_local_size(0);
    
    // Local histogram (one per work-group)
    __local uint local_hist[NUM_BINS];
    
    // Initialize local histogram
    if (lid < NUM_BINS) {
        local_hist[lid] = 0;
    }
    barrier(CLK_LOCAL_MEM_FENCE);
    
    // Extract radix digit and increment histogram
    if (gid < num_elements) {
        ulong key = keys[gid];
        uint digit = (key >> (pass * BITS_PER_PASS)) & 0xFF;
        atomic_inc(&local_hist[digit]);
    }
    barrier(CLK_LOCAL_MEM_FENCE);
    
    // Write local histogram to global memory
    if (lid < NUM_BINS) {
        histograms[group_id * NUM_BINS + lid] = local_hist[lid];
    }
}

/**
 * @brief Prefix sum kernel: Compute cumulative offsets from histograms
 * 
 * Combines histograms from all work-groups and computes exclusive prefix sum.
 * This gives us the starting position for each bin in the output array.
 */
__kernel void prefix_sum(
    __global uint* histograms,
    uint num_groups
) {
    uint lid = get_local_id(0);
    
    // Each work-item processes one bin across all groups
    if (lid >= NUM_BINS) return;
    
    // Sum across all groups for this bin
    uint total = 0;
    for (uint g = 0; g < num_groups; ++g) {
        uint count = histograms[g * NUM_BINS + lid];
        histograms[g * NUM_BINS + lid] = total;  // Store exclusive prefix sum
        total += count;
    }
}

/**
 * @brief Scatter kernel: Reorder keys and values based on radix digit
 * 
 * Uses the prefix sum offsets to place each element in its correct position.
 */
__kernel void radix_scatter(
    __global const ulong* keys_in,
    __global const uint* values_in,
    __global ulong* keys_out,
    __global uint* values_out,
    __global uint* offsets,      // Prefix sum results
    uint pass,
    uint num_elements
) {
    uint gid = get_global_id(0);
    uint group_id = get_group_id(0);
    
    if (gid >= num_elements) return;
    
    // Read key and extract digit
    ulong key = keys_in[gid];
    uint value = values_in[gid];
    uint digit = (key >> (pass * BITS_PER_PASS)) & 0xFF;
    
    // Get starting offset for this bin in this group
    uint offset = offsets[group_id * NUM_BINS + digit];
    
    // Atomically claim a position in the output
    uint pos = atomic_inc(&offsets[group_id * NUM_BINS + digit]);
    
    // Write to output
    keys_out[pos] = key;
    values_out[pos] = value;
}

/**
 * @brief Simple copy kernel for final pass if needed
 */
__kernel void copy_buffer(
    __global const ulong* src_keys,
    __global const uint* src_values,
    __global ulong* dst_keys,
    __global uint* dst_values,
    uint num_elements
) {
    uint gid = get_global_id(0);
    if (gid >= num_elements) return;
    
    dst_keys[gid] = src_keys[gid];
    dst_values[gid] = src_values[gid];
}
