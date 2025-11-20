#pragma once

#include "fluidloom/adaptation/AdaptationTypes.h"
#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif
#include <vector>
#include <string>

namespace fluidloom {
namespace adaptation {

/**
 * @brief Orchestrates the cell splitting process for AMR.
 * 
 * Responsibilities:
 * 1. Identify cells marked for refinement
 * 2. Calculate memory requirements for children (prefix sum)
 * 3. Allocate and generate child cells
 * 4. Interpolate fields from parents to children
 */
class SplitEngine {
public:
    /**
     * @brief Initialize the split engine
     * @param context OpenCL context
     * @param queue OpenCL command queue
     * @param config Adaptation configuration
     */
    SplitEngine(cl_context context, cl_command_queue queue, const AdaptationConfig& config);
    
    ~SplitEngine();

    /**
     * @brief Execute the split operation
     * 
     * @param parent_x Parent X coordinates buffer
     * @param parent_y Parent Y coordinates buffer
     * @param parent_z Parent Z coordinates buffer
     * @param parent_level Parent refinement levels buffer
     * @param parent_states Parent cell states buffer (FLUID, BOUNDARY, etc.)
     * @param refine_flags Refinement flags buffer (>0 means split)
     * @param parent_material_id Parent material IDs buffer
     * @param num_parents Number of parent cells
     * @param parent_fields Optional: Parent field data for interpolation
     * @param num_field_components Number of components per field (e.g. 1 for scalar, 3 for vector)
     * @return SplitResult containing new children and mapping
     */
    SplitResult split(
        cl_mem parent_x, cl_mem parent_y, cl_mem parent_z,
        cl_mem parent_level, cl_mem parent_states,
        cl_mem refine_flags,
        cl_mem parent_material_id,
        size_t num_parents,
        cl_mem parent_fields = nullptr,
        uint32_t num_field_components = 0
    );

private:
    cl_context m_context;
    cl_command_queue m_queue;
    AdaptationConfig m_config;
    cl_program m_program;
    
    // Kernels
    cl_kernel m_kernel_count_allocate;
    cl_kernel m_kernel_generate_children;
    cl_kernel m_kernel_interpolate;
    
    // Internal helpers
    void compileKernels();
    void releaseResources();
    
    // Helper to load kernel source
    std::string loadKernelSource(const std::string& filename);
};

} // namespace adaptation
} // namespace fluidloom
