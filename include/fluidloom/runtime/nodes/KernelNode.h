#pragma once
// @stable - Kernel execution node from DSL

#include "fluidloom/runtime/nodes/ExecutionNode.h"
#include <unordered_map>

// Forward declare OpenCL types
typedef struct _cl_kernel* cl_kernel;
typedef struct _cl_mem* cl_mem;
typedef struct _cl_context* cl_context;
typedef struct _cl_device_id* cl_device_id;
typedef struct _cl_command_queue* cl_command_queue;

namespace fluidloom {
namespace runtime {
namespace nodes {

/**
 * @brief Represents a user-defined kernel from DSL
 * 
 * Contains the compiled OpenCL kernel. The execute() method enqueues
 * the kernel with correct global/local work sizes.
 * 
 * Note: For Module 9, we use a simplified version without full DSL integration.
 * Module 10 will add the KernelAST parsing.
 */
class KernelNode : public ExecutionNode {
private:
    // Compiled OpenCL kernel object
    cl_kernel cl_kernel_handle = nullptr;
    
    // OpenCL context and queue for execution
    cl_context context = nullptr;
    cl_command_queue command_queue = nullptr;
    
    // Work size configuration
    size_t global_work_size = 0;
    size_t local_work_size = 256;  // Default, tunable
    
    // Field buffer bindings (populated at launch)
    std::unordered_map<std::string, cl_mem> field_bindings;
    
    // Kernel source (for Module 9, simplified)
    std::string kernel_source;
    
public:
    KernelNode(std::string name, std::string source = "")
        : ExecutionNode(NodeType::KERNEL, std::move(name)), 
          kernel_source(std::move(source)) {}
    
    ~KernelNode();
    
    // Compile kernel (called once during graph construction)
    void compile(cl_context context, cl_device_id device);
    
    // Set field bindings before execution
    void bindField(const std::string& field_name, cl_mem buffer);
    
    // Override execution
    cl_event execute(cl_event wait_event) override;
    
    // Visitor pattern
    void accept(Visitor& visitor) override { visitor.visit(*this); }
    
    // For fusion (Module 12)
    void setGlobalWorkSize(size_t size) { global_work_size = size; }
    void setLocalWorkSize(size_t size) { local_work_size = size; }
    
    // Set compiled kernel (for code generation)
    void setKernel(cl_kernel kernel, cl_context ctx, cl_command_queue queue);
    
    const std::string& getKernelSource() const { return kernel_source; }
};

} // namespace nodes
} // namespace runtime
} // namespace fluidloom
