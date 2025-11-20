// Helper function for trilinear interpolation

// Forward declaration
__device void trilinear_interpolate(
    __global const float* field_data,
    const uint fine_cell_idx,
    __global float* output,
    __constant TrilinearParams* params,
    const uint num_components
);

__device void trilinear_interpolate(
    __global const float* field_data,
    const uint fine_cell_idx,
    __global float* output,
    __constant TrilinearParams* params,
    const uint num_components
) {
    // Get coordinate of fine cell
    int3 fine_coord = get_cell_coord(fine_cell_idx);
    
    // Find containing coarse cell (divide by 2)
    int3 coarse_coord = fine_coord / 2;
    
    // Iterate over 8 coarse corner cells
    float result[32];  // Max 32 components
    for (uint comp = 0; comp < num_components; ++comp) {
        result[comp] = 0.0f;
    }
    
    for (uint corner = 0; corner < 8; ++corner) {
        int3 offset = (int3)(params->offset_x[corner], 
                           params->offset_y[corner], 
                           params->offset_z[corner]);
        int3 neighbor_coord = coarse_coord + offset;
        
        // Query hash map for coarse cell index
        uint64_t neighbor_hilbert = hilbert_encode_3d(neighbor_coord.x, 
                                                    neighbor_coord.y, 
                                                    neighbor_coord.z, 
                                                    MAX_LEVEL);
        uint neighbor_idx = hash_query(neighbor_hilbert);
        
        if (neighbor_idx != INVALID_INDEX) {
            float weight = params->weights[corner];
            for (uint comp = 0; comp < num_components; ++comp) {
                result[comp] += weight * field_data[neighbor_idx * num_components + comp];
            }
        }
    }
    
    // Write interpolated result
    for (uint comp = 0; comp < num_components; ++comp) {
        output[comp] = result[comp];
    }
}
