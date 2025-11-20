// @stable - Module 7 API frozen
#pragma once

#include <cstdint>
#include <vector>
#include <string>

namespace fluidloom {
namespace halo {

/**
 * @brief Describes memory layout of pack buffers for OpenCL kernels
 * 
 * Layout is **Structure of Arrays (SoA)** (Field-major, Component-major) for optimal coalescing:
 * [Field0_Comp0_AllCells, Field0_Comp1_AllCells, ..., Field1_Comp0_AllCells, ...]
 * 
 * Each component array is contiguous. Stride is determined by buffer capacity (max_cells).
 */
struct PackBufferLayout {
    // Overall buffer parameters
    size_t capacity_bytes{0};        // Total allocated size per buffer (double buffered)
    size_t used_bytes{0};            // Current usage (sum of all range.pack_size_bytes)
    
    // Field metadata
    struct FieldLayout {
        std::string field_name;
        size_t num_components{0};
        size_t bytes_per_component{0};
        size_t offset_in_cell{0};    // Byte offset within a cell's data chunk
    };
    std::vector<FieldLayout> fields;
    
    // Per-cell size
    size_t cell_size_bytes{0};       // Sum of all components for one cell
    
    // Methods
    PackBufferLayout() = default;
    
    // Calculate offset for a specific field, component, and cell index
    size_t getOffset(size_t field_idx, size_t component_idx, size_t cell_idx) const {
        if (field_idx >= fields.size()) return 0;
        const FieldLayout& field = fields[field_idx];
        
        // SoA Layout:
        // [Field0_Comp0_AllCells, Field0_Comp1_AllCells, ..., Field1_...]
        // Stride is max_cells (derived from capacity)
        size_t max_cells = (cell_size_bytes > 0) ? (capacity_bytes / cell_size_bytes) : 0;
        if (max_cells == 0) return 0;
        
        // Actually, let's use the stored 'offset_in_cell' which holds "bytes before this field in a cell".
        // In SoA, the start of this field is: offset_in_cell * max_cells.
        size_t global_field_start = field.offset_in_cell * max_cells;
        
        // Component offset: component_idx * bytes_per_component * max_cells
        size_t component_offset = component_idx * field.bytes_per_component * max_cells;
        
        // Cell offset: cell_idx * bytes_per_component
        size_t cell_offset = cell_idx * field.bytes_per_component;
        
        return global_field_start + component_offset + cell_offset;
    }
    
    // Validate layout
    bool validate() const {
        if (capacity_bytes == 0 || used_bytes > capacity_bytes) return false;
        if (cell_size_bytes == 0) return false;
        
        // Check alignment (optional for SoA but good practice)
        for (const auto& field : fields) {
            if (field.bytes_per_component % 4 != 0) return false; // Components should be aligned
            if (field.offset_in_cell % 4 != 0) return false; // Field start should be aligned
        }
        return true;
    }
    
    // Add a field to the layout
    void addField(const std::string& name, size_t num_comp, size_t bytes_per_comp) {
        FieldLayout fl;
        fl.field_name = name;
        fl.num_components = num_comp;
        fl.bytes_per_component = bytes_per_comp;
        fl.offset_in_cell = cell_size_bytes; // Store cumulative size so far
        
        fields.push_back(fl);
        cell_size_bytes += num_comp * bytes_per_comp;
    }
};

} // namespace halo
} // namespace fluidloom
