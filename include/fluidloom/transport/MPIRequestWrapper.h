#pragma once

#ifdef FLUIDLOOM_MPI_ENABLED
#include <mpi.h>
#endif

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif
#include "fluidloom/transport/GPUAwareBuffer.h"

namespace fluidloom {
namespace transport {

/**
 * @brief Union type that holds either MPI_Request or cl_event for dependency chaining
 * 
 * This is the **critical interop structure** that allows the event chain (Module 7)
 * to wait for MPI completion before launching unpack kernels. The type is determined
 * at runtime based on whether GPU-aware MPI or P2P is used.
 * 
 * The memory layout must match EventNode in Module 7's EventChain.
 */
class MPIRequestWrapper {
private:
    enum class RequestType { MPI, CL_EVENT, P2P };
    
    RequestType type;
    
    union {
        #ifdef FLUIDLOOM_MPI_ENABLED
        MPI_Request mpi_request;   // For non-blocking MPI
        #endif
        cl_event cl_event_handle;   // For clEnqueueCopyBuffer (P2P)
    };
    
    // Associated buffer (for unmarking on completion)
    GPUAwareBuffer* buffer;
    GPUAwareBuffer* dst_buffer; // Optional secondary buffer (e.g., for P2P copy)
    
public:
    // Constructor for MPI request
    #ifdef FLUIDLOOM_MPI_ENABLED
    explicit MPIRequestWrapper(MPI_Request req, GPUAwareBuffer* buf) 
        : type(RequestType::MPI), mpi_request(req), buffer(buf), dst_buffer(nullptr) {
        if (buffer) buffer->markBound();
    }
    #endif
    
    // Constructor for OpenCL event
    explicit MPIRequestWrapper(cl_event event, GPUAwareBuffer* buf) 
        : type(RequestType::CL_EVENT), cl_event_handle(event), buffer(buf), dst_buffer(nullptr) {
        if (buffer) buffer->markBound();
    }
    
    // Constructor for P2P copy (no MPI request, but still need buffer tracking)
    explicit MPIRequestWrapper(GPUAwareBuffer* buf) 
        : type(RequestType::P2P), cl_event_handle(nullptr), buffer(buf), dst_buffer(nullptr) {
        if (buffer) buffer->markBound();
    }
    
    // Destructor ensures buffer is unmarked
    ~MPIRequestWrapper() {
        if (buffer && buffer->is_bound_to_mpi) {
            buffer->markUnbound();
        }
        if (dst_buffer && dst_buffer->is_bound_to_mpi) {
            dst_buffer->markUnbound();
        }
    }
    
    void setDstBuffer(GPUAwareBuffer* buf) {
        dst_buffer = buf;
        if (dst_buffer) dst_buffer->markBound();
    }
    
    // Wait for request to complete
    void wait();
    
    // Test if request is complete (non-blocking)
    bool test();
    
    // Get underlying native handle for MPI_Waitall
    #ifdef FLUIDLOOM_MPI_ENABLED
    MPI_Request* getMPIRequest() { 
        return (type == RequestType::MPI) ? &mpi_request : nullptr; 
    }
    #endif
    
    cl_event* getCLEvent() { 
        return (type == RequestType::CL_EVENT || type == RequestType::P2P) ? &cl_event_handle : nullptr; 
    }
    
    // Cancel outstanding request
    void cancel();
    
    // Unmark buffers manually (used by wait_all)
    void markUnbound() {
        if (buffer) buffer->markUnbound();
        if (dst_buffer) dst_buffer->markUnbound();
    }
};

} // namespace transport
} // namespace fluidloom
