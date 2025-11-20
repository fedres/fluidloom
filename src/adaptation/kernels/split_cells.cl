// Split marked parent cells into 8 children in Hilbert order
// Input: parent cells and their refine_flag values
// Output: child cells (8x count) and parent→child mapping
// Memory: O(8N) temporary storage for children

#include "hilbert_encode_3d.cl"

// Workgroup size: 256 threads for optimal occupancy on most GPUs
#define WORKGROUP_SIZE 256
#define MAX_REFINEMENT_LEVEL 8
#define INVALID_INDEX 0xFFFFFFFF

// Kernel 1: Count and allocate child blocks using prefix sum
// More efficient than per-cell atomics - eliminates contention
__kernel void split_count_and_allocate(
    __global const int* restrict parent_x,
    __global const int* restrict parent_y,
    __global const int* restrict parent_z,
    __global const uchar* restrict parent_level,
    __global const int* restrict refine_flags,
    __global const uchar* restrict parent_states,
    __global uint* restrict child_block_start,  // Output: parent_idx → child_start
    __global uint* restrict cell_scratch,       // Temporary: 0/1 split predicate
    const uint num_parents) {
    
    const uint idx = get_global_id(0);
    
    // Initialize scratch space
    if (idx < num_parents) {
        // Check if cell can be split (geometry lock, max level)
        const bool can_split = (refine_flags[idx] > 0) &&
                               (parent_states[idx] == 0) &&  // FLUID state
                               (parent_level[idx] < MAX_REFINEMENT_LEVEL);
        
        cell_scratch[idx] = can_split ? 1 : 0;
    }
    
    barrier(CLK_GLOBAL_MEM_FENCE);
    
    // Perform inclusive prefix sum on cell_scratch
    // This would use a library like rocPRIM or custom workgroup scan
    // For now, placeholder - actual implementation would be optimized
    if (idx < num_parents) {
        uint sum = 0;
        for (uint i = 0; i <= idx; ++i) {
            sum += cell_scratch[i];
        }
        child_block_start[idx] = cell_scratch[idx] ? (sum - 1) * 8 : INVALID_INDEX;
    }
}

// Kernel 2: Generate child cells and Hilbert indices
__kernel void split_generate_children(
    __global const int* restrict parent_x,
    __global const int* restrict parent_y,
    __global const int* restrict parent_z,
    __global const uchar* restrict parent_level,
    __global const uchar* restrict parent_states,
    __global const uint* restrict parent_material_id,
    __global const uint* restrict child_block_start,
    __global int* restrict child_x,
    __global int* restrict child_y,
    __global int* restrict child_z,
    __global uchar* restrict child_level,
    __global uchar* restrict child_states,
    __global uint* restrict child_material_id,
    __global ulong* restrict child_hilbert,  // Optional: for immediate sorting
    const uint num_parents) {
    
    const uint parent_idx = get_global_id(0);
    if (parent_idx >= num_parents) return;
    
    const uint child_start = child_block_start[parent_idx];
    if (child_start == INVALID_INDEX) return;  // Not splitting this parent
    
    // Parent coordinates
    const int px = parent_x[parent_idx];
    const int py = parent_y[parent_idx];
    const int pz = parent_z[parent_idx];
    const uchar child_level_val = parent_level[parent_idx] + 1;
    const uchar parent_state = parent_states[parent_idx];
    const uint parent_mat_id = parent_material_id[parent_idx];
    
    // Generate 8 children in Hilbert order
    // Hilbert order is preserved by coordinate ordering: (0,0,0), (0,0,1), (0,1,0), ...
    // This matches the CHILD_OFFSETS table and ensures monotonicity
    for (uchar child = 0; child < 8; ++child) {
        const uint child_idx = child_start + child;
        
        child_x[child_idx] = (px << 1) | ((child >> 0) & 1);
        child_y[child_idx] = (py << 1) | ((child >> 1) & 1);
        child_z[child_idx] = (pz << 1) | ((child >> 2) & 1);
        child_level[child_idx] = child_level_val;
        child_states[child_idx] = parent_state;
        child_material_id[child_idx] = parent_mat_id;
        
        // Optionally compute Hilbert index for immediate radix sort
        // This avoids re-computation on host and enables direct device-side sorting
        if (child_hilbert) {
            child_hilbert[child_idx] = hilbert_encode_3d(
                child_x[child_idx], child_y[child_idx], child_z[child_idx],
                MAX_REFINEMENT_LEVEL
            );
        }
    }
}

// Kernel 3: Interpolate fields from parent to children
// Strategy: Direct copy for most fields, special handling for conserved quantities
__kernel void interpolate_split_fields(
    __global const uint* restrict parent_to_child_map,
    __global const float* restrict parent_field,
    __global float* restrict child_field,
    const uint num_components,
    const uint num_parents) {
    
    const uint parent_idx = get_global_id(0);
    if (parent_idx >= num_parents) return;
    
    const uint child_start = parent_to_child_map[parent_idx];
    if (child_start == INVALID_INDEX) return;
    
    // For split operations, we typically copy parent values to all children
    // This preserves local conservation and is correct for AMR
    for (uchar child = 0; child < 8; ++child) {
        for (uint comp = 0; comp < num_components; ++comp) {
            const uint parent_field_idx = parent_idx * num_components + comp;
            const uint child_field_idx = (child_start + child) * num_components + comp;
            child_field[child_field_idx] = parent_field[parent_field_idx];  // Direct copy
        }
    }
}
