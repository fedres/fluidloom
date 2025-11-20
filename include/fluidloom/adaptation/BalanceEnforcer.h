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
 * @brief Enforces 2:1 balance constraint on the mesh.
 * 
 * Ensures that no two adjacent cells differ by more than 1 refinement level.
 * Uses an iterative approach to propagate refinement flags (cascading).
 */
class BalanceEnforcer {
public:
    /**
     * @brief Initialize the balance enforcer
     * @param context OpenCL context
     * @param queue OpenCL command queue
     * @param config Adaptation configuration
     */
    BalanceEnforcer(cl_context context, cl_command_queue queue, const AdaptationConfig& config);
    
    ~BalanceEnforcer();

    /**
     * @brief Enforce balance by updating refinement flags
     * 
     * Iteratively detects violations and marks neighbors for refinement until convergence
     * or max iterations reached. Does NOT split cells, only updates flags.
     * 
     * @param coord_x X coordinates buffer
     * @param coord_y Y coordinates buffer
     * @param coord_z Z coordinates buffer
     * @param levels Refinement levels buffer
     * @param cell_states Cell states buffer
     * @param refine_flags Refinement flags buffer (input/output)
     * @param num_cells Number of cells
     * @return BalanceResult with convergence stats
     */
    BalanceResult enforce(
        cl_mem coord_x, cl_mem coord_y, cl_mem coord_z,
        cl_mem levels, cl_mem cell_states,
        cl_mem refine_flags,
        size_t num_cells
    );

private:
    cl_context m_context;
    cl_command_queue m_queue;
    AdaptationConfig m_config;
    cl_program m_program;
    
    // Kernels
    cl_kernel m_kernel_detect_violations;
    cl_kernel m_kernel_mark_cascading;
    cl_kernel m_kernel_update_shadow_levels;
    
    // Internal helpers
    void compileKernels();
    void releaseResources();
    std::string loadKernelSource(const std::string& filename);
    
    // Hash table for neighbor lookup
    cl_mem m_hash_table;
    size_t m_hash_table_size;
    void buildHashTable(cl_mem x, cl_mem y, cl_mem z, size_t num_cells);
};

} // namespace adaptation
} // namespace fluidloom
