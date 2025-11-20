// Unpack kernel: opposite of pack, handles fine→coarse averaging

#include "fluidloom_preamble.cl"
#include "interpolation_constants.cl"

__kernel void unpack_halo(
    __global float* field_data,                 // Destination field SOA
    __global const uint* ghost_cell_indices,    // Where to write unpacked data
    __global const uchar* levels,               // cell_level array
    __global GhostRange* ghost_ranges,
    __global const float* pack_buffer,          // Source buffer (received data)
    __constant TrilinearParams* interp_params,
    const uint range_id,
    const uint field_idx,
    const uint num_components
) {
    const uint gid = get_global_id(0);
    const uint range_offset = ghost_ranges[range_id].pack_offset / sizeof(float);
    const uint num_cells = ghost_ranges[range_id].num_cells;
    
    if (gid >= num_cells) return;
    
    const uint ghost_idx = ghost_cell_indices[gid];
    const uint pack_idx = range_offset + gid * num_components;
    
    if (ghost_ranges[range_id].requires_interpolation) {
        const uint local_level = levels[ghost_idx];
        const uint remote_level = ghost_ranges[range_id].remote_level;
        
        if (local_level < remote_level) {
            // Fine → Coarse: Volume-weighted average
            volume_weighted_average(field_data, ghost_idx, pack_buffer + pack_idx,
                                  interp_params, num_components);
        } else {
            // Coarse → Fine: Direct write (interpolation done on remote)
            for (uint comp = 0; comp < num_components; ++comp) {
                field_data[ghost_idx * num_components + comp] = pack_buffer[pack_idx + comp];
            }
        }
    } else {
        // Direct write
        for (uint comp = 0; comp < num_components; ++comp) {
            field_data[ghost_idx * num_components + comp] = pack_buffer[pack_idx + comp];
        }
    }
}
