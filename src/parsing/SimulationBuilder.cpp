#include "fluidloom/parsing/SimulationBuilder.h"
#include "fluidloom/parsing/OpenCLBackend.h"
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
    
    // Create OpenCL backend wrapper
    m_backend = std::make_unique<OpenCLBackend>(context, queue);
    
    // Create SOAFieldManager
    m_field_manager = std::make_unique<fields::SOAFieldManager>(m_backend.get());
    
    // Create GeometryPlacer
    m_geometry_placer = std::make_unique<geometry::GeometryPlacer>();
    
    FL_LOG(INFO) << "SimulationBuilder initialized with SOAFieldManager and GeometryPlacer";
    
    initializeMeshBuffers();
}

#include "FluidLoomSimulationParser.h"
    FL_LOG(INFO) << "Processing geometry definitions...";
    
    auto* ast = parser->simulationFile();
    std::vector<geometry::GeometryDescriptor> geometries;
    
    for (auto* geomDef : ast->geometryDefinition()) {
        geometry::GeometryDescriptor geom;
        geom.name = geomDef->IDENTIFIER()->getText();
        
        // Default transform
        geom.transform = {0,0,0, 1,1,1, 0,0,0};
        
        for (auto* prop : geomDef->geometryProperty()) {
            if (prop->TYPE()) {
                auto typeCtx = prop->geometryType();
                if (typeCtx->STL()) geom.type = geometry::GeometryDescriptor::Type::STL_MESH;
                else if (typeCtx->IMPLICIT()) geom.type = geometry::GeometryDescriptor::Type::IMPLICIT_FUNC;
                else if (typeCtx->BOX()) geom.type = geometry::GeometryDescriptor::Type::BOX;
                else if (typeCtx->SPHERE()) geom.type = geometry::GeometryDescriptor::Type::SPHERE;
                else if (typeCtx->CYLINDER()) geom.type = geometry::GeometryDescriptor::Type::CYLINDER;
            }
            else if (prop->SOURCE()) {
                std::string src = prop->STRING()->getText();
                // Remove quotes
                if (src.length() >= 2) geom.source = src.substr(1, src.length() - 2);
            }
            else if (prop->MATERIAL_ID()) {
                geom.material_id = std::stoi(prop->INT()->getText());
            }
            else if (prop->TRANSLATE()) {
                auto vec = prop->vectorLiteral();
                // Assuming vectorLiteral structure: LPAREN (FLOAT|INT) COMMA ... RPAREN
                // We need to extract 3 numbers. This is a bit manual without a visitor.
                // Let's assume the text is "(x,y,z)" and parse it manually for now
                // or use the children if accessible.
                // Simplified parsing:
                std::string vecText = vec->getText();
                sscanf(vecText.c_str(), "(%f,%f,%f)", &geom.transform.tx, &geom.transform.ty, &geom.transform.tz);
            }
            else if (prop->SCALE()) {
                std::string vecText = prop->vectorLiteral()->getText();
                sscanf(vecText.c_str(), "(%f,%f,%f)", &geom.transform.sx, &geom.transform.sy, &geom.transform.sz);
            }
            else if (prop->ROTATE()) {
                std::string vecText = prop->vectorLiteral()->getText();
                sscanf(vecText.c_str(), "(%f,%f,%f)", &geom.transform.rx, &geom.transform.ry, &geom.transform.rz);
            }
            else if (prop->RADIUS()) {
                geom.params.radius = std::stof(prop->children[2]->getText());
            }
            else if (prop->HEIGHT()) {
                geom.params.height = std::stof(prop->children[2]->getText());
            }
            else if (prop->WIDTH()) {
                geom.params.width = std::stof(prop->children[2]->getText());
            }
            else if (prop->LENGTH()) {
                geom.params.length = std::stof(prop->children[2]->getText());
            }
        }
        
        try {
            geom.validate();
            geometries.push_back(geom);
            FL_LOG(INFO) << "Parsed geometry: " << geom.name;
        } catch (const std::exception& e) {
            FL_LOG(ERROR) << "Invalid geometry definition '" << geom.name << "': " << e.what();
        }
    }
    
    if (!geometries.empty()) {
        std::vector<CellCoord> fluid_cells;
        // In a real implementation, we would read back current cells from GPU
        // For now, we'll just generate cells for the geometry and log them
        // or append to a local list if we were building the mesh on CPU first.
        
        // Since we initialized mesh on GPU in initializeMeshBuffers, we can't easily merge here
        // without reading back.
        // For this phase, we will just demonstrate the placement logic.
        
        geometry::GeometryDescriptor::AABB domain_bbox = {-10.0f, -10.0f, -10.0f, 10.0f, 10.0f, 10.0f};
        m_geometry_placer->placeGeometry(geometries, fluid_cells, 1.0f, domain_bbox);
        
        FL_LOG(INFO) << "Placed geometry, generated " << fluid_cells.size() << " solid cells (not uploaded to GPU in this demo)";
    }
}

