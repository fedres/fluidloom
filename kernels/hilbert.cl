#pragma OPENCL EXTENSION cl_khr_fp64 : enable

// Exact same constants as CPU
#define MAX_REFINEMENT_LEVEL 8
#define COORDINATE_BITS 21
#define HILBERT_EMPTY 0xFFFFFFFFFFFFFFFFULL
#define HILBERT_INVALID 0xFFFFFFFFFFFFFFFEULL

// Rotation tables in constant memory
constant uchar HILBERT_TABLE[12][8] = {
    {8, 17, 27, 18, 47, 38, 28, 37},
    {16, 71, 1, 62, 51, 52, 2, 61},
    {0, 75, 95, 76, 9, 10, 86, 85},
    {4, 77, 55, 78, 3, 66, 80, 65},
    {50, 49, 45, 46, 67, 88, 68, 7},
    {6, 57, 39, 72, 5, 58, 84, 83},
    {12, 11, 29, 90, 79, 32, 30, 89},
    {74, 91, 73, 40, 69, 92, 70, 15},
    {14, 13, 81, 82, 63, 36, 24, 35},
    {20, 31, 19, 56, 53, 54, 42, 41},
    {26, 93, 43, 44, 25, 94, 64, 23},
    {22, 87, 21, 60, 33, 48, 34, 59},
};

constant uchar INV_HILBERT_TABLE[12][8] = {
    {8, 17, 19, 26, 30, 39, 37, 44},
    {16, 2, 6, 52, 53, 63, 59, 65},
    {0, 12, 13, 73, 75, 87, 86, 90},
    {86, 71, 69, 4, 0, 73, 75, 50},
    {93, 49, 48, 68, 70, 42, 43, 7},
    {75, 57, 61, 87, 86, 4, 0, 34},
    {37, 95, 91, 9, 8, 26, 30, 76},
    {43, 74, 72, 89, 93, 68, 70, 15},
    {30, 82, 83, 39, 37, 9, 8, 60},
    {59, 47, 46, 18, 16, 52, 53, 25},
    {70, 28, 24, 42, 43, 89, 93, 23},
    {53, 36, 38, 63, 59, 18, 16, 81},
};

// Device-side encode function
inline ulong hilbert_encode_3d(int x, int y, int z, uchar level) {
    // Truncate coordinates
    uint ux = (uint)x & ((1u << COORDINATE_BITS) - 1);
    uint uy = (uint)y & ((1u << COORDINATE_BITS) - 1);
    uint uz = (uint)z & ((1u << COORDINATE_BITS) - 1);
    
    ulong h = 0;
    uchar state = 0;
    
    for (char i = level - 1; i >= 0; --i) {
        uint bit_x = (ux >> i) & 1;
        uint bit_y = (uy >> i) & 1;
        uint bit_z = (uz >> i) & 1;
        
        uchar quadrant = (uchar)((bit_z << 2) | (bit_y << 1) | bit_x);
        uchar val = HILBERT_TABLE[state][quadrant];
        uchar curve_idx = val & 0x7;
        state = val >> 3;
        
        h = (h << 3) | curve_idx;
    }
    
    // Canonicalize
    uchar shift = 64 - (level * 3);
    if (shift < 64) {
        h <<= shift;
        h >>= shift;
    }
    
    return h;
}

// Device-side decode function
inline void hilbert_decode_3d(ulong hilbert, uchar level, __private int* x, __private int* y, __private int* z) {
    if (level == 0) {
        *x = *y = *z = 0;
        return;
    }
    
    hilbert <<= (64 - level * 3);
    
    uint ux = 0, uy = 0, uz = 0;
    uchar state = 0;
    
    for (uchar i = 0; i < level; ++i) {
        uchar curve_idx = (uchar)((hilbert >> 61) & 0b111);
        uchar val = INV_HILBERT_TABLE[state][curve_idx];
        uchar quadrant = val & 0x7;
        state = val >> 3;
        
        ux = (ux << 1) | (quadrant & 1);
        uy = (uy << 1) | ((quadrant >> 1) & 1);
        uz = (uz << 1) | ((quadrant >> 2) & 1);
        
        hilbert <<= 3;
    }
    
    *x = (int)ux;
    *y = (int)uy;
    *z = (int)uz;
}

// Test kernel for validation
__kernel void test_hilbert_encode(
    __global const int3* coords,
    __global ulong* hilbert_out,
    const uint num_cells,
    const uchar level
) {
    uint gid = get_global_id(0);
    if (gid >= num_cells) return;
    
    int3 coord = coords[gid];
    hilbert_out[gid] = hilbert_encode_3d(coord.x, coord.y, coord.z, level);
}

// Decode test kernel
__kernel void test_hilbert_decode(
    __global const ulong* hilbert_in,
    __global int3* coords_out,
    const uint num_cells,
    const uchar level
) {
    uint gid = get_global_id(0);
    if (gid >= num_cells) return;
    
    int x, y, z;
    hilbert_decode_3d(hilbert_in[gid], level, &x, &y, &z);
    coords_out[gid] = (int3)(x, y, z);
}
