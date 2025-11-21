#pragma once

#include <cstdint>

namespace fluidloom {
namespace geometry {

/**
 * @brief Represents a single voxelized cell
 * 
 * Used as intermediate format between voxelizer and SOA insertion.
 */
struct VoxelizedCell {
    int32_t x, y, z;       // Grid coordinates
    uint8_t level;         // Refinement level (usually 0)
    int32_t material_id;   // Surface material ID
    uint64_t hilbert_idx;  // Hilbert index for sorting
    
    // Default constructor
    VoxelizedCell() : x(0), y(0), z(0), level(0), material_id(0), hilbert_idx(0) {}
    
    // Constructor
    VoxelizedCell(int32_t _x, int32_t _y, int32_t _z, int32_t _mat, uint64_t _h = 0)
        : x(_x), y(_y), z(_z), level(0), material_id(_mat), hilbert_idx(_h) {}
        
    // Equality for deduplication
    bool operator==(const VoxelizedCell& other) const {
        return x == other.x && y == other.y && z == other.z;
    }
};

} // namespace geometry
} // namespace fluidloom