SimulationBuilder::~SimulationBuilder() {
    // Release mesh buffers
    if (m_coord_x) clReleaseMemObject(m_coord_x);
    if (m_coord_y) clReleaseMemObject(m_coord_y);
    if (m_coord_z) clReleaseMemObject(m_coord_z);
    if (m_levels) clReleaseMemObject(m_levels);
    if (m_cell_states) clReleaseMemObject(m_cell_states);
    if (m_refine_flags) clReleaseMemObject(m_refine_flags);
    if (m_material_id) clReleaseMemObject(m_material_id);
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
#include "FluidLoomSimulationLexer.h"
        antlr4::CommonTokenStream tokens(&lexer);
#include "FluidLoomSimulationParser.h"
        
        auto* ast = parser.simulationFile();
        
        FL_LOG(INFO) << "Parsing complete, building execution graph";
        
        // Parse field declarations from AST and register them
        parseFieldDeclarations(&parser);
        
        // Process geometry definitions
        processGeometry(&parser);
        
        // Allocate buffers for all registered fields
        allocateFieldBuffers();
        
        // Use existing FieldRegistry to get field buffers
        auto& field_registry = registry::FieldRegistry::instance();
        auto all_fields = field_registry.getAllNames();
        FL_LOG(INFO) << "Registered " << all_fields.size() << " fields from .fl file";
        
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
                    // Check for run statements using ANTLR AST
                    auto* runStmt = stmt->runStatement();
                    if (runStmt) {
                        std::string kernel_name = runStmt->IDENTIFIER()->getText();
                        
                        // Extract parameters from argumentList
                        std::vector<std::string> param_names;
                        if (runStmt->argumentList()) {
                            for (auto* arg : runStmt->argumentList()->argument()) {
                                // Get the expression text (for now, assume simple identifiers)
                                std::string arg_text = arg->expression()->getText();
                                // Extract identifier (handle simple cases)
                                size_t id_start = 0;
                                while (id_start < arg_text.length() && !std::isalnum(arg_text[id_start]) && arg_text[id_start] != '_') id_start++;
                                size_t id_end = id_start;
                                while (id_end < arg_text.length() && (std::isalnum(arg_text[id_end]) || arg_text[id_end] == '_')) id_end++;
                                if (id_end > id_start) {
                                    param_names.push_back(arg_text.substr(id_start, id_end - id_start));
                                }
                            }
                        }
                        
                        FL_LOG(INFO) << "Extracted kernel: " << kernel_name << " with " << param_names.size() << " parameters";
                        for (const auto& param : param_names) {
                            FL_LOG(INFO) << "  - Parameter: " << param;
                        }
                        
                        // Generate kernel code based on name and parameters
                        std::string kernel_source = generateKernelCode(kernel_name, param_names);
                        
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
                                    
                                    // Set kernel handle
                                    kernel_node->setKernel(kernel, m_context, m_queue);
                                    
                                    // Bind field buffers to kernel arguments
                                    for (const auto& param_name : param_names) {
                                        auto handle_it = m_field_handles.find(param_name);
                                        if (handle_it != m_field_handles.end()) {
                                            cl_mem buffer = static_cast<cl_mem>(
                                                m_field_manager->getDevicePtr(handle_it->second)
                                            );
                                            kernel_node->bindField(param_name, buffer);
                                            FL_LOG(INFO) << "Bound field '" << param_name << "' to kernel " << kernel_name;
                                        } else {
                                            FL_LOG(WARN) << "Field '" << param_name << "' not found in field handles for kernel " << kernel_name;
                                        }
                                    }
                                    
                                    // Set work size based on number of cells
                                    kernel_node->setGlobalWorkSize(m_num_cells);
                                    kernel_node->setLocalWorkSize(256);
                                    
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
                    
                    // Check if this is an adapt_mesh statement
#include "FluidLoomSimulationParser.h"
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

#include "FluidLoomSimulationParser.h"
    FL_LOG(INFO) << "Parsing field declarations from AST";
    
    auto& field_registry = registry::FieldRegistry::instance();
    auto* ast = parser->simulationFile();
    
    // Extract field declarations
    for (auto* fieldDecl : ast->fieldDeclaration()) {
        std::string field_name = fieldDecl->IDENTIFIER()->getText();
        std::string field_type_str = fieldDecl->fieldType()->getText();
        
        // Determine number of components based on field type
        uint16_t num_components = 1;
        fields::FieldType field_type = fields::FieldType::FLOAT32;
        
        if (field_type_str == "scalar") {
            num_components = 1;
            field_type = fields::FieldType::FLOAT32;
        } else if (field_type_str == "vec2") {
            num_components = 2;
            field_type = fields::FieldType::FLOAT32;
        } else if (field_type_str == "vec3") {
            num_components = 3;
            field_type = fields::FieldType::FLOAT32;
        } else if (field_type_str == "int") {
            num_components = 1;
            field_type = fields::FieldType::INT32;
        } else if (field_type_str == "float") {
            num_components = 1;
            field_type = fields::FieldType::FLOAT32;
        } else if (field_type_str == "double") {
            num_components = 1;
            field_type = fields::FieldType::FLOAT64;
        } else {
            FL_LOG(WARN) << "Unknown field type '" << field_type_str << "' for field '" 
                         << field_name << "', defaulting to scalar float";
        }
        
        // Create field descriptor with halo depth 1 (standard for CFD)
        fields::FieldDescriptor desc(field_name, field_type, num_components, 1);
        
        // Register field
        if (field_registry.registerField(desc)) {
            FL_LOG(INFO) << "Registered field '" << field_name << "' (type: " 
                         << field_type_str << ", components: " << num_components << ")";
        } else {
            FL_LOG(WARN) << "Field '" << field_name << "' already registered, skipping";
        }
    }
}

void SimulationBuilder::allocateFieldBuffers() {
    FL_LOG(INFO) << "Allocating field buffers via SOAFieldManager";
    
    auto& field_registry = registry::FieldRegistry::instance();
    auto all_field_names = field_registry.getAllNames();
    
    for (const auto& field_name : all_field_names) {
        auto desc_opt = field_registry.lookupByName(field_name);
        if (!desc_opt) {
            FL_LOG(ERROR) << "Field '" << field_name << "' not found in registry";
            continue;
        }
        
        const auto& desc = *desc_opt;
        
        // Allocate buffer for this field
        try {
            fields::FieldHandle handle = m_field_manager->allocate(desc, m_num_cells);
            m_field_handles[field_name] = handle;
            
            // Initialize with zeros
            size_t bytes_per_cell = desc.bytesPerCell();
            size_t total_bytes = bytes_per_cell * m_num_cells;
            std::vector<uint8_t> zeros(total_bytes, 0);
            
            cl_mem buffer = static_cast<cl_mem>(m_field_manager->getDevicePtr(handle));
            clEnqueueWriteBuffer(m_queue, buffer, CL_TRUE, 0, total_bytes, 
                               zeros.data(), 0, nullptr, nullptr);
            
            FL_LOG(INFO) << "Allocated and initialized field '" << field_name << "': " 
                         << m_num_cells << " cells × " << bytes_per_cell << " bytes = "
                         << total_bytes << " bytes";
        } catch (const std::exception& e) {
            FL_LOG(ERROR) << "Failed to allocate field '" << field_name << "': " << e.what();
        }
    }
    
    FL_LOG(INFO) << "Total field memory: " 
                 << m_field_manager->getTotalMemoryUsage() / (1024.0 * 1024.0) << " MB";
}

} // namespace parsing

} // namespace fluidloom
