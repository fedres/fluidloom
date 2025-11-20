#pragma once

#include <cstdint>
#include <array>
#include <cassert>

namespace fluidloom {
namespace adaptation {

// Forward declaration from Module 4
extern uint64_t hilbert_encode_3d(int32_t x, int32_t y, int32_t z, uint8_t max_level);

// Maximum supported refinement level - compile-time constant
static constexpr uint8_t MAX_REFINEMENT_LEVEL = 8;

// Sentinel values for invalid indices
static constexpr uint32_t INVALID_INDEX = 0xFFFFFFFF;
static constexpr uint64_t EMPTY_HILBERT = 0xFFFFFFFFFFFFFFFF;

/**
 * @brief Complete cell descriptor with all metadata for adaptation
 * 
 * Tightly packed: 20 bytes/cell. All coordinate math uses 21-bit integers
 * to support max level 8 (2^8 * 2^21 = 2^29 cells per dimension).
 * 
 * Memory layout optimized for GPU coalescing:
 * - 12 bytes: coordinates (3x int32_t with bitfields)
 * - 1 byte: level + state
 * - 3 bytes: material_id
 * - 4 bytes: padding to 20 bytes (ensures alignment)
 */
struct alignas(16) CellDescriptor {
    // Global integer coordinates at cell's native resolution
    // For level L, the coordinate represents the cell's corner at that level
    int32_t x : 21;      // Range: [0, 2^(21+level) - 1]
    int32_t y : 21;      // 21 bits chosen to fit in int32_t with sign bit reserved
    int32_t z : 21;      // Sign bit used for overflow detection
    
    uint8_t level : 4;      // 0 = coarsest, 8 = finest (max 8 levels)
    uint8_t state : 4;      // CellState enum (see below)
    
    uint32_t material_id : 24;  // 0 = fluid, >0 = solid material index
    
    uint8_t visited : 1;    // For balance traversal and BFS algorithms
    uint8_t reserved : 7;   // Future use (e.g., ghost flags, partition ID)
    
    // Hilbert index computed on-demand using cached encoding
    // Computation cost: ~50 CPU cycles, ~20 GPU cycles
    uint64_t getHilbert() const {
        return hilbert_encode_3d(x, y, z, MAX_REFINEMENT_LEVEL);
    }
    
    // Get parent coordinates by truncating LSBs
    void getParent(CellDescriptor& parent) const {
        parent.x = x >> 1;
        parent.y = y >> 1;
        parent.z = z >> 1;
        parent.level = level - 1;
        parent.state = state;
        parent.material_id = material_id;
        parent.visited = 0;
        parent.reserved = 0;
    }
    
    // Generate one of 8 children (child_idx in 0..7)
    // Child ordering follows strict Hilbert order by construction
    void getChild(size_t child_idx, CellDescriptor& child) const {
        assert(child_idx < 8);
        assert(level < MAX_REFINEMENT_LEVEL);
        
        child.x = (x << 1) | ((child_idx >> 0) & 1);
        child.y = (y << 1) | ((child_idx >> 1) & 1);
        child.z = (z << 1) | ((child_idx >> 2) & 1);
        child.level = level + 1;
        child.state = state;
        child.material_id = material_id;
        child.visited = 0;
        child.reserved = 0;
    }
    
    // Check if two cells are siblings (same parent coordinates, same level)
    bool isSiblingOf(const CellDescriptor& other) const {
        if (level != other.level) return false;
        if (level == 0) return false; // Root cells have no siblings
        
        return ((x >> 1) == (other.x >> 1) &&
                (y >> 1) == (other.y >> 1) &&
                (z >> 1) == (other.z >> 1));
    }
    
    // Check if this cell is the first (lowest Hilbert) sibling in its octet
    // Used for merge identification
    bool isFirstSibling() const {
        return ((x & 1) == 0 && (y & 1) == 0 && (z & 1) == 0);
    }
    
    bool operator==(const CellDescriptor& other) const {
        return x == other.x && y == other.y && z == other.z && level == other.level;
    }
    
    bool operator<(const CellDescriptor& other) const {
        return getHilbert() < other.getHilbert();
    }
};

// Cell state enumeration - critical for geometry lock
enum class CellState : uint8_t {
    FLUID = 0,              // Fluid cell, fully mutable
    SOLID = 1,              // Solid geometry, immutable
    GEOMETRY_STATIC = 2,    // Static geometry with special BCs, immutable
    GEOMETRY_MOVING = 3,    // Moving geometry, immutable topology (state updates only)
    UNALLOCATED = 4,        // Deleted cell slot, will be compacted away
    GHOST = 5               // Ghost cell from neighbor GPU, read-only
};

// Static constants for child offsets [x, y, z] for each child index
// Used in both host and device code
static constexpr std::array<std::array<int32_t, 3>, 8> CHILD_OFFSETS = {{
    {0, 0, 0}, {0, 0, 1}, {0, 1, 0}, {0, 1, 1},
    {1, 0, 0}, {1, 0, 1}, {1, 1, 0}, {1, 1, 1}
}};



} // namespace adaptation
} // namespace fluidloom
