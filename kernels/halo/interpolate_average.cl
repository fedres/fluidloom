// Helper function for volume-weighted averaging

// Forward declaration
__device void volume_weighted_average(
    __global float* field_data,
    const uint coarse_cell_idx,
    __global const float* pack_data,
    __constant TrilinearParams* params,
    const uint num_components
);

__device void volume_weighted_average(
    __global float* field_data,
    const uint coarse_cell_idx,
    __global const float* pack_data,
    __constant TrilinearParams* params,
    const uint num_components
) {
    // This GPU is coarse, remote is fine. Average 8 fine cells into 1 coarse.
    float result[32] = {0};
    
    // Sum all incoming fine cell values (already averaged on remote side)
    // We receive pre-averaged values because fine→coarse can't be done locally
    for (uint comp = 0; comp < num_components; ++comp) {
        result[comp] = pack_data[comp];
    }
    
    // Write to coarse cell
    for (uint comp = 0; comp < num_components; ++comp) {
        field_data[coarse_cell_idx * num_components + comp] = result[comp];
    }
}
