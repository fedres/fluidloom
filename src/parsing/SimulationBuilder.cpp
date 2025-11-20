#include "fluidloom/parsing/SimulationBuilder.h"
#include "fluidloom/common/Logger.h"
#include "fluidloom/adaptation/AdaptationEngine.h"
#include "fluidloom/adaptation/AdaptationTypes.h"
#include "fluidloom/runtime/nodes/KernelNode.h"
#include "fluidloom/core/registry/FieldRegistry.h"
#include "fluidloom/core/fields/FieldDescriptor.h"

// ANTLR includes
#include "antlr4-runtime.h"
#include "FluidLoomSimulationLexer.h"
#include "FluidLoomSimulationParser.h"
#include "FluidLoomKernelParser.h"

#include <sstream>

namespace fluidloom {
namespace parsing {

SimulationBuilder::SimulationBuilder(cl_context context, cl_command_queue queue)
    : m_context(context), m_queue(queue),
      m_coord_x(nullptr), m_coord_y(nullptr), m_coord_z(nullptr),
      m_levels(nullptr), m_cell_states(nullptr), m_refine_flags(nullptr), m_material_id(nullptr),
      m_num_cells(0), m_capacity(0) {
    
    initializeMeshBuffers();
}

void SimulationBuilder::initializeMeshBuffers() {
    // Initialize with a simple 8x8x8 base grid (512 cells)
    m_num_cells = 512;
    m_capacity = 1024;
    
    cl_int err;
    m_coord_x = clCreateBuffer(m_context, CL_MEM_READ_WRITE, m_capacity * sizeof(int), nullptr, &err);
    m_coord_y = clCreateBuffer(m_context, CL_MEM_READ_WRITE, m_capacity * sizeof(int), nullptr, &err);
    m_coord_z = clCreateBuffer(m_context, CL_MEM_READ_WRITE, m_capacity * sizeof(int), nullptr, &err);
    m_levels = clCreateBuffer(m_context, CL_MEM_READ_WRITE, m_capacity * sizeof(uint8_t), nullptr, &err);
    m_cell_states = clCreateBuffer(m_context, CL_MEM_READ_WRITE, m_capacity * sizeof(uint8_t), nullptr, &err);
    m_refine_flags = clCreateBuffer(m_context, CL_MEM_READ_WRITE, m_capacity * sizeof(int), nullptr, &err);
    m_material_id = clCreateBuffer(m_context, CL_MEM_READ_WRITE, m_capacity * sizeof(uint32_t), nullptr, &err);
    
    // Initialize on host
    std::vector<int> h_x(m_num_cells), h_y(m_num_cells), h_z(m_num_cells);
    std::vector<uint8_t> h_levels(m_num_cells, 0);
    std::vector<uint8_t> h_states(m_num_cells, 0); // FLUID
    std::vector<int> h_flags(m_num_cells, 0);
    std::vector<uint32_t> h_mat(m_num_cells, 0);
    
    // Simple grid layout
    int idx = 0;
    for (int z = 0; z < 8; ++z) {
        for (int y = 0; y < 8; ++y) {
            for (int x = 0; x < 8; ++x) {
                h_x[idx] = x;
                h_y[idx] = y;
                h_z[idx] = z;
                
                // Mark some cells for refinement (demo)
                if (x >= 3 && x <= 4 && y >= 3 && y <= 4 && z >= 3 && z <= 4) {
                    h_flags[idx] = 1; // Refine center region
                }
                
                idx++;
            }
        }
    }
    
    // Upload to GPU
    clEnqueueWriteBuffer(m_queue, m_coord_x, CL_TRUE, 0, m_num_cells * sizeof(int), h_x.data(), 0, nullptr, nullptr);
    clEnqueueWriteBuffer(m_queue, m_coord_y, CL_TRUE, 0, m_num_cells * sizeof(int), h_y.data(), 0, nullptr, nullptr);
    clEnqueueWriteBuffer(m_queue, m_coord_z, CL_TRUE, 0, m_num_cells * sizeof(int), h_z.data(), 0, nullptr, nullptr);
    clEnqueueWriteBuffer(m_queue, m_levels, CL_TRUE, 0, m_num_cells * sizeof(uint8_t), h_levels.data(), 0, nullptr, nullptr);
    clEnqueueWriteBuffer(m_queue, m_cell_states, CL_TRUE, 0, m_num_cells * sizeof(uint8_t), h_states.data(), 0, nullptr, nullptr);
    clEnqueueWriteBuffer(m_queue, m_refine_flags, CL_TRUE, 0, m_num_cells * sizeof(int), h_flags.data(), 0, nullptr, nullptr);
    clEnqueueWriteBuffer(m_queue, m_material_id, CL_TRUE, 0, m_num_cells * sizeof(uint32_t), h_mat.data(), 0, nullptr, nullptr);
    
    FL_LOG(INFO) << "Initialized mesh with " << m_num_cells << " cells (center region marked for refinement)";
}

std::unique_ptr<runtime::ExecutionGraph> SimulationBuilder::buildFromScript(const std::string& script_content) {
    FL_LOG(INFO) << "Building execution graph from .fl script using ANTLR";
    
    try {
        // Parse with ANTLR
        antlr4::ANTLRInputStream input(script_content);
        FluidLoomSimulationLexer lexer(&input);
        antlr4::CommonTokenStream tokens(&lexer);
        FluidLoomSimulationParser parser(&tokens);
        
        auto* ast = parser.simulationFile();
        
        FL_LOG(INFO) << "Parsing complete, building execution graph";
        
        // Use existing FieldRegistry to get field buffers
        auto& field_registry = registry::FieldRegistry::instance();
        
        // TODO: Fields should be registered from field declarations in .fl file
        // For now, we'll just check if they exist and log
        FL_LOG(INFO) << "Checking field registry...";
        auto all_fields = field_registry.getAllNames();
        FL_LOG(INFO) << "Found " << all_fields.size() << " registered fields";
        
        auto graph = std::make_unique<runtime::ExecutionGraph>();
        
        // Traverse AST to extract kernel calls and adapt_mesh
        int kernel_count = 0;
        int adapt_mesh_count = 0;
        
        // Visit all code blocks (initial_condition, time_loop, final_output)
        for (auto* codeBlock : ast->codeBlock()) {
            if (codeBlock->TIME_LOOP()) {
                FL_LOG(INFO) << "Processing time_loop block";
                
                // Traverse all statements directly in the code block
                for (auto* stmt : codeBlock->scriptStatement()) {
                    std::string stmt_text = stmt->getText();
                    
                    // Check if this starts with "run"
                    if (stmt_text.find("run") == 0 || stmt_text.find("run ") != std::string::npos) {
                        // Extract kernel name and parameters
                        size_t run_pos = stmt_text.find("run");
                        if (run_pos != std::string::npos) {
                            size_t name_start = run_pos + 3;
                            while (name_start < stmt_text.length() && std::isspace(stmt_text[name_start])) name_start++;
                            size_t name_end = name_start;
                            while (name_end < stmt_text.length() && (std::isalnum(stmt_text[name_end]) || stmt_text[name_end] == '_')) name_end++;
                            
                            if (name_end > name_start) {
                                std::string kernel_name = stmt_text.substr(name_start, name_end - name_start);
                                
                                // Extract parameters from parentheses
                                std::vector<std::string> params;
                                size_t paren_start = stmt_text.find('(', name_end);
                                size_t paren_end = stmt_text.find(')', paren_start);
                                if (paren_start != std::string::npos && paren_end != std::string::npos) {
                                    std::string params_str = stmt_text.substr(paren_start + 1, paren_end - paren_start - 1);
                                    // Simple comma split (ignoring named parameters for now)
                                    size_t pos = 0;
                                    while (pos < params_str.length()) {
                                        size_t comma = params_str.find(',', pos);
                                        if (comma == std::string::npos) comma = params_str.length();
                                        std::string param = params_str.substr(pos, comma - pos);
                                        // Trim whitespace and extract identifier
                                        size_t id_start = 0;
                                        while (id_start < param.length() && std::isspace(param[id_start])) id_start++;
                                        size_t id_end = id_start;
                                        while (id_end < param.length() && (std::isalnum(param[id_end]) || param[id_end] == '_')) id_end++;
                                        if (id_end > id_start) {
                                            params.push_back(param.substr(id_start, id_end - id_start));
                                        }
                                        pos = comma + 1;
                                    }
                                }
                                
                                FL_LOG(INFO) << "Extracted kernel: " << kernel_name << " with " << params.size() << " parameters";
                                
                                // Generate kernel code based on name and parameters
                                std::string kernel_source = generateKernelCode(kernel_name, params);
                                
                                // Compile kernel
                                cl_int err;
                                const char* source_ptr = kernel_source.c_str();
                                size_t source_len = kernel_source.length();
                                cl_program program = clCreateProgramWithSource(m_context, 1, &source_ptr, &source_len, &err);
                                
                                if (err == CL_SUCCESS) {
                                    err = clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr);
                                    if (err != CL_SUCCESS) {
                                        // Get build log
                                        size_t log_size;
                                        clGetProgramBuildInfo(program, nullptr, CL_PROGRAM_BUILD_LOG, 0, nullptr, &log_size);
                                        std::vector<char> log(log_size);
                                        clGetProgramBuildInfo(program, nullptr, CL_PROGRAM_BUILD_LOG, log_size, log.data(), nullptr);
                                        FL_LOG(ERROR) << "Kernel build failed for " << kernel_name << ": " << log.data();
                                    } else {
                                        cl_kernel kernel = clCreateKernel(program, kernel_name.c_str(), &err);
                                        if (err == CL_SUCCESS) {
                                            FL_LOG(INFO) << "Successfully compiled kernel: " << kernel_name;
                                            
                                            // Create KernelNode with compiled kernel
                                            auto kernel_node = std::make_shared<runtime::nodes::KernelNode>(
                                                kernel_name, kernel_source
                                            );
                                            
                                            // Store kernel handle (need to add setter to KernelNode)
                                            kernel_node->setKernel(kernel, m_context, m_queue);
                                            
                                            graph->addNode(kernel_node);
                                            kernel_count++;
                                        } else {
                                            FL_LOG(ERROR) << "Failed to create kernel object for " << kernel_name;
                                        }
                                    }
                                } else {
                                    FL_LOG(ERROR) << "Failed to create program for " << kernel_name;
                                }
                            }
                        }
                    }
                    
                    // Check if this is an adapt_mesh statement
                    auto* adaptStmt = dynamic_cast<FluidLoomSimulationParser::AdaptMeshStatementContext*>(stmt);
                    if (adaptStmt) {
                        FL_LOG(INFO) << "Found adapt_mesh() call in time_loop";
                        
                        // Create AdaptationEngine
                        auto adapt_config = adaptation::AdaptationConfig{};
                        adapt_config.enforce_2_1_balance = true;
                        adapt_config.max_balance_iterations = 10;
                        
                        auto adapt_engine = new adaptation::AdaptationEngine(m_context, m_queue, adapt_config);
                        
                        // Create and add AdaptMeshNode
                        auto adapt_node = std::make_shared<runtime::nodes::AdaptMeshNode>("adapt_mesh", adapt_engine);
                        adapt_node->bindMesh(
                            &m_coord_x, &m_coord_y, &m_coord_z,
                            &m_levels, &m_cell_states, &m_refine_flags, &m_material_id,
                            &m_num_cells, &m_capacity
                        );
                        
                        graph->addNode(adapt_node);
                        adapt_mesh_count++;
                    }
                }
            }
        }
        
        FL_LOG(INFO) << "Extracted " << kernel_count << " kernel(s) and " << adapt_mesh_count << " adapt_mesh call(s)";
        
        if (kernel_count == 0 && adapt_mesh_count == 0) {
            FL_LOG(WARN) << "No kernels or adapt_mesh() calls found, falling back to stub";
            return buildStub();
        }
        
        FL_LOG(INFO) << "Graph built with " << graph->getNodeCount() << " nodes";
        
        return graph;
        
    } catch (const std::exception& e) {
        FL_LOG(ERROR) << "Failed to parse .fl file: " << e.what();
        FL_LOG(INFO) << "Falling back to stub implementation";
        return buildStub();
    }
}

std::string SimulationBuilder::generateKernelCode(const std::string& kernel_name, const std::vector<std::string>& params) {
    // Generate OpenCL kernel code based on kernel name and parameters
    std::ostringstream oss;
    
    // Generate kernel signature
    oss << "__kernel void " << kernel_name << "(";
    
    // Add parameters as __global float* buffers
    for (size_t i = 0; i < params.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << "__global float* " << params[i];
    }
    
    // Add num_cells parameter
    if (!params.empty()) oss << ", ";
    oss << "int num_cells";
    
    oss << ") {\n";
    oss << "    int gid = get_global_id(0);\n";
    oss << "    if (gid >= num_cells) return;\n";
    oss << "    \n";
    
    // Generate kernel body based on kernel name patterns
    if (kernel_name.find("init") == 0 || kernel_name.find("initialize") == 0) {
        // Initialization kernels - set first parameter to 1.0
        if (!params.empty()) {
            oss << "    // Initialize " << params[0] << "\n";
            oss << "    " << params[0] << "[gid] = 1.0f;\n";
        }
    } else if (kernel_name.find("lbm") != std::string::npos) {
        // LBM kernels - placeholder for now
        oss << "    // LBM kernel: " << kernel_name << "\n";
        oss << "    // TODO: Implement actual LBM physics\n";
        for (const auto& param : params) {
            oss << "    // Parameter: " << param << "\n";
        }
    } else if (kernel_name.find("compute") != std::string::npos || kernel_name.find("calc") != std::string::npos) {
        // Computation kernels
        oss << "    // Computation kernel: " << kernel_name << "\n";
        if (params.size() >= 2) {
            oss << "    // Read from " << params[0] << ", write to " << params[1] << "\n";
            oss << "    " << params[1] << "[gid] = " << params[0] << "[gid];\n";
        }
    } else if (kernel_name.find("apply") != std::string::npos || kernel_name.find("boundary") != std::string::npos) {
        // Boundary condition kernels
        oss << "    // Boundary condition: " << kernel_name << "\n";
        oss << "    // TODO: Implement boundary logic\n";
    } else {
        // Generic kernel
        oss << "    // Generic kernel: " << kernel_name << "\n";
        oss << "    // Parameters: " << params.size() << "\n";
    }
    
    oss << "}\n";
    
    return oss.str();
}

std::string SimulationBuilder::generateKernelStub(const std::string& kernel_name) {
    // Generate a simple OpenCL kernel stub that does nothing but can be compiled
    std::ostringstream oss;
    oss << "__kernel void " << kernel_name << "(__global float* dummy) {\n";
    oss << "    int gid = get_global_id(0);\n";
    oss << "    // Stub kernel for: " << kernel_name << "\n";
    oss << "    if (gid == 0) {\n";
    oss << "        dummy[0] = 1.0f;\n";
    oss << "    }\n";
    oss << "}\n";
    return oss.str();
}

std::unique_ptr<runtime::ExecutionGraph> SimulationBuilder::buildStub() {
    FL_LOG(INFO) << "Building execution graph (stub implementation)";
    
    auto graph = std::make_unique<runtime::ExecutionGraph>();
    
    // Create AdaptationEngine
    auto adapt_config = adaptation::AdaptationConfig{};
    adapt_config.enforce_2_1_balance = true;
    adapt_config.max_balance_iterations = 10;
    
    auto adapt_engine = new adaptation::AdaptationEngine(m_context, m_queue, adapt_config);
    
    // Create and add AdaptMeshNode
    auto adapt_node = std::make_shared<runtime::nodes::AdaptMeshNode>("adapt_mesh", adapt_engine);
    adapt_node->bindMesh(
        &m_coord_x, &m_coord_y, &m_coord_z,
        &m_levels, &m_cell_states, &m_refine_flags, &m_material_id,
        &m_num_cells, &m_capacity
    );
    
    graph->addNode(adapt_node);
    
    FL_LOG(INFO) << "Added AdaptMeshNode to execution graph";
    FL_LOG(INFO) << "Graph built with " << graph->getNodeCount() << " nodes";
    
    return graph;
}

} // namespace parsing
} // namespace fluidloom
