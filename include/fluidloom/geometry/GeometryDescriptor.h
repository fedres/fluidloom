#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <stdexcept>
#include <cmath>
#include <algorithm>

namespace fluidloom {
namespace geometry {

/**
 * @brief Defines a geometry source and its transformation parameters
 * 
 * This descriptor is parsed from DSL and remains immutable after creation.
 * It contains all information needed to voxelize geometry at base resolution.
 */
struct GeometryDescriptor {
    enum class Type {
        STL_MESH,      // Triangle mesh file
        IMPLICIT_FUNC, // Lambda expression
        BOX,           // Axis-aligned bounding box (implicit)
        SPHERE,        // Implicit sphere
        CYLINDER       // Implicit cylinder
    };
    
    Type type;
    std::string name;              // Unique identifier (e.g., "cylinder_01")
    std::string source;            // File path for STL, lambda string for implicit
    int32_t material_id;           // Maps to surface_material_id field
    
    // Transformation parameters (applied before voxelization)
    struct Transform {
        float tx = 0.0f, ty = 0.0f, tz = 0.0f;          // Translation
        float sx = 1.0f, sy = 1.0f, sz = 1.0f;          // Scale (non-uniform allowed)
        float rx = 0.0f, ry = 0.0f, rz = 0.0f;          // Rotation in degrees
    } transform;
    
    // Primitive parameters (for BOX, SPHERE, CYLINDER)
    struct Params {
        float radius = 1.0f;
        float height = 1.0f;
        float width = 1.0f;
        float length = 1.0f;
    } params;
    
    // Optional refinement control
    bool trigger_refinement = false;       // If true, cells near surface get refine_flag=1
    float refinement_threshold = 0.0f;     // Distance threshold for refinement trigger
    
    // Validation
    void validate() const {
        if (name.empty()) {
            throw std::invalid_argument("Geometry name cannot be empty");
        }
        if (material_id <= 0) {
            throw std::invalid_argument("Material ID must be positive integer");
        }
        if (transform.sx <= 0 || transform.sy <= 0 || transform.sz <= 0) {
            throw std::invalid_argument("Scale factors must be positive");
        }
        if (trigger_refinement && refinement_threshold <= 0) {
            throw std::invalid_argument("Refinement threshold must be positive");
        }
        if (type == Type::SPHERE && params.radius <= 0) {
            throw std::invalid_argument("Sphere radius must be positive");
        }
    }
    
    // Axis-Aligned Bounding Box
    struct AABB {
        float min_x, min_y, min_z;
        float max_x, max_y, max_z;
        
        bool contains(float x, float y, float z) const {
            return x >= min_x && x <= max_x &&
                   y >= min_y && y <= max_y &&
                   z >= min_z && z <= max_z;
        }
        
        void expand(float padding) {
            min_x -= padding; min_y -= padding; min_z -= padding;
            max_x += padding; max_y += padding; max_z += padding;
        }
    };
    
    /**
     * @brief Compute transformed bounding box
     * For primitives, computes exact bounds. For STL, returns large bounds (needs loading).
     */
    AABB computeTransformedAABB() const {
        AABB aabb;
        
        // Center in local space
        float lx_min = 0, lx_max = 0;
        float ly_min = 0, ly_max = 0;
        float lz_min = 0, lz_max = 0;
        
        switch (type) {
            case Type::SPHERE:
                lx_min = -params.radius; lx_max = params.radius;
                ly_min = -params.radius; ly_max = params.radius;
                lz_min = -params.radius; lz_max = params.radius;
                break;
            case Type::BOX:
                lx_min = -params.width/2; lx_max = params.width/2;
                ly_min = -params.height/2; ly_max = params.height/2;
                lz_min = -params.length/2; lz_max = params.length/2;
                break;
            case Type::CYLINDER:
                lx_min = -params.radius; lx_max = params.radius;
                ly_min = -params.height/2; ly_max = params.height/2;
                lz_min = -params.radius; lz_max = params.radius;
                break;
            default:
                // For STL/Implicit, use very large bounds or rely on user input
                // Here we default to a unit cube centered at origin
                lx_min = -0.5f; lx_max = 0.5f;
                ly_min = -0.5f; ly_max = 0.5f;
                lz_min = -0.5f; lz_max = 0.5f;
                break;
        }
        
        // Apply scale
        lx_min *= transform.sx; lx_max *= transform.sx;
        ly_min *= transform.sy; ly_max *= transform.sy;
        lz_min *= transform.sz; lz_max *= transform.sz;
        
        // Apply translation (rotation ignored for AABB approximation)
        aabb.min_x = lx_min + transform.tx;
        aabb.max_x = lx_max + transform.tx;
        aabb.min_y = ly_min + transform.ty;
        aabb.max_y = ly_max + transform.ty;
        aabb.min_z = lz_min + transform.tz;
        aabb.max_z = lz_max + transform.tz;
        
        return aabb;
    }
};

} // namespace geometry
} // namespace fluidloom
