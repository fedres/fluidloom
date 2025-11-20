#include <cstdint>

namespace fluidloom {
namespace adaptation {

// Rotation tables for Hilbert curve generation
static const uint8_t rotation_table[8][8] = {
    {0, 1, 2, 3, 4, 5, 6, 7},
    {1, 0, 7, 6, 5, 4, 3, 2},
    {2, 3, 0, 1, 6, 7, 4, 5},
    {3, 2, 5, 4, 7, 6, 1, 0},
    {4, 5, 6, 7, 0, 1, 2, 3},
    {5, 4, 3, 2, 1, 0, 7, 6},
    {6, 7, 4, 5, 2, 3, 0, 1},
    {7, 6, 1, 0, 3, 2, 5, 4}
};

static const uint8_t direction_table[8][8] = {
    {0, 0, 0, 0, 0, 0, 0, 0},
    {3, 0, 0, 5, 5, 0, 0, 3},
    {4, 4, 0, 3, 3, 0, 4, 4},
    {3, 5, 5, 0, 0, 5, 5, 3},
    {2, 2, 2, 0, 0, 2, 2, 2},
    {3, 0, 0, 5, 5, 0, 0, 3},
    {4, 4, 0, 3, 3, 0, 4, 4},
    {3, 5, 5, 0, 0, 5, 5, 3}
};

uint64_t hilbert_encode_3d(int32_t x, int32_t y, int32_t z, uint8_t max_level) {
    uint64_t hilbert = 0;
    uint8_t rotation = 0;
    
    for (int level = max_level - 1; level >= 0; --level) {
        // Extract current bits
        uint8_t bx = ((x >> level) & 1);
        uint8_t by = ((y >> level) & 1);
        uint8_t bz = ((z >> level) & 1);
        
        // Compute block index (0-7)
        uint8_t block = (bx << 0) | (by << 1) | (bz << 2);
        
        // Apply rotation
        uint8_t rotated_block = rotation_table[rotation][block];
        
        // Append to Hilbert index
        hilbert = (hilbert << 3) | rotated_block;
        
        // Update rotation for next level
        rotation = direction_table[rotation][block];
    }
    
    return hilbert;
}

} // namespace adaptation
} // namespace fluidloom
