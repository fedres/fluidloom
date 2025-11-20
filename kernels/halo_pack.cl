// Packing kernel
// Gathers data from field arrays into a contiguous buffer
__kernel void pack_fields(
    __global const float* field_data,
    __global uchar* packed_buffer,
    const ulong num_cells,
    const ulong field_offset_bytes
) {
    size_t gid = get_global_id(0);
    if (gid >= num_cells) return;
    
    // Simple float packing for now
    // In reality, we need to handle different types and strides
    // This is a simplified version assuming float fields
    
    float val = field_data[gid];
    
    // Calculate destination address
    size_t dest_offset = field_offset_bytes + (gid * sizeof(float));
    
    // Store float as bytes
    __global float* dest = (__global float*)(packed_buffer + dest_offset);
    *dest = val;
}

// Unpacking kernel
// Scatters data from contiguous buffer to field arrays
__kernel void unpack_fields(
    __global const uchar* packed_buffer,
    __global float* field_data,
    const ulong num_cells,
    const ulong field_offset_bytes
) {
    size_t gid = get_global_id(0);
    if (gid >= num_cells) return;
    
    // Calculate source address
    size_t src_offset = field_offset_bytes + (gid * sizeof(float));
    
    // Read float from bytes
    __global const float* src = (__global const float*)(packed_buffer + src_offset);
    float val = *src;
    
    field_data[gid] = val;
}
