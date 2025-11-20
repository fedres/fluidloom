#include <gtest/gtest.h>
#include "fluidloom/halo/PackBufferLayout.h"

using namespace fluidloom::halo;

TEST(PackBufferLayoutTest, LayoutCalculation) {
    PackBufferLayout layout;
    layout.capacity_bytes = 1024;
    
    // Add scalar field (4 bytes)
    layout.addField("density", 1, 4);
    
    // Add vector field (12 bytes)
    layout.addField("velocity", 3, 4);
    
    EXPECT_EQ(layout.cell_size_bytes, 16);
    EXPECT_EQ(layout.fields.size(), 2);
    
    // Capacity 1024, Cell Size 16 -> Max Cells = 64
    size_t max_cells = 64;
    
    // Check offsets
    // Field 0 (density): offset 0
    EXPECT_EQ(layout.getOffset(0, 0, 0), 0);
    
    // Field 1 (velocity): 
    // Starts after Field 0 (1 component * 4 bytes * 64 cells = 256 bytes)
    size_t f0_size = 1 * 4 * max_cells;
    EXPECT_EQ(layout.getOffset(1, 0, 0), f0_size);
    
    // Field 1, Component 1:
    // Starts after F1_C0 (1 component * 4 bytes * 64 cells = 256 bytes)
    // So 256 + 256 = 512
    size_t f1_c0_size = 1 * 4 * max_cells;
    EXPECT_EQ(layout.getOffset(1, 1, 0), f0_size + f1_c0_size);
    
    // Field 1, Component 2:
    // 512 + 256 = 768
    EXPECT_EQ(layout.getOffset(1, 2, 0), 768);
    
    // Cell 1
    // Density (F0, C0): 0 + 1 * 4 = 4
    EXPECT_EQ(layout.getOffset(0, 0, 1), 4);
    
    // Velocity (F1, C0): 256 + 1 * 4 = 260
    EXPECT_EQ(layout.getOffset(1, 0, 1), 260);
}

TEST(PackBufferLayoutTest, Validation) {
    PackBufferLayout layout;
    layout.capacity_bytes = 100;
    layout.cell_size_bytes = 4;
    layout.used_bytes = 50;
    
    PackBufferLayout::FieldLayout fl;
    fl.offset_in_cell = 0;
    layout.fields.push_back(fl);
    
    EXPECT_TRUE(layout.validate());
    
    // Invalid alignment
    layout.fields[0].offset_in_cell = 1;
    EXPECT_FALSE(layout.validate());
}
