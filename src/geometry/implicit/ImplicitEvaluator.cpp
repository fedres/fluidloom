#include "fluidloom/geometry/ImplicitEvaluator.h"
#include "fluidloom/common/Logger.h"
#include <cmath>

namespace fluidloom {
namespace geometry {

void ImplicitEvaluator::evaluate(
    const GeometryDescriptor& geom,
    const std::vector<CellCoord>& candidates,
    std::vector<bool>& is_inside
) {
    is_inside.resize(candidates.size());
    
    // TODO: Parallelize with OpenMP or OpenCL
    for (size_t i = 0; i < candidates.size(); ++i) {
        const auto& cell = candidates[i];
        // Use cell center for evaluation
        float x = static_cast<float>(cell.x) + 0.5f;
        float y = static_cast<float>(cell.y) + 0.5f;
        float z = static_cast<float>(cell.z) + 0.5f;
        
        is_inside[i] = isInside(geom, x, y, z);
    }
}

bool ImplicitEvaluator::isInside(const GeometryDescriptor& geom, float x, float y, float z) const {
    // Transform point to local space
    float lx, ly, lz;
    transformPoint(geom, x, y, z, lx, ly, lz);
    
    switch (geom.type) {
        case GeometryDescriptor::Type::SPHERE:
            return evaluateSphere(geom, lx, ly, lz);
        case GeometryDescriptor::Type::BOX:
            return evaluateBox(geom, lx, ly, lz);
        case GeometryDescriptor::Type::CYLINDER:
            return evaluateCylinder(geom, lx, ly, lz);
        case GeometryDescriptor::Type::IMPLICIT_FUNC:
            return evaluateExpression(geom, lx, ly, lz);
        default:
            return false;
    }
}

void ImplicitEvaluator::transformPoint(
    const GeometryDescriptor& geom,
    float x, float y, float z,
    float& lx, float& ly, float& lz
) const {
    // 1. Translate
    float tx = x - geom.transform.tx;
    float ty = y - geom.transform.ty;
    float tz = z - geom.transform.tz;
    
    // 2. Rotate (inverse rotation)
    // Simplified: assuming no rotation for now or axis-aligned
    // TODO: Implement full 3D rotation matrix
    
    // 3. Scale (inverse scale)
    lx = tx / geom.transform.sx;
    ly = ty / geom.transform.sy;
    lz = tz / geom.transform.sz;
}

bool ImplicitEvaluator::evaluateSphere(const GeometryDescriptor& geom, float x, float y, float z) const {
    // x^2 + y^2 + z^2 < r^2
    float r = geom.params.radius;
    return (x*x + y*y + z*z) <= (r*r);
}

bool ImplicitEvaluator::evaluateBox(const GeometryDescriptor& geom, float x, float y, float z) const {
    // |x| < w/2 && |y| < h/2 && |z| < l/2
    float half_w = geom.params.width * 0.5f;
    float half_h = geom.params.height * 0.5f;
    float half_l = geom.params.length * 0.5f;
    
    return (std::abs(x) <= half_w) &&
           (std::abs(y) <= half_h) &&
           (std::abs(z) <= half_l);
}

bool ImplicitEvaluator::evaluateCylinder(const GeometryDescriptor& geom, float x, float y, float z) const {
    // x^2 + z^2 < r^2 && |y| < h/2 (assuming Y-axis aligned)
    float r = geom.params.radius;
    float half_h = geom.params.height * 0.5f;
    
    bool inside_radius = (x*x + z*z) <= (r*r);
    bool inside_height = std::abs(y) <= half_h;
    
    return inside_radius && inside_height;
}

bool ImplicitEvaluator::evaluateExpression(const GeometryDescriptor& geom, float x, float y, float z) const {
    (void)geom;
    (void)x;
    (void)y;
    (void)z;
    // Stub for general expression evaluation
    // In a full implementation, this would parse geom.source and evaluate it
    return false;
}

} // namespace geometry
} // namespace fluidloom
