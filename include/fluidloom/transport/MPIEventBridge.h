#pragma once

#include "fluidloom/halo/events/EventChain.h"
#include "fluidloom/transport/MPIRequestWrapper.h"
#include "fluidloom/core/backend/IBackend.h"
#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>

namespace fluidloom {
namespace transport {

/**
 * @brief Bridges MPI_Request completion to cl_event for unified dependency chain
 * 
 * This class solves the **critical interop problem**: MPI and OpenCL have different
 * synchronization primitives. We need to wait for MPI_Isend/Irecv to complete
 * before launching unpack kernels.
 * 
 * Two strategies:
 * 1. **Polling**: Create a cl_event that polls MPI_Test in a separate thread
 * 2. **Native**: Use cl_event from clEnqueueCopyBuffer (P2P case)
 * 
 * The bridge ensures the EventChain from Module 7 can treat MPI completions
 * as regular cl_event dependencies.
 */
class MPIEventBridge {
private:
    IBackend* backend;
    
    // Thread for polling MPI completions (if not using native events)
    std::thread polling_thread;
    std::atomic<bool> stop_polling;
    std::queue<std::pair<MPIRequestWrapper*, cl_event>> pending_tests;
    std::mutex queue_mutex;
    std::condition_variable queue_cv;
    
public:
    explicit MPIEventBridge(IBackend* backend);
    ~MPIEventBridge();
    
    // Create a cl_event that will be signaled when MPI request completes
    cl_event bridgeMPIRequest(MPIRequestWrapper* request);
    
    // Check if a request is complete (for polling)
    static bool isMPIComplete(MPIRequestWrapper* request);
    
    // Shutdown polling thread
    void shutdown();
    
private:
    // Polling loop thread function
    void pollingLoop();
};

} // namespace transport
} // namespace fluidloom
