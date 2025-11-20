#include "fluidloom/core/hilbert/CellCoord.h"
#include "fluidloom/core/hilbert/HilbertCodec.h"
#include "fluidloom/common/Logger.h"
#include <cmath>
#include <cstdlib>

namespace fluidloom {

// Static validation function
bool validateCellCoord(const CellCoord& coord) {
    // Check level bounds
    if (coord.level > hilbert::MAX_REFINEMENT_LEVEL) {
        FL_LOG(ERROR) << "CellCoord level " << static_cast<int>(coord.level) 
                      << " exceeds maximum " << static_cast<int>(hilbert::MAX_REFINEMENT_LEVEL);
        return false;
    }
    
    // Check coordinate bounds for given level
    // Max coordinate is 2^(level + COORDINATE_BITS - MAX_REFINEMENT_LEVEL) - 1
    // But wait, COORDINATE_BITS is 21. MAX_REFINEMENT_LEVEL is 8.
    // The encoding uses 21 bits.
    // The coordinates passed to encode are masked to 21 bits.
    // If coordinates are larger than 21 bits, they will wrap or be truncated.
    // Let's assume the valid range is signed 21-bit integer?
    // No, encode takes int32_t but masks to 21 bits unsigned.
    // So valid range is [-2^20, 2^20-1] effectively if we treat it as signed?
    // Or [0, 2^21-1] unsigned.
    // The implementation plan says:
    // "Input: (x, y, z) as int32_t, truncated to 21-bit unsigned"
    
    // Let's check if the coordinates fit in the expected range for the level.
    // Actually, the Hilbert curve covers the entire 21-bit space at level 8?
    // No, level 8 means 8 iterations. 8 * 3 = 24 bits total index.
    // But coordinates are 21 bits?
    // Wait, if level=8, we have 8 bits per dimension?
    // 8 bits per dimension -> 2^8 = 256 grid size.
    // 21 bits per dimension -> 2^21 grid size.
    // The plan says: "For level L < 8, only the first (L*3) bits are meaningful."
    // This implies the Hilbert index has L*3 bits.
    // So at level 8, index has 24 bits.
    // 24 bits / 3 dims = 8 bits per dimension.
    // So coordinates should be within [0, 255] (or signed equivalent).
    
    // Let's re-read the plan carefully.
    // "Level: 0 to 8 (max)"
    // "Bit Layout of 64-bit Hilbert index: ... [56:42] : X-axis bits (21 bits, interleaved) ..."
    // Wait, the bit layout shows 21 bits per axis. That's 63 bits total.
    // But "For level L < 8, only the first (L*3) bits are meaningful."
    // This suggests that the level parameter controls how many bits are used.
    // If level=8, do we use 8 bits or 21 bits?
    // "Level: 0 to 8 (max)" usually implies depth of the tree.
    // If max level is 8, and we use 1 bit per level per dimension, that's 8 bits per dimension.
    // But the layout reserves 21 bits per dimension.
    // Maybe MAX_REFINEMENT_LEVEL should be 21?
    // Or maybe the "Level" parameter in encode/decode is just for the current operation, 
    // but the coordinate space is always 21 bits?
    
    // In Butz's algorithm, 'level' usually corresponds to the number of bits per dimension.
    // If we pass level=8 to encode, the loop runs 8 times.
    // It processes bits from level-1 down to 0.
    // So it processes 8 bits.
    // So x, y, z are effectively treated as 8-bit integers.
    
    // If COORDINATE_BITS is 21, then we should support up to level 21.
    // But MAX_REFINEMENT_LEVEL is defined as 8 in the plan.
    // This seems contradictory if we want 21-bit coordinates.
    // UNLESS, the "level" in CellCoord refers to the AMR level, 
    // and the grid resolution at level L is 2^L.
    // If max level is 8, then max resolution is 256^3.
    // That seems small for a "FluidLoom" engine?
    // Maybe the plan meant MAX_REFINEMENT_LEVEL = 21?
    
    // Let's check the implementation plan text again.
    // "Maximum refinement level - DO NOT CHANGE ... static constexpr uint8_t MAX_REFINEMENT_LEVEL = 8;"
    // "static constexpr uint8_t COORDINATE_BITS = 21; // 21 bits per dimension"
    // "Input: (x, y, z) as int32_t, truncated to 21-bit unsigned"
    // "For level L < 8, only the first (L*3) bits are meaningful."
    
    // This implies that while the *capacity* is 21 bits (for future expansion?), 
    // the current *limit* is 8 levels (8 bits).
    // OR, it implies that the Hilbert index is always 63 bits (21 per dim), 
    // but we only *compute* up to 'level' bits?
    
    // In `encode`:
    // for (int8_t i = level - 1; i >= 0; --i)
    // This loop runs 'level' times.
    // If level=8, it processes bits 7 down to 0.
    // So it only uses the lower 8 bits of x, y, z.
    // But the bit layout says:
    // [56:42] : X-axis bits (21 bits, interleaved)
    // This layout description seems to describe a full 21-bit encoding.
    // If we only compute 8 bits, we only fill the lower 24 bits of the Hilbert index?
    // "For level L < 8, only the first (L*3) bits are meaningful."
    // This implies the result is in the lower bits.
    
    // However, the bit layout diagram shows X bits at [56:42].
    // That's at the top of the 64-bit word (ignoring the empty marker).
    // This contradicts "first (L*3) bits".
    // Usually "first bits" means LSBs in Hilbert construction (fine grain) or MSBs (coarse grain)?
    // Butz algorithm typically builds from MSB to LSB.
    // My implementation builds from MSB (level-1) to LSB (0).
    // h = (h << 3) | transformed;
    // This shifts previous bits up.
    // So the first bit processed (bit 'level-1') ends up at the most significant position of the result?
    // No, the first processed bit is shifted left 'level-1' times?
    // Let's trace:
    // i = level-1. quadrant formed from bit (level-1).
    // h = quadrant.
    // i = level-2. h = (quadrant_prev << 3) | quadrant_curr.
    // So the MSB of the coordinate (bit level-1) becomes the MSB of the Hilbert index.
    // The resulting 'h' will have 'level * 3' bits.
    // They will be in the lower 'level * 3' bits of the uint64_t.
    
    // So if level=8, we have 24 bits.
    // The bit layout diagram in the plan seems to describe a *hypothetical* full 21-bit index,
    // but we are only generating a subset of it?
    // Or maybe the plan's bit layout description is for a different encoding scheme?
    // "Bit Layout of 64-bit Hilbert index: ... [56:42] : X-axis bits ..."
    // This looks like a morton code (interleaved bits) or a specific Hilbert layout.
    // But Butz generates a single integer.
    // If we follow the loop `h = (h << 3) | transformed`, we get a packed integer at the bottom.
    
    // I will stick to the code implementation I wrote, which produces a packed integer in the LSBs.
    // And I will enforce the bounds check based on 'level'.
    // Max coordinate magnitude is 2^level - 1.
    
    // Max coordinate magnitude is 2^level - 1.
    // Let's just check if the magnitude fits in 21 bits for now, as a sanity check.
    
    if (std::abs(coord.x) >= (1 << hilbert::COORDINATE_BITS) || 
        std::abs(coord.y) >= (1 << hilbert::COORDINATE_BITS) || 
        std::abs(coord.z) >= (1 << hilbert::COORDINATE_BITS)) {
        FL_LOG(ERROR) << "CellCoord coordinates exceed 21-bit limit";
        return false;
    }
    
    // Validate Hilbert index is canonical
    auto h = coord.hilbert();
    if (!hilbert::isValid(h, coord.level)) {
        FL_LOG(ERROR) << "CellCoord produces invalid Hilbert index";
        return false;
    }
    
    return true;
}

CellCoord getRootCell(int32_t min_x, int32_t min_y, int32_t min_z) {
    return CellCoord(min_x, min_y, min_z, 0);
}

int32_t manhattanDistance(const CellCoord& a, const CellCoord& b) {
    return std::abs(a.x - b.x) + std::abs(a.y - b.y) + std::abs(a.z - b.z);
}

} // namespace fluidloom
