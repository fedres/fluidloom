#pragma once

#include "fluidloom/geometry/GeometryDescriptor.h"
#include "fluidloom/core/hilbert/CellCoord.h"
#include <vector>
#include <string>
#include <cmath>

namespace fluidloom {
namespace geometry {

/**
 * @brief Simplified STL Voxelizer
 * 
 * Loads STL files (ASCII or Binary) and voxelizes them using ray casting.
 * Does not depend on external libraries like CGAL.
 */
class SimpleSTLVoxelizer {
public:
    SimpleSTLVoxelizer() = default;
    
    /**
     * @brief Load STL file from disk
     * @param filename Path to STL file
     * @return true if successful
     */
    bool loadSTL(const std::string& filename);
    
    /**
     * @brief Voxelize the loaded mesh
     * 
     * @param geom Geometry descriptor (for transformation)
     * @param cell_size Size of each voxel cell
     * @return List of cells inside the mesh
     */
    std::vector<CellCoord> voxelize(
        const GeometryDescriptor& geom,
        float cell_size
    );
    
private:
    struct Triangle {
        float v1[3], v2[3], v3[3];
        float normal[3];
        
        // Bounding box for acceleration
        float min_x, min_y, min_z;
        float max_x, max_y, max_z;
        
        void computeBounds() {
            min_x = std::min({v1[0], v2[0], v3[0]});
            max_x = std::max({v1[0], v2[0], v3[0]});
            min_y = std::min({v1[1], v2[1], v3[1]});
            max_y = std::max({v1[1], v2[1], v3[1]});
            min_z = std::min({v1[2], v2[2], v3[2]});
            max_z = std::max({v1[2], v2[2], v3[2]});
        }
    };
    
    std::vector<Triangle> m_triangles;
    
    // Bounding box of the entire mesh
    float m_mesh_min_x, m_mesh_min_y, m_mesh_min_z;
    float m_mesh_max_x, m_mesh_max_y, m_mesh_max_z;
    
    /**
     * @brief Check if point is inside mesh using ray casting
     * Casts a ray in +X direction and counts intersections
     */
    bool isInside(float x, float y, float z) const;
    
    /**
     * @brief Ray-triangle intersection test (Möller–Trumbore algorithm)
     */
    bool rayIntersectsTriangle(
        const Triangle& tri,
        float ox, float oy, float oz,  // Ray origin
        float dx, float dy, float dz   // Ray direction
    ) const;
    
    // Helper to parse ASCII STL
    bool loadAsciiSTL(const std::string& filename);
    
    // Helper to parse Binary STL
    bool loadBinarySTL(const std::string& filename);
};

} // namespace geometry
} // namespace fluidloom
