#pragma once

#include "fluidloom/adaptation/AdaptationTypes.h"
#include "fluidloom/adaptation/SplitEngine.h"
#include "fluidloom/adaptation/MergeEngine.h"
#include "fluidloom/adaptation/BalanceEnforcer.h"
#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif
#include <vector>
#include <memory>

namespace fluidloom {
namespace adaptation {

/**
 * @brief Main orchestration engine for Adaptive Mesh Refinement (AMR).
 * 
 * Coordinates the adaptation pipeline:
 * 1. Balance enforcement (cascading refinement)
 * 2. Cell splitting
 * 3. Cell merging
 * 4. Mesh rebuilding and compaction
 * 5. Field data remapping
 */
class AdaptationEngine {
public:
    /**
     * @brief Initialize the adaptation engine
     * @param context OpenCL context
     * @param queue OpenCL command queue
     * @param config Adaptation configuration
     */
    AdaptationEngine(cl_context context, cl_command_queue queue, const AdaptationConfig& config);
    
    ~AdaptationEngine();

    /**
     * @brief Execute the full adaptation cycle
     * 
     * @param coord_x X coordinates buffer (in/out)
     * @param coord_y Y coordinates buffer (in/out)
     * @param coord_z Z coordinates buffer (in/out)
     * @param levels Refinement levels buffer (in/out)
     * @param cell_states Cell states buffer (in/out)
     * @param refine_flags Refinement flags buffer (in/out)
     * @param material_id Material IDs buffer (in/out)
     * @param num_cells Pointer to number of cells (in/out)
     * @param capacity Current buffer capacity (in/out)
     * @param fields Optional: Field data buffers to remap
     * @param num_field_components Number of components per field
     * @return cl_event Event signaling completion of the adaptation cycle
     */
    cl_event adapt(
        cl_mem* coord_x, cl_mem* coord_y, cl_mem* coord_z,
        cl_mem* levels, cl_mem* cell_states,
        cl_mem* refine_flags,
        cl_mem* material_id,
        size_t* num_cells,
        size_t* capacity,
        cl_mem* fields = nullptr,
        uint32_t num_field_components = 0
    );

private:
    cl_context m_context;
    cl_command_queue m_queue;
    AdaptationConfig m_config;
    
    // Sub-engines
    std::unique_ptr<SplitEngine> m_split_engine;
    std::unique_ptr<MergeEngine> m_merge_engine;
    std::unique_ptr<BalanceEnforcer> m_balance_enforcer;
    
    // Internal helpers
    void resizeBuffers(
        size_t new_capacity,
        cl_mem* coord_x, cl_mem* coord_y, cl_mem* coord_z,
        cl_mem* levels, cl_mem* cell_states,
        cl_mem* refine_flags,
        cl_mem* material_id,
        cl_mem* fields,
        uint32_t num_field_components
    );
    
    void compactAndRebuild(
        const SplitResult& split_result,
        const MergeResult& merge_result,
        cl_mem* coord_x, cl_mem* coord_y, cl_mem* coord_z,
        cl_mem* levels, cl_mem* cell_states,
        cl_mem* refine_flags,
        cl_mem* material_id,
        size_t* num_cells,
        size_t* capacity,
        cl_mem* fields,
        uint32_t num_field_components
    );

    // GPU Compaction
    void compactAndRebuildGPU(
        const SplitResult& split_res,
        const MergeResult& merge_res,
        cl_mem* coord_x, cl_mem* coord_y, cl_mem* coord_z,
        cl_mem* levels, cl_mem* cell_states,
        cl_mem* refine_flags,
        cl_mem* material_id,
        size_t* num_cells,
        size_t* capacity,
        cl_mem* fields,
        uint32_t num_field_components
    );

    // Compaction Kernels
    cl_program m_compaction_program;
    cl_kernel m_kernel_mark_valid;
    cl_kernel m_kernel_scan_local;
    cl_kernel m_kernel_scan_add_sums;
    cl_kernel m_kernel_compact;
    cl_kernel m_kernel_append;
    
    void compileCompactionKernels();
    std::string loadKernelSource(const std::string& filename);
    
    // Helper for exclusive scan
    void exclusiveScan(cl_mem input, cl_mem output, uint32_t num_elements, uint32_t* total_sum);
};

} // namespace adaptation
} // namespace fluidloom
