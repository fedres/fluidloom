#pragma once

#include "fluidloom/core/hilbert/HilbertCodec.h"
#include "fluidloom/common/FluidLoomError.h"
#include <cstdint>
#include <stdexcept>

namespace fluidloom {

/**
 * @brief Represents a single cell in the octree grid
 * 
 * This struct is the fundamental spatial descriptor used throughout the engine.
 * It MUST remain trivially copyable and have no virtual functions for GPU compatibility.
 */
struct alignas(16) CellCoord {
    int32_t x;      // Global integer coordinate (can be negative)
    int32_t y;
    int32_t z;
    uint8_t level;  // Refinement level: 0 = coarsest, 8 = finest
    
    // Default constructor for container use
    CellCoord() : x(0), y(0), z(0), level(0) {}
    
    // Full constructor
    CellCoord(int32_t ix, int32_t iy, int32_t iz, uint8_t lvl) 
        : x(ix), y(iy), z(iz), level(lvl) {}
    
    // Compute Hilbert index for this cell (cached on demand)
    hilbert::HilbertIndex hilbert() const {
        return hilbert::encode(x, y, z, level);
    }
    
    // Compute parent coordinate (level-1)
    CellCoord parent() const {
        if (level == 0) {
            throw std::invalid_argument("Cannot get parent of level 0 cell");
        }
        // Integer division by 2 (arithmetic shift preserves sign for negative coords)
        return CellCoord(x >> 1, y >> 1, z >> 1, level - 1);
    }
    
    // Compute child coordinate for given quadrant (0-7)
    CellCoord child(uint8_t quadrant) const {
        if (level >= hilbert::MAX_REFINEMENT_LEVEL) {
            throw std::invalid_argument("Cannot get child at max level");
        }
        if (quadrant > 7) {
            throw std::invalid_argument("Quadrant must be 0-7");
        }
        int dx = quadrant & 1;
        int dy = (quadrant >> 1) & 1;
        int dz = (quadrant >> 2) & 1;
        return CellCoord((x << 1) | dx, (y << 1) | dy, (z << 1) | dz, level + 1);
    }
    
    // Check equality (level must match)
    bool operator==(const CellCoord& other) const {
        return x == other.x && y == other.y && z == other.z && level == other.level;
    }
    
    bool operator!=(const CellCoord& other) const {
        return !(*this == other);
    }
    
    // Strict ordering for sorting (Hilbert order)
    bool operator<(const CellCoord& other) const {
        return hilbert() < other.hilbert();
    }
};

/**
 * @brief Range of Hilbert indices [start, end) for partitioning
 */
struct HilbertRange {
    hilbert::HilbertIndex start;
    hilbert::HilbertIndex end;
    
    bool contains(hilbert::HilbertIndex h) const {
        return h >= start && h < end;
    }
    
    size_t size() const {
        return static_cast<size_t>(end - start);
    }
};

// Validation function declaration
bool validateCellCoord(const CellCoord& coord);

// Generate root cell for given domain
CellCoord getRootCell(int32_t min_x, int32_t min_y, int32_t min_z);

// Compute Manhattan distance (for locality validation)
int32_t manhattanDistance(const CellCoord& a, const CellCoord& b);

} // namespace fluidloom
