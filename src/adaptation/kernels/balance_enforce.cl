// 2:1 Balance enforcement kernels
// Detect level violations and mark cells for cascading refinement

#include "hilbert_encode_3d.cl"

#define INVALID_INDEX 0xFFFFFFFF
#define MAX_REFINEMENT_LEVEL 8

// Helper to find a cell containing a point (px, py, pz)
// Returns the index of the cell if found, or INVALID_INDEX
uint find_cell_at_point(
    int px, int py, int pz,
    __global const uint* restrict hash_table,
    const uint hash_table_size) {
    
    // Probe all levels from MAX down to 0 (or 0 to MAX, doesn't matter if mesh is valid)
    // We iterate 0 to MAX because coarser cells are more likely? No.
    // Just iterate.
    for (int l = 0; l <= MAX_REFINEMENT_LEVEL; ++l) {
        int size = 1 << (MAX_REFINEMENT_LEVEL - l);
        int mask = ~(size - 1);
        
        int ax = px & mask;
        int ay = py & mask;
        int az = pz & mask;
        
        ulong hilbert = hilbert_encode_3d(ax, ay, az, MAX_REFINEMENT_LEVEL);
        uint hash = hilbert % hash_table_size;
        uint idx = hash_table[hash];
        
        // Note: This simple hash lookup assumes no collisions or that we don't handle them.
        // If we have collisions, we might get the wrong cell.
        // But for now we assume the hash table is built with the same logic (direct mapping).
        // If the hash table stores the cell index, we should verify the cell matches the key?
        // But we don't have the keys stored in the table, only indices.
        // We can check the cell's coordinates and level.
        // But we don't have access to cell arrays here easily without passing them.
        // Wait, we DO have access to coord_x, coord_y, etc. in the main kernel.
        // But this helper doesn't.
        // So we rely on the hash being correct.
        
        if (idx != INVALID_INDEX) {
            // We found *something*. Is it the correct cell?
            // We should verify, but for now assume yes.
            // Wait, if we probe L=0 and find a cell, but the actual cell is L=2...
            // The L=0 anchor might hash to a slot occupied by some other cell.
            // This is risky.
            // But we can't verify without accessing global memory.
            // Let's inline this logic in the kernel.
            return idx;
        }
    }
    return INVALID_INDEX;
}

// Kernel 1: Detect 2:1 balance violations
__kernel void detect_balance_violations(
    __global const int* restrict coord_x,
    __global const int* restrict coord_y,
    __global const int* restrict coord_z,
    __global const uchar* restrict levels,
    __global const uchar* restrict cell_states,
    __global const ulong* restrict cell_hilbert,
    __global const uint* restrict hash_table,
    const uint hash_table_size,
    __global uchar* restrict violation_flags,
    __global uint* restrict violation_count,
    const uint num_cells) {
    
    const uint idx = get_global_id(0);
    if (idx >= num_cells) return;
    
    violation_flags[idx] = 0;
    
    // Skip non-fluid cells
    if (cell_states[idx] != 0) return;
    
    const int cx = coord_x[idx];
    const int cy = coord_y[idx];
    const int cz = coord_z[idx];
    const uchar my_level = levels[idx];
    const int my_size = 1 << (MAX_REFINEMENT_LEVEL - my_level);
    
    // Check 6 face neighbors
    // Directions: -X, +X, -Y, +Y, -Z, +Z
    // Test points:
    // -X: cx - 1
    // +X: cx + my_size
    // ...
    
    const int test_points[6][3] = {
        {cx - 1, cy, cz},             // -X
        {cx + my_size, cy, cz},       // +X
        {cx, cy - 1, cz},             // -Y
        {cx, cy + my_size, cz},       // +Y
        {cx, cy, cz - 1},             // -Z
        {cx, cy, cz + my_size}        // +Z
    };
    
    for (int n = 0; n < 6; ++n) {
        int px = test_points[n][0];
        int py = test_points[n][1];
        int pz = test_points[n][2];
        
        // Probe for neighbor
        bool found = false;
        for (int l = 0; l <= MAX_REFINEMENT_LEVEL; ++l) {
            int size = 1 << (MAX_REFINEMENT_LEVEL - l);
            int mask = ~(size - 1);
            
            int ax = px & mask;
            int ay = py & mask;
            int az = pz & mask;
            
            ulong hilbert = hilbert_encode_3d(ax, ay, az, MAX_REFINEMENT_LEVEL);
            uint hash = hilbert % hash_table_size;
            
            // Linear probing
            for (uint probe = 0; probe < 64; ++probe) {
                uint neighbor_idx = hash_table[hash];
                
                if (neighbor_idx == INVALID_INDEX) break; // Not found
                
                if (levels[neighbor_idx] == l &&
                    coord_x[neighbor_idx] == ax &&
                    coord_y[neighbor_idx] == ay &&
                    coord_z[neighbor_idx] == az) {
                    
                    // Found neighbor!
                    // Only flag violation if neighbor is significantly finer than us
                    // i.e., we are the coarse one that needs to split
                    if (l > my_level + 1) {
                        violation_flags[idx] = 1;
                        atomic_inc(violation_count);
                        return;
                    }
                    found = true;
                    break; // Found the neighbor at this point
                }
                
                hash = (hash + 1) % hash_table_size;
            }
            if (found) break;
        }
        // If not found, it might be a boundary or hole. Ignore.
    }
}

// Kernel 2: Mark cells for cascading refinement
__kernel void mark_cascading_refinement(
    __global const int* restrict coord_x,
    __global const int* restrict coord_y,
    __global const int* restrict coord_z,
    __global const uchar* restrict levels,
    __global const uchar* restrict cell_states,
    __global const uchar* restrict violation_flags,
    __global int* restrict refine_flags,
    __global uint* restrict marked_count,
    const uint num_cells) {
    
    const uint idx = get_global_id(0);
    if (idx >= num_cells) return;
    
    if (!violation_flags[idx]) return;
    if (cell_states[idx] != 0) return;
    if (levels[idx] >= MAX_REFINEMENT_LEVEL) return;
    
    if (refine_flags[idx] <= 0) {
        refine_flags[idx] = 1;
        atomic_inc(marked_count);
    }
}
