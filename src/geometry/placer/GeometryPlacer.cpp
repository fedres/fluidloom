#include "fluidloom/geometry/GeometryPlacer.h"
#include "fluidloom/common/Logger.h"
#include "fluidloom/core/hilbert/HilbertCodec.h"
#include <algorithm>
#include <set>

namespace fluidloom {
namespace geometry {

GeometryPlacer::GeometryPlacer() {
    FL_LOG(INFO) << "GeometryPlacer initialized";
}

void GeometryPlacer::placeGeometry(
    const std::vector<GeometryDescriptor>& geometries,
    std::vector<CellCoord>& fluid_cells,
    float cell_size,
    const GeometryDescriptor::AABB& domain_bbox
) {
    if (geometries.empty()) return;
    
    FL_LOG(INFO) << "Placing " << geometries.size() << " geometry objects...";
    
    std::vector<VoxelizedCell> all_geometry_cells;
    
    // 1. Process all geometries
    for (const auto& geom : geometries) {
        auto cells = processGeometry(geom, cell_size, domain_bbox);
        all_geometry_cells.insert(all_geometry_cells.end(), cells.begin(), cells.end());
    }
    
    if (all_geometry_cells.empty()) {
        FL_LOG(INFO) << "No geometry cells generated.";
        return;
    }
    
    FL_LOG(INFO) << "Generated " << all_geometry_cells.size() << " solid cells from geometry.";
    
    // Add to fluid_cells list (converting VoxelizedCell to CellCoord)
    for (const auto& vcell : all_geometry_cells) {
        fluid_cells.push_back(CellCoord{vcell.x, vcell.y, vcell.z, 0});
    }
    
    // Deduplicate
    std::sort(fluid_cells.begin(), fluid_cells.end(), [](const CellCoord& a, const CellCoord& b) {
        if (a.z != b.z) return a.z < b.z;
        if (a.y != b.y) return a.y < b.y;
        return a.x < b.x;
    });
    
    fluid_cells.erase(
        std::unique(fluid_cells.begin(), fluid_cells.end(), [](const CellCoord& a, const CellCoord& b) {
            return a.x == b.x && a.y == b.y && a.z == b.z;
        }),
        fluid_cells.end()
    );
    
    FL_LOG(INFO) << "Total cells after merging geometry: " << fluid_cells.size();
}

std::vector<VoxelizedCell> GeometryPlacer::processGeometry(
    const GeometryDescriptor& geom,
    float cell_size,
    const GeometryDescriptor::AABB& domain_bbox
) {
    std::vector<VoxelizedCell> result;
    std::vector<CellCoord> raw_cells;
    
    if (geom.type == GeometryDescriptor::Type::STL_MESH) {
        // Voxelize STL
        raw_cells = m_stl_voxelizer.voxelize(geom, cell_size);
    } else {
        // Implicit evaluation
        // 1. Compute bounding box intersection with domain
        auto geom_bbox = geom.computeTransformedAABB();
        
        // Clip to domain
        geom_bbox.min_x = std::max(geom_bbox.min_x, domain_bbox.min_x);
        geom_bbox.min_y = std::max(geom_bbox.min_y, domain_bbox.min_y);
        geom_bbox.min_z = std::max(geom_bbox.min_z, domain_bbox.min_z);
        geom_bbox.max_x = std::min(geom_bbox.max_x, domain_bbox.max_x);
        geom_bbox.max_y = std::min(geom_bbox.max_y, domain_bbox.max_y);
        geom_bbox.max_z = std::min(geom_bbox.max_z, domain_bbox.max_z);
        
        // 2. Generate candidates
        auto candidates = generateCandidates(geom_bbox, cell_size);
        
        // 3. Evaluate
        std::vector<bool> is_inside;
        m_implicit_evaluator.evaluate(geom, candidates, is_inside);
        
        // 4. Filter
        for (size_t i = 0; i < candidates.size(); ++i) {
            if (is_inside[i]) {
                raw_cells.push_back(candidates[i]);
            }
        }
    }
    
    // Convert to VoxelizedCell and assign material
    result.reserve(raw_cells.size());
    for (const auto& cell : raw_cells) {
        // Compute Hilbert index
        uint64_t hilbert = fluidloom::hilbert::encode(cell.x, cell.y, cell.z);
        result.emplace_back(cell.x, cell.y, cell.z, geom.material_id, hilbert);
    }
    
    FL_LOG(INFO) << "Geometry '" << geom.name << "' produced " << result.size() << " cells";
    return result;
}

std::vector<CellCoord> GeometryPlacer::generateCandidates(
    const GeometryDescriptor::AABB& bbox,
    float cell_size
) {
    std::vector<CellCoord> candidates;
    
    int start_x = static_cast<int>(std::floor(bbox.min_x / cell_size));
    int end_x = static_cast<int>(std::ceil(bbox.max_x / cell_size));
    int start_y = static_cast<int>(std::floor(bbox.min_y / cell_size));
    int end_y = static_cast<int>(std::ceil(bbox.max_y / cell_size));
    int start_z = static_cast<int>(std::floor(bbox.min_z / cell_size));
    int end_z = static_cast<int>(std::ceil(bbox.max_z / cell_size));
    
    // Reserve memory (approximate)
    size_t estimated = (end_x - start_x) * (end_y - start_y) * (end_z - start_z);
    if (estimated > 0) candidates.reserve(estimated);
    
    for (int z = start_z; z < end_z; ++z) {
        for (int y = start_y; y < end_y; ++y) {
            for (int x = start_x; x < end_x; ++x) {
                candidates.push_back(CellCoord{x, y, z, 0}); // The original line was `candidates.push_back(CellCoord{x, y, z, 0});, 0});` which is a syntax error. Assuming the intent was to keep the existing level argument as 0.
            }
        }
    }
    
    return candidates;
}

} // namespace geometry
} // namespace fluidloom
