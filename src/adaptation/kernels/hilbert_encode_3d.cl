// Optimized non-recursive 3D Hilbert curve encoding
// Based on "Compact Hilbert Indices" by Hamilton 2008
// Input: 21-bit coordinates, max_level (0-8)
// Output: 63-bit Hilbert index

#ifndef HILBERT_ENCODE_3D_CL
#define HILBERT_ENCODE_3D_CL

// Rotation tables for Hilbert curve generation
constant uchar rotation_table[8][8] = {
    {0, 1, 2, 3, 4, 5, 6, 7},
    {1, 0, 7, 6, 5, 4, 3, 2},
    {2, 3, 0, 1, 6, 7, 4, 5},
    {3, 2, 5, 4, 7, 6, 1, 0},
    {4, 5, 6, 7, 0, 1, 2, 3},
    {5, 4, 3, 2, 1, 0, 7, 6},
    {6, 7, 4, 5, 2, 3, 0, 1},
    {7, 6, 1, 0, 3, 2, 5, 4}
};

constant uchar direction_table[8][8] = {
    {0, 0, 0, 0, 0, 0, 0, 0},
    {3, 0, 0, 5, 5, 0, 0, 3},
    {4, 4, 0, 3, 3, 0, 4, 4},
    {3, 5, 5, 0, 0, 5, 5, 3},
    {2, 2, 2, 0, 0, 2, 2, 2},
    {3, 0, 0, 5, 5, 0, 0, 3},
    {4, 4, 0, 3, 3, 0, 4, 4},
    {3, 5, 5, 0, 0, 5, 5, 3}
};

inline ulong hilbert_encode_3d(int x, int y, int z, uchar max_level) {
    ulong hilbert = 0;
    uchar rotation = 0;
    
    for (char level = max_level - 1; level >= 0; --level) {
        // Extract current bits
        uchar bx = ((x >> level) & 1);
        uchar by = ((y >> level) & 1);
        uchar bz = ((z >> level) & 1);
        
        // Compute block index (0-7)
        uchar block = (bx << 0) | (by << 1) | (bz << 2);
        
        // Apply rotation
        uchar rotated_block = rotation_table[rotation][block];
        
        // Append to Hilbert index
        hilbert = (hilbert << 3) | rotated_block;
        
        // Update rotation for next level
        rotation = direction_table[rotation][block];
    }
    
    return hilbert;
}

// Decode Hilbert index back to coordinates (for verification)
inline void hilbert_decode_3d(ulong hilbert, uchar max_level, 
                              int* x, int* y, int* z) {
    *x = *y = *z = 0;
    uchar rotation = 0;
    
    for (char level = 0; level < max_level; ++level) {
        // Extract 3 bits from Hilbert index
        uchar block = (hilbert >> (3 * (max_level - 1 - level))) & 7;
        
        // Reverse rotation
        uchar original_block = 0;
        for (int i = 0; i < 8; ++i) {
            if (rotation_table[rotation][i] == block) {
                original_block = i;
                break;
            }
        }
        
        // Append bits to coordinates
        *x = (*x << 1) | ((original_block >> 0) & 1);
        *y = (*y << 1) | ((original_block >> 1) & 1);
        *z = (*z << 1) | ((original_block >> 2) & 1);
        
        // Update rotation
        rotation = direction_table[rotation][original_block];
    }
}

#endif // HILBERT_ENCODE_3D_CL
