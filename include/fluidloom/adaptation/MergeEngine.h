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
 * @brief Orchestrates the cell merging process for AMR.
 * 
 * Responsibilities:
 * 1. Identify sibling groups eligible for merging
 * 2. Verify conservation and validity
 * 3. Average fields from children to parents
 * 4. Create parent cells
 */
class MergeEngine {
public:
    /**
     * @brief Initialize the merge engine
     * @param context OpenCL context
     * @param queue OpenCL command queue
     * @param config Adaptation configuration
     */
    MergeEngine(cl_context context, cl_command_queue queue, const AdaptationConfig& config);
    
    ~MergeEngine();

    /**
     * @brief Execute the merge operation
     * 
     * @param child_x Child X coordinates buffer
     * @param child_y Child Y coordinates buffer
     * @param child_z Child Z coordinates buffer
     * @param child_level Child refinement levels buffer
     * @param child_states Child cell states buffer
     * @param refine_flags Refinement flags buffer (-1 means merge)
     * @param child_material_id Child material IDs buffer
     * @param num_children Number of child cells
     * @param child_fields Optional: Child field data for averaging
     * @param num_field_components Number of components per field
     * @return MergeResult containing new parents and mapping
     */
    MergeResult merge(
        cl_mem child_x, cl_mem child_y, cl_mem child_z,
        cl_mem child_level, cl_mem child_states,
        cl_mem refine_flags,
        cl_mem child_material_id,
        size_t num_children,
        cl_mem child_fields = nullptr,
        uint32_t num_field_components = 0
    );

private:
    cl_context m_context;
    cl_command_queue m_queue;
    AdaptationConfig m_config;
    cl_program m_program;
    
    // Kernels
    cl_kernel m_kernel_mark_siblings;
    cl_kernel m_kernel_merge_fields;
    cl_kernel m_kernel_create_parents;
    
    // Internal helpers
    void compileKernels();
    void releaseResources();
    std::string loadKernelSource(const std::string& filename);
    
    // Hash table management
    cl_mem m_hash_table;
    size_t m_hash_table_size;
    void buildHashTable(cl_mem x, cl_mem y, cl_mem z, size_t num_cells);
};

} // namespace adaptation
} // namespace fluidloom
