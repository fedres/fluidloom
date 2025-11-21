#pragma once

#include "fluidloom/geometry/GeometryDescriptor.h"
#include "fluidloom/geometry/ImplicitEvaluator.h"
#include "fluidloom/geometry/SimpleSTLVoxelizer.h"
#include "fluidloom/geometry/VoxelizedCell.h"
#include "fluidloom/core/hilbert/CellCoord.h"
#include <vector>
#include <memory>

namespace fluidloom {
namespace geometry {

/**
 * @brief Orchestrates geometry voxelization and placement
 * 
 * Handles both implicit and STL geometry, merges results,
 * and deduplicates cells by Hilbert index.
 */
class GeometryPlacer {
public:
    GeometryPlacer();
    
    /**
     * @brief Place geometry into the simulation mesh
     * 
     * @param geometries List of geometry descriptors
     * @param fluid_cells Existing fluid cells (will be modified/merged)
     * @param cell_size Size of each cell
     * @param domain_bbox Domain bounds for clipping
     */
    void placeGeometry(
        const std::vector<GeometryDescriptor>& geometries,
        std::vector<CellCoord>& fluid_cells,
        float cell_size,
        const GeometryDescriptor::AABB& domain_bbox
    );
    
private:
    ImplicitEvaluator m_implicit_evaluator;
    SimpleSTLVoxelizer m_stl_voxelizer;
    
    /**
     * @brief Process a single geometry descriptor
     */
    std::vector<VoxelizedCell> processGeometry(
        const GeometryDescriptor& geom,
        float cell_size,
        const GeometryDescriptor::AABB& domain_bbox
    );
    
    /**
     * @brief Generate candidate cells within a bounding box
     */
    std::vector<CellCoord> generateCandidates(
        const GeometryDescriptor::AABB& bbox,
        float cell_size
    );
};

} // namespace geometry
} // namespace fluidloom
