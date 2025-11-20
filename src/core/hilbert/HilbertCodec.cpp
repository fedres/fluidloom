#include "fluidloom/core/hilbert/HilbertCodec.h"
#include "fluidloom/core/hilbert/BitOps.h"
#include "fluidloom/common/FluidLoomError.h"
#include "fluidloom/common/Logger.h"
#include <algorithm>
#include <stdexcept>

namespace fluidloom {
namespace hilbert {

// Generated Hilbert Table. States: 12
// Format: (next_state << 3) | output_quadrant
static constexpr uint8_t HILBERT_TABLE[12][8] = {
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

// Inverse: [state][curve_index] -> (next_state << 3) | geometric_quadrant
static constexpr uint8_t INV_HILBERT_TABLE[12][8] = {
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

HilbertIndex encode(int32_t x, int32_t y, int32_t z, uint8_t level) {
    // Truncate coordinates to COORDINATE_BITS bits
    uint32_t ux = static_cast<uint32_t>(x) & ((1u << COORDINATE_BITS) - 1);
    uint32_t uy = static_cast<uint32_t>(y) & ((1u << COORDINATE_BITS) - 1);
    uint32_t uz = static_cast<uint32_t>(z) & ((1u << COORDINATE_BITS) - 1);
    
    HilbertIndex h = 0;
    uint8_t state = 0;  // Initial state 0
    
    // Process bits from most significant to least
    for (int8_t i = level - 1; i >= 0; --i) {
        // Extract i-th bit of each coordinate
        uint32_t bit_x = (ux >> i) & 1;
        uint32_t bit_y = (uy >> i) & 1;
        uint32_t bit_z = (uz >> i) & 1;
        
        // Form 3-bit quadrant index
        uint8_t quadrant = (bit_z << 2) | (bit_y << 1) | bit_x;
        
        // Lookup table
        uint8_t val = HILBERT_TABLE[state][quadrant];
        uint8_t curve_idx = val & 0x7;
        state = val >> 3;
        
        // Append 3 bits to Hilbert index
        h = (h << 3) | curve_idx;
    }
    
    // Zero out bits beyond level*3 for canonical form
    uint8_t shift = 64 - (level * 3);
    if (shift < 64) {
        h <<= shift;
        h >>= shift;
    }
    
    return h;
}

void decode(HilbertIndex hilbert, uint8_t level, int32_t& x, int32_t& y, int32_t& z) {
    if (level == 0) {
        x = y = z = 0;
        return;
    }
    
    // Canonicalize: shift bits to MSB for processing
    HilbertIndex temp_h = hilbert << (64 - level * 3);
    
    uint32_t ux = 0, uy = 0, uz = 0;
    uint8_t state = 0;
    
    for (uint8_t i = 0; i < level; ++i) {
        // Extract top 3 bits
        uint8_t curve_idx = (temp_h >> 61) & 0b111;
        
        // Lookup inverse table
        uint8_t val = INV_HILBERT_TABLE[state][curve_idx];
        uint8_t quadrant = val & 0x7;
        state = val >> 3;
        
        // Append bits to coordinates
        ux = (ux << 1) | (quadrant & 1);
        uy = (uy << 1) | ((quadrant >> 1) & 1);
        uz = (uz << 1) | ((quadrant >> 2) & 1);
        
        // Shift for next iteration
        temp_h <<= 3;
    }
    
    x = static_cast<int32_t>(ux);
    y = static_cast<int32_t>(uy);
    z = static_cast<int32_t>(uz);
}

HilbertIndex getParent(HilbertIndex hilbert, uint8_t level) {
    if (level == 0) {
        throw std::invalid_argument("Cannot get parent of level 0 cell");
    }
    // Shift right by 3 bits to drop the least significant quadrant
    return hilbert >> 3;
}

HilbertIndex getChild(HilbertIndex hilbert, uint8_t level, uint8_t quadrant) {
    if (level >= MAX_REFINEMENT_LEVEL) {
        throw std::invalid_argument("Cannot get child at max level");
    }
    if (quadrant > 7) {
        throw std::invalid_argument("Quadrant must be 0-7");
    }
    // Shift left by 3 bits and add quadrant
    return (hilbert << 3) | quadrant;
}

bool isValid(HilbertIndex hilbert, uint8_t level) {
    // Check empty marker
    if (hilbert == HILBERT_EMPTY) return false;
    
    // Check for overflow into bit 63
    if (hilbert & (1ULL << 63)) return false;
    
    // Verify bits beyond level*3 are zero
    uint8_t unused_bits = 64 - (level * 3);
    if (unused_bits > 0 && unused_bits < 64) {
        if (hilbert >> (level * 3)) return false;
    }
    
    return true;
}

} // namespace hilbert
} // namespace fluidloom
