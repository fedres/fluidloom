// GPU Mesh Compaction Kernels
// Implements parallel prefix sum (scan) and stream compaction

#define WORKGROUP_SIZE 256

// 1. Predicate Kernel: Mark cells that should be kept
// Input: split_refine_flags, merge_refine_flags, cell_states
// Output: valid_flags (1 if kept, 0 if removed)
__kernel void mark_valid_cells(
    __global const int* restrict refine_flags,
    __global const uchar* restrict cell_states,
    __global const uint* restrict merge_group_id, // If part of a merge group, only parent keeps it? No.
    // Actually, compaction happens AFTER split/merge logic has generated NEW lists?
    // No, the plan is:
    // 1. Split -> generates new children (appended or separate buffer)
    // 2. Merge -> generates new parents (appended or separate buffer)
    // 3. Compact -> takes OLD cells + NEW children + NEW parents, removes DEAD cells.
    // Dead cells are:
    // - Parents that split (refine_flags > 0)
    // - Children that merged (merge_group_id != INVALID)
    // - Cells explicitly marked for removal (if any)
    
    // Wait, AdaptationEngine currently:
    // 1. Collects all UNTOUCHED old cells.
    // 2. Appends NEW children from splits.
    // 3. Appends NEW parents from merges.
    // 4. Sorts and rebuilds.
    
    // So we need a kernel to mark "survivors" from the old cell list.
    // Survivors = (refine_flags <= 0) AND (merge_group_id == INVALID_INDEX)
    
    __global uint* restrict valid_flags,
    const uint num_cells) {
    
    const uint idx = get_global_id(0);
    if (idx >= num_cells) return;
    
    int flag = refine_flags[idx];
    uint grp = merge_group_id[idx];
    
    // Keep if NOT splitting AND NOT merging
    // Note: refine_flags=1 means split. -1 means merge (but merge_group_id handles the actual merge success).
    // If refine_flags=-1 but merge_group_id is INVALID, it failed to merge, so we KEEP it.
    
    bool is_splitting = (flag > 0);
    bool is_merging = (grp != 0xFFFFFFFF); // INVALID_INDEX
    
    valid_flags[idx] = (is_splitting || is_merging) ? 0 : 1;
}

// 2. Hillis-Steele Scan (Inclusive) - Single Workgroup (for small arrays) or Multi-pass
// For simplicity, we'll use a simple Blelloch scan or just a global atomic counter for now?
// Atomic counter is easiest but non-deterministic order.
// Deterministic order is preferred for debugging.
// Let's use a standard 3-phase scan:
// A. Block-wise local scan + write block sums
// B. Scan block sums
// C. Add block sums to local results

// Phase A: Local Scan
__kernel void scan_local(
    __global uint* restrict input,
    __global uint* restrict output,
    __global uint* restrict block_sums,
    const uint n,
    __local uint* temp) {
    
    uint gid = get_global_id(0);
    uint lid = get_local_id(0);
    uint bid = get_group_id(0);
    
    // Load input into shared memory
    if (gid < n) {
        temp[lid] = input[gid];
    } else {
        temp[lid] = 0;
    }
    barrier(CLK_LOCAL_MEM_FENCE);
    
    // Up-sweep (Reduce)
    for (uint stride = 1; stride < WORKGROUP_SIZE; stride *= 2) {
        uint index = (lid + 1) * stride * 2 - 1;
        if (index < WORKGROUP_SIZE) {
            temp[index] += temp[index - stride];
        }
        barrier(CLK_LOCAL_MEM_FENCE);
    }
    
    // Save block sum
    if (lid == WORKGROUP_SIZE - 1) {
        if (block_sums) block_sums[bid] = temp[lid];
        temp[lid] = 0; // Clear last element for exclusive scan
    }
    barrier(CLK_LOCAL_MEM_FENCE);
    
    // Down-sweep
    for (uint stride = WORKGROUP_SIZE / 2; stride > 0; stride /= 2) {
        uint index = (lid + 1) * stride * 2 - 1;
        if (index < WORKGROUP_SIZE) {
            uint t = temp[index - stride];
            temp[index - stride] = temp[index];
            temp[index] += t;
        }
        barrier(CLK_LOCAL_MEM_FENCE);
    }
    
    // Write output
    if (gid < n) {
        output[gid] = temp[lid]; // Exclusive scan result
    }
}

// Phase C: Add block sums
__kernel void scan_add_sums(
    __global uint* restrict output,
    __global const uint* restrict block_sums,
    const uint n) {
    
    uint gid = get_global_id(0);
    uint bid = get_group_id(0);
    
    if (gid < n) {
        output[gid] += block_sums[bid];
    }
}

// 3. Compact Kernel
// Uses the scanned offsets to write valid cells to the new buffer
__kernel void compact_cells(
    __global const int* restrict old_x,
    __global const int* restrict old_y,
    __global const int* restrict old_z,
    __global const uchar* restrict old_levels,
    __global const uchar* restrict old_states,
    __global const uint* restrict old_mat_id,
    __global const float* restrict old_fields,
    
    __global const uint* restrict valid_flags,
    __global const uint* restrict scan_offsets,
    
    __global int* restrict new_x,
    __global int* restrict new_y,
    __global int* restrict new_z,
    __global uchar* restrict new_levels,
    __global uchar* restrict new_states,
    __global uint* restrict new_mat_id,
    __global float* restrict new_fields,
    
    const uint num_cells,
    const uint num_components) {
    
    uint idx = get_global_id(0);
    if (idx >= num_cells) return;
    
    if (valid_flags[idx]) {
        uint new_idx = scan_offsets[idx];
        
        new_x[new_idx] = old_x[idx];
        new_y[new_idx] = old_y[idx];
        new_z[new_idx] = old_z[idx];
        new_levels[new_idx] = old_levels[idx];
        new_states[new_idx] = old_states[idx];
        new_mat_id[new_idx] = old_mat_id[idx];
        
        // Copy fields
        if (old_fields && new_fields) {
            for (uint c = 0; c < num_components; ++c) {
                new_fields[new_idx * num_components + c] = old_fields[idx * num_components + c];
            }
        }
    }
}

// 4. Append Kernel
// Appends new cells (from split/merge) to the end of the compacted buffer
__kernel void append_cells(
    __global const int* restrict src_x,
    __global const int* restrict src_y,
    __global const int* restrict src_z,
    __global const uchar* restrict src_levels,
    __global const uchar* restrict src_states,
    __global const uint* restrict src_mat_id,
    __global const float* restrict src_fields,
    
    __global int* restrict dst_x,
    __global int* restrict dst_y,
    __global int* restrict dst_z,
    __global uchar* restrict dst_levels,
    __global uchar* restrict dst_states,
    __global uint* restrict dst_mat_id,
    __global float* restrict dst_fields,
    
    const uint src_count,
    const uint dst_offset,
    const uint num_components) {
    
    uint idx = get_global_id(0);
    if (idx >= src_count) return;
    
    uint dst_idx = dst_offset + idx;
    
    dst_x[dst_idx] = src_x[idx];
    dst_y[dst_idx] = src_y[idx];
    dst_z[dst_idx] = src_z[idx];
    dst_levels[dst_idx] = src_levels[idx];
    dst_states[dst_idx] = src_states[idx];
    dst_mat_id[dst_idx] = src_mat_id[idx];
    
    if (src_fields && dst_fields) {
        for (uint c = 0; c < num_components; ++c) {
            dst_fields[dst_idx * num_components + c] = src_fields[idx * num_components + c];
        }
    }
}
