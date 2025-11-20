// Find groups of 8 siblings that can be merged
// Siblings must: (1) exist, (2) all have refine_flag == -1, (3) be at same level, (4) be FLUID state

#include "hilbert_encode_3d.cl"

#define INVALID_INDEX 0xFFFFFFFF
#define MAX_REFINEMENT_LEVEL 8

// Kernel 1: Mark sibling groups that are candidates for merging
// Uses a hash map to find siblings quickly (avoids O(N^2) search)
__kernel void mark_sibling_groups(
    __global const int* restrict coord_x,
    __global const int* restrict coord_y,
    __global const int* restrict coord_z,
    __global const uchar* restrict levels,
    __global const int* restrict refine_flags,
    __global const uchar* restrict cell_states,
    __global uint* restrict merge_group_id,  // Output: cell_idx → group_id
    __global uint* restrict group_counter,   // Atomic counter for groups
    __global const ulong* restrict cell_hilbert,  // Pre-computed Hilbert indices
    __global const uint* restrict hash_table,     // Hash table for lookups
    const uint hash_table_size,
    const uint num_cells) {
    
    const uint idx = get_global_id(0);
    if (idx >= num_cells) return;
    
    // Initialize output
    merge_group_id[idx] = INVALID_INDEX;
    
    // Only process cells marked for coarsening AND in FLUID state
    if (refine_flags[idx] != -1 || cell_states[idx] != 0) {
        return;
    }
    
    // Only process cells that are the "first" sibling (x,y,z all even)
    // This ensures each octet is processed exactly once
    if ((coord_x[idx] & 1) != 0 || (coord_y[idx] & 1) != 0 || (coord_z[idx] & 1) != 0) {
        return;
    }
    
    // Compute parent coordinates
    const int parent_x = coord_x[idx] >> 1;
    const int parent_y = coord_y[idx] >> 1;
    const int parent_z = coord_z[idx] >> 1;
    const uchar current_level = levels[idx];
    
    // Check all 8 siblings exist and are coarsenable
    uchar present_mask = 0;
    uint sibling_indices[8];
    
    for (uchar child = 0; child < 8; ++child) {
        // Compute sibling coordinate
        const int sx = (parent_x << 1) | ((child >> 0) & 1);
        const int sy = (parent_y << 1) | ((child >> 1) & 1);
        const int sz = (parent_z << 1) | ((child >> 2) & 1);
        
        // Compute Hilbert index for sibling
        const ulong sibling_hilbert = hilbert_encode_3d(sx, sy, sz, MAX_REFINEMENT_LEVEL);
        
        // Query hash table to find sibling index
        uint hash = sibling_hilbert % hash_table_size;
        uint sibling_idx = INVALID_INDEX;
        
        // Linear probing
        for (uint probe = 0; probe < 64; ++probe) {
            uint idx = hash_table[hash];
            if (idx == INVALID_INDEX) break;
            
            // Verify match
            if (levels[idx] == current_level &&
                coord_x[idx] == sx &&
                coord_y[idx] == sy &&
                coord_z[idx] == sz) {
                sibling_idx = idx;
                break;
            }
            hash = (hash + 1) % hash_table_size;
        }
        
        // If any sibling missing, cannot merge
        if (sibling_idx == INVALID_INDEX) {
            return;
        }
        
        // Verify sibling is coarsenable
        if (refine_flags[sibling_idx] != -1 || 
            levels[sibling_idx] != current_level ||
            cell_states[sibling_idx] != 0) {
            return;
        }
        
        present_mask |= (1 << child);
        sibling_indices[child] = sibling_idx;
    }
    
    // All 8 siblings found and valid - assign group ID
    if (present_mask == 0xFF) {
        const uint group_id = atomic_inc(group_counter);
        
        // Update all siblings in the group
        for (uchar child = 0; child < 8; ++child) {
            merge_group_id[sibling_indices[child]] = group_id;
        }
    }
}

inline void atomic_add_float(volatile __global float *addr, float val) {
    union {
        uint u;
        float f;
    } next, expected, current;
    current.f = *addr;
    do {
        expected.f = current.f;
        next.f = expected.f + val;
        current.u = atomic_cmpxchg((volatile __global uint*)addr, expected.u, next.u);
    } while (current.u != expected.u);
}

// Kernel 2: Average fields for merged group based on averaging rule
// This is the critical kernel for conservation properties
__kernel void merge_fields(
    __global const uint* restrict merge_group_id,
    __global const uint* restrict group_to_parent,
    __global const float* restrict input_field,
    __global float* restrict output_field,
    const uint num_components,
    const uint averaging_rule,  // 0=arithmetic, 1=volume_weighted
    const uint num_cells) {
    
    const uint idx = get_global_id(0);
    if (idx >= num_cells) return;
    
    const uint group_id = merge_group_id[idx];
    if (group_id == INVALID_INDEX) return;
    
    const uint parent_idx = group_to_parent[group_id];
    
    // Accumulate contributions from all 8 children
    // Each thread processes one child, uses atomic add to parent
    for (uint comp = 0; comp < num_components; ++comp) {
        const float child_value = input_field[idx * num_components + comp];
        float contribution = child_value;
        
        if (averaging_rule == 0) {
            // Arithmetic mean: divide by 8
            contribution *= 0.125f;  // 1/8
        } else if (averaging_rule == 1) {
            // Volume-weighted: sum of children = parent (conserved)
            // No scaling needed for conserved quantities
            contribution = child_value;
        }
        
        // Atomic add to parent field
        // Atomic add to parent field
        atomic_add_float(&output_field[parent_idx * num_components + comp], contribution);
    }
}

// Kernel 3: Create parent cell descriptors
__kernel void create_parent_cells(
    __global const int* restrict child_x,
    __global const int* restrict child_y,
    __global const int* restrict child_z,
    __global const uchar* restrict child_level,
    __global const uchar* restrict child_states,
    __global const uint* restrict child_material_id,
    __global const uint* restrict merge_group_id,
    __global const uint* restrict group_to_parent,
    __global int* restrict parent_x,
    __global int* restrict parent_y,
    __global int* restrict parent_z,
    __global uchar* restrict parent_level,
    __global uchar* restrict parent_states,
    __global uint* restrict parent_material_id,
    const uint num_cells) {
    
    const uint idx = get_global_id(0);
    if (idx >= num_cells) return;
    
    const uint group_id = merge_group_id[idx];
    if (group_id == INVALID_INDEX) return;
    
    // Only the first sibling creates the parent
    if ((child_x[idx] & 1) != 0 || (child_y[idx] & 1) != 0 || (child_z[idx] & 1) != 0) {
        return;
    }
    
    const uint parent_idx = group_to_parent[group_id];
    
    // Create parent cell
    parent_x[parent_idx] = child_x[idx] >> 1;
    parent_y[parent_idx] = child_y[idx] >> 1;
    parent_z[parent_idx] = child_z[idx] >> 1;
    parent_level[parent_idx] = child_level[idx] - 1;
    parent_states[parent_idx] = child_states[idx];
    parent_material_id[parent_idx] = child_material_id[idx];
}
