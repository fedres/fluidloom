#pragma once

#include "fluidloom/geometry/GeometryDescriptor.h"
#include "fluidloom/core/hilbert/CellCoord.h"
#include <vector>
#include <cmath>

namespace fluidloom {
namespace geometry {

/**
 * @brief Evaluates implicit functions to determine if points are inside geometry
 * 
 * Supports primitive shapes (Sphere, Box, Cylinder) and general mathematical expressions.
 */
class ImplicitEvaluator {
public:
    ImplicitEvaluator() = default;
    
    /**
     * @brief Evaluate geometry for a batch of candidate cells
     * 
     * @param geom Geometry descriptor defining the shape
     * @param candidates List of cells to test
     * @param is_inside Output vector (true if cell center is inside geometry)
     */
    void evaluate(
        const GeometryDescriptor& geom,
        const std::vector<CellCoord>& candidates,
        std::vector<bool>& is_inside
    );
    
    /**
     * @brief Check if a single point is inside the geometry
     * 
     * @param geom Geometry descriptor
     * @param x, y, z Point coordinates
     * @return true if point is inside
     */
    bool isInside(const GeometryDescriptor& geom, float x, float y, float z) const;

private:
    // Primitive evaluators
    bool evaluateSphere(const GeometryDescriptor& geom, float x, float y, float z) const;
    bool evaluateBox(const GeometryDescriptor& geom, float x, float y, float z) const;
    bool evaluateCylinder(const GeometryDescriptor& geom, float x, float y, float z) const;
    
    // General expression evaluator (stubbed for now)
    bool evaluateExpression(const GeometryDescriptor& geom, float x, float y, float z) const;
    
    // Helper to transform point to local space
    void transformPoint(
        const GeometryDescriptor& geom,
        float x, float y, float z,
        float& lx, float& ly, float& lz
    ) const;
};

} // namespace geometry
} // namespace fluidloom
