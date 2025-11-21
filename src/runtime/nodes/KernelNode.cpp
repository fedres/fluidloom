#include "fluidloom/runtime/nodes/KernelNode.h"
#include "fluidloom/common/Logger.h"

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

namespace fluidloom {
namespace runtime {
namespace nodes {

KernelNode::~KernelNode() {
    if (cl_kernel_handle) {
        clReleaseKernel(cl_kernel_handle);
    }
}

void KernelNode::compile(cl_context context, cl_device_id device) {
    (void)context;
    (void)device;
    // Simplified for Module 9 - full implementation in Module 10
    // For now, just a placeholder
}

void KernelNode::bindField(const std::string& field_name, cl_mem buffer) {
    field_bindings[field_name] = buffer;
}

void KernelNode::setKernel(cl_kernel kernel, cl_context ctx, cl_command_queue queue) {
    cl_kernel_handle = kernel;
    context = ctx;
    command_queue = queue;
}

cl_event KernelNode::execute(cl_event wait_event) {
    if (!cl_kernel_handle) {
        FL_LOG(ERROR) << "KernelNode " << node_name << " has no compiled kernel";
        return nullptr;
    }
    
    if (!command_queue) {
        FL_LOG(ERROR) << "KernelNode " << node_name << " has no command queue";
        return nullptr;
    }
    
    // Set kernel arguments from field_bindings
    cl_uint arg_idx = 0;
    for (const auto& [field_name, buffer] : field_bindings) {
        cl_int err = clSetKernelArg(cl_kernel_handle, arg_idx, sizeof(cl_mem), &buffer);
        if (err != CL_SUCCESS) {
            FL_LOG(ERROR) << "Failed to set kernel argument " << arg_idx 
                          << " (field: " << field_name << ") for kernel " << node_name 
                          << ", error code: " << err;
            return nullptr;
        }
        FL_LOG(INFO) << "Set kernel argument " << arg_idx << " for field " << field_name;
        arg_idx++;
    }
    
    // Calculate work sizes
    // Ensure global_work_size is a multiple of local_work_size
    size_t global_size = global_work_size;
    size_t local_size = local_work_size;
    
    if (global_size == 0) {
        FL_LOG(WARN) << "Global work size is 0 for kernel " << node_name << ", skipping execution";
        return nullptr;
    }
    
    // Round up to nearest multiple of local_work_size
    if (global_size % local_size != 0) {
        global_size = ((global_size + local_size - 1) / local_size) * local_size;
        FL_LOG(INFO) << "Rounded global work size to " << global_size 
                     << " (multiple of local size " << local_size << ")";
    }
    
    FL_LOG(INFO) << "Enqueueing kernel " << node_name 
                 << " with global_size=" << global_size 
                 << ", local_size=" << local_size;
    
    // Enqueue kernel
    cl_event completion_event;
    cl_int err = clEnqueueNDRangeKernel(
        command_queue, 
        cl_kernel_handle, 
        1,  // work_dim (1D)
        nullptr,  // global_work_offset
        &global_size,  // global_work_size
        &local_size,  // local_work_size
        wait_event ? 1 : 0,  // num_events_in_wait_list
        wait_event ? &wait_event : nullptr,  // event_wait_list
        &completion_event  // event
    );
    
    if (err != CL_SUCCESS) {
        FL_LOG(ERROR) << "Failed to enqueue kernel " << node_name 
                      << ", error code: " << err;
        return nullptr;
    }
    
    FL_LOG(INFO) << "Successfully enqueued kernel " << node_name;
    
    return completion_event;
}

} // namespace nodes
} // namespace runtime
} // namespace fluidloom
