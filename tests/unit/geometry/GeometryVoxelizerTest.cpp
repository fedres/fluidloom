#include <gtest/gtest.h>
#include "fluidloom/geometry/GeometryPlacer.h"
#include "fluidloom/geometry/GeometryDescriptor.h"
#include "fluidloom/core/hilbert/CellCoord.h"

using namespace fluidloom::geometry;
using namespace fluidloom;

class GeometryVoxelizerTest : public ::testing::Test {
protected:
    GeometryPlacer placer;
    std::vector<CellCoord> fluid_cells;
    GeometryDescriptor::AABB domain_bbox = {-10.0f, -10.0f, -10.0f, 10.0f, 10.0f, 10.0f};
};

TEST_F(GeometryVoxelizerTest, VoxelizeSphere) {
    GeometryDescriptor geom;
    geom.type = GeometryDescriptor::Type::SPHERE;
    geom.name = "test_sphere";
    geom.material_id = 1;
    geom.params.radius = 2.5f; // Should cover cells within distance 2.5
    
    std::vector<GeometryDescriptor> geometries = {geom};
    placer.placeGeometry(geometries, fluid_cells, 1.0f, domain_bbox);
    
    // Check that we have cells
    ASSERT_FALSE(fluid_cells.empty());
    
    // Check a known inside cell (0,0,0)
    bool center_found = false;
    for (const auto& cell : fluid_cells) {
        if (cell.x == 0 && cell.y == 0 && cell.z == 0) {
            center_found = true;
            break;
        }
    }
    EXPECT_TRUE(center_found);
    
    // Check a known outside cell (3,0,0) -> center at 3.5, dist 3.5 > 2.5
    bool outside_found = false;
    for (const auto& cell : fluid_cells) {
        if (cell.x == 3 && cell.y == 0 && cell.z == 0) {
            outside_found = true;
            break;
        }
    }
    EXPECT_FALSE(outside_found);
}

TEST_F(GeometryVoxelizerTest, VoxelizeBox) {
    GeometryDescriptor geom;
    geom.type = GeometryDescriptor::Type::BOX;
    geom.name = "test_box";
    geom.material_id = 2;
    geom.params.width = 2.0f;
    geom.params.height = 2.0f;
    geom.params.length = 2.0f;
    // Box from -1 to 1 in all axes
    
    std::vector<GeometryDescriptor> geometries = {geom};
    fluid_cells.clear();
    placer.placeGeometry(geometries, fluid_cells, 1.0f, domain_bbox);
    
    // Should contain (0,0,0) which is center 0.5,0.5,0.5 -> inside -1..1
    // Wait, box is centered at origin.
    // Cell (0,0,0) center is (0.5, 0.5, 0.5).
    // Box extent: [-1, 1]. 0.5 is inside.
    
    bool center_found = false;
    for (const auto& cell : fluid_cells) {
        if (cell.x == 0 && cell.y == 0 && cell.z == 0) {
            center_found = true;
            break;
        }
    }
    EXPECT_TRUE(center_found);
}
