#pragma once

#include "fluidloom/runtime/ExecutionGraph.h"
#include "fluidloom/runtime/nodes/AdaptMeshNode.h"
#include "fluidloom/core/fields/SOAFieldManager.h"
#include "fluidloom/core/fields/FieldDescriptor.h"
#include "fluidloom/geometry/GeometryPlacer.h"
#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif
#include <memory>
#include <string>
#include <unordered_map>

// Forward declarations for ANTLR generated classes
class FluidLoomSimulationParser;

namespace fluidloom {
namespace parsing {

// Forward declare OpenCLBackend
class OpenCLBackend;

/**
 * @brief Builds execution graph from simulation script
 * 
 * Uses ANTLR-generated parsers to parse .fl files
 */
class SimulationBuilder {
public:
    SimulationBuilder(cl_context context, cl_command_queue queue);
    ~SimulationBuilder();
    
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
    
    // Field management
    std::unique_ptr<OpenCLBackend> m_backend;
    std::unique_ptr<fields::SOAFieldManager> m_field_manager;
    std::unordered_map<std::string, fields::FieldHandle> m_field_handles;
    
    // Mesh buffers (simplified for AMR demo)
    cl_mem m_coord_x, m_coord_y, m_coord_z;
    cl_mem m_levels, m_cell_states, m_refine_flags, m_material_id;
    size_t m_num_cells;
    size_t m_capacity;
    
    // Helper: Initialize mesh buffers
    void initializeMeshBuffers();
    
    // Helper: Parse field declarations and register them
    void parseFieldDeclarations(FluidLoomSimulationParser* parser);
    
    // Helper: Allocate buffers for all registered fields
    void allocateFieldBuffers();
    
    // Helper: Generate kernel code based on name and parameters
    std::string generateKernelCode(const std::string& kernel_name, const std::vector<std::string>& params);
    
    // Helper: Generate simple kernel stub for testing
    std::string generateKernelStub(const std::string& kernel_name);
    
    // Geometry placement
    std::unique_ptr<geometry::GeometryPlacer> m_geometry_placer;
    
    // Helper: Process geometry definitions from AST
    void processGeometry(FluidLoomSimulationParser* parser);

};

} // namespace parsing
} // namespace fluidloom
