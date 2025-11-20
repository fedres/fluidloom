#pragma once

#include <cstdint>
#include <array>

namespace fluidloom {
namespace hilbert {

// Maximum refinement level - DO NOT CHANGE
static constexpr uint8_t MAX_REFINEMENT_LEVEL = 8;
static constexpr uint8_t COORDINATE_BITS = 21;  // 21 bits per dimension

// Resulting Hilbert index type
using HilbertIndex = uint64_t;

// Empty marker for hash tables
static constexpr HilbertIndex HILBERT_EMPTY = 0xFFFFFFFFFFFFFFFFULL;
static constexpr HilbertIndex HILBERT_INVALID = 0xFFFFFFFFFFFFFFFEULL;

/**
 * @brief Encode 3D integer coordinates to Hilbert index
 * 
 * @param x X-coordinate (will be masked to 21 bits)
 * @param y Y-coordinate (will be masked to 21 bits)
 * @param z Z-coordinate (will be masked to 21 bits)
 * @param level Refinement level (0-8). Bits beyond level*3 are ignored.
 * @return HilbertIndex 64-bit Hilbert index with bit[63]=0
 */
HilbertIndex encode(int32_t x, int32_t y, int32_t z, uint8_t level = MAX_REFINEMENT_LEVEL);

/**
 * @brief Decode Hilbert index to 3D coordinates
 * 
 * @param hilbert Hilbert index (bit[63] must be 0)
 * @param level Refinement level (must match encoding level)
 * @param x Output X-coordinate
 * @param y Output Y-coordinate
 * @param z Output Z-coordinate
 */
void decode(HilbertIndex hilbert, uint8_t level, int32_t& x, int32_t& y, int32_t& z);

/**
 * @brief Get parent Hilbert index at coarser level
 * 
 * @param hilbert Child index at level L
 * @param level Current level L
 * @return HilbertIndex Parent index at level L-1
 */
HilbertIndex getParent(HilbertIndex hilbert, uint8_t level);

/**
 * @brief Get child Hilbert index at finer level (quadrant 0-7)
 * 
 * @param hilbert Parent index at level L
 * @param level Current level L
 * @param quadrant Which child quadrant (0-7)
 * @return HilbertIndex Child index at level L+1
 */
HilbertIndex getChild(HilbertIndex hilbert, uint8_t level, uint8_t quadrant);

/**
 * @brief Validate that a Hilbert index is in canonical form
 * 
 * @param hilbert Index to validate
 * @param level Level to validate against
 * @return bool True if valid (bit[63]=0 and bits beyond level*3 are 0)
 */
bool isValid(HilbertIndex hilbert, uint8_t level);

} // namespace hilbert
} // namespace fluidloom
