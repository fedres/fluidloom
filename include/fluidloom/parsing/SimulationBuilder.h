#pragma once

#include "fluidloom/runtime/ExecutionGraph.h"
#include "fluidloom/runtime/nodes/AdaptMeshNode.h"
#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif
#include <memory>
#include <string>

// Forward declarations for ANTLR generated classes
class FluidLoomSimulationParser;

namespace fluidloom {
namespace parsing {

/**
 * @brief Builds execution graph from simulation script
 * 
 * Uses ANTLR-generated parsers to parse .fl files
 */
class SimulationBuilder {
public:
    SimulationBuilder(cl_context context, cl_command_queue queue);
    
    /**
     * @brief Build execution graph from .fl script content
     * @param script_content Simulation script content
     * @return Execution graph with AMR node
     */
    std::unique_ptr<runtime::ExecutionGraph> buildFromScript(const std::string& script_content);
    
    /**
     * @brief Build execution graph (stub for demo)
     * @return Execution graph with AMR node
     */
    std::unique_ptr<runtime::ExecutionGraph> buildStub();
    
private:
    cl_context m_context;
    cl_command_queue m_queue;
    
    // Mesh buffers (simplified for AMR demo)
    cl_mem m_coord_x, m_coord_y, m_coord_z;
    cl_mem m_levels, m_cell_states, m_refine_flags, m_material_id;
    size_t m_num_cells;
    size_t m_capacity;
    
    // Helper: Initialize mesh buffers
    void initializeMeshBuffers();
    
    // Helper: Generate kernel code based on name and parameters
    std::string generateKernelCode(const std::string& kernel_name, const std::vector<std::string>& params);
    
    // Helper: Generate simple kernel stub for testing
    std::string generateKernelStub(const std::string& kernel_name);
};

} // namespace parsing
} // namespace fluidloom
