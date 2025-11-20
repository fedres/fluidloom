#pragma once

#include <vector>
#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif
#include <unordered_set>
#include <memory>
#include <stdexcept>
#include <string>
#include "fluidloom/adaptation/CellDescriptor.h"

namespace fluidloom {
namespace adaptation {

// Forward declarations


/**
 * @brief Result structure for cell splitting operation
 * 
 * Contains all metadata needed for subsequent rebuild and field interpolation.
 * All vectors are pre-allocated to avoid resize overhead.
 */
struct SplitResult {
    bool success = false;
    size_t num_children = 0;
    size_t num_parents_split = 0;
    
    // Child cell descriptors (8x split_count)
    std::vector<CellDescriptor> children;
    
    // Mapping: parent index in old cell list → child block start index
    // Size: old_cell_count, value = INVALID_INDEX if parent not split
    std::vector<uint32_t> parent_to_child_map;
    
    // Indices of parents that were actually split (for validation)
    std::vector<uint32_t> split_parent_indices;
    
    // OpenCL event for synchronization
    cl_event event = nullptr;
    
    // Memory usage statistics
    size_t device_memory_used = 0;  // Bytes allocated on device
    
    ~SplitResult() {
        if (event) clReleaseEvent(event);
    }
};

/**
 * @brief Result structure for cell merging operation
 * 
 * Contains parent cells created by merging siblings and metadata for
 * field averaging and index mapping.
 */
struct MergeResult {
    bool success = false;
    size_t num_parents_created = 0;
    size_t num_children_merged = 0;
    
    // New parent cell descriptors
    std::vector<CellDescriptor> parents;
    
    // Averaged fields for new parents (num_parents * num_components)
    std::vector<float> averaged_fields;
    
    // Set of child indices that were merged (for exclusion from new cell list)
    std::unordered_set<size_t> merged_child_indices;
    
    // Mapping: merge group ID → parent index in new cell list
    std::vector<uint32_t> group_to_parent_map;
    
    // OpenCL event for synchronization
    cl_event event = nullptr;
    
    // Field averaging statistics (for conservation validation)
    double mass_before_merge = 0.0;
    double mass_after_merge = 0.0;
    double conservation_error = 0.0;
    
    ~MergeResult() {
        if (event) clReleaseEvent(event);
    }
};

/**
 * @brief Result structure for balance enforcement
 * 
 * Tracks convergence, iterations, and final state for validation.
 */
struct BalanceResult {
    bool converged = false;
    uint32_t iterations = 0;
    uint32_t total_violations_detected = 0;
    uint32_t total_cells_marked_for_balance = 0;
    size_t final_cell_count = 0;
    double max_level_difference = 0.0;
    
    // Per-iteration statistics for profiling
    struct IterationStats {
        uint32_t violations_this_iter = 0;
        uint32_t cells_marked_this_iter = 0;
        double time_ms = 0.0;
    };
    std::vector<IterationStats> per_iteration_stats;
};

/**
 * @brief Global adaptation configuration
 * 
 * Centralized configuration for all adaptation behavior. Loaded from
 * config.json and validated at engine startup.
 */
struct AdaptationConfig {
    // Level constraints
    uint8_t max_refinement_level = 8;
    uint8_t min_refinement_level = 0;
    
    // Performance tuning
    size_t max_cells_per_split_batch = 65536;  // For kernel workgroup sizing
    size_t max_cells_per_merge_batch = 32768;
    uint32_t max_balance_iterations = 10;
    
    // Balance enforcement
    bool enforce_2_1_balance = true;
    bool cascade_refinement = true;
    uint8_t cascade_depth = 2;  // How many layers to cascade
    
    // Geometry lock
    bool allow_modifying_geometry_moving = false;  // If true, allows split/merge of moving geometry cells
    
    // Field averaging rules (overrides per-field rules)
    std::string default_averaging_rule = "arithmetic";
    
    // Profiling and validation
    bool validate_conservation = true;
    double conservation_tolerance = 0.001;  // 0.1%
    
    // Memory allocation strategy
    double buffer_growth_factor = 1.5;  // When resizing buffers
    size_t initial_buffer_capacity = 1048576;  // 1M cells
    
    void validate() const {
        if (max_refinement_level > 8) {
            throw std::invalid_argument("max_refinement_level cannot exceed 8");
        }
        if (min_refinement_level > max_refinement_level) {
            throw std::invalid_argument("min_refinement_level must be <= max_refinement_level");
        }
        if (cascade_depth > max_refinement_level) {
            throw std::invalid_argument("cascade_depth cannot exceed max_refinement_level");
        }
    }
};

} // namespace adaptation
} // namespace fluidloom
