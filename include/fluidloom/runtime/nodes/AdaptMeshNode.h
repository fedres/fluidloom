#pragma once

#include "fluidloom/runtime/nodes/ExecutionNode.h"
#include "fluidloom/adaptation/AdaptationEngine.h"
#include <vector>

namespace fluidloom {
namespace runtime {
namespace nodes {

/**
 * @brief Node that triggers mesh adaptation (AMR)
 * 
 * Integrates Module 11 AdaptationEngine into the execution DAG.
 * This node is unique because it can RESIZE buffers.
 * Therefore, it holds pointers to cl_mem handles, not the handles themselves.
 */
class AdaptMeshNode : public ExecutionNode {
private:
    fluidloom::adaptation::AdaptationEngine* engine = nullptr;
    
    // Pointers to mesh buffers (so we can update them on resize)
    cl_mem* coord_x = nullptr;
    cl_mem* coord_y = nullptr;
    cl_mem* coord_z = nullptr;
    cl_mem* levels = nullptr;
    cl_mem* cell_states = nullptr;
    cl_mem* refine_flags = nullptr;
    cl_mem* material_id = nullptr;
    
    size_t* num_cells = nullptr;
    size_t* capacity = nullptr;
    
    // Fields to be remapped
    std::vector<cl_mem*> fields;
    uint32_t num_field_components = 0;
    
public:
    AdaptMeshNode(std::string name, fluidloom::adaptation::AdaptationEngine* engine_ptr)
        : ExecutionNode(NodeType::ADAPT_MESH, std::move(name)), engine(engine_ptr) {}
    
    ~AdaptMeshNode() = default;
    
    // Bind mesh state buffers
    void bindMesh(
        cl_mem* x, cl_mem* y, cl_mem* z,
        cl_mem* l, cl_mem* s,
        cl_mem* flags,
        cl_mem* mat,
        size_t* count,
        size_t* cap
    ) {
        coord_x = x; coord_y = y; coord_z = z;
        levels = l; cell_states = s;
        refine_flags = flags; material_id = mat;
        num_cells = count; capacity = cap;
    }
    
    // Bind fields to be remapped
    void bindFields(std::vector<cl_mem*> field_ptrs, uint32_t components) {
        fields = std::move(field_ptrs);
        num_field_components = components;
    }
    
    // Execute adaptation
    cl_event execute(cl_event wait_event) override;
    
    // Visitor pattern
    void accept(Visitor& visitor) override { visitor.visit(*this); }
};

} // namespace nodes
} // namespace runtime
} // namespace fluidloom
