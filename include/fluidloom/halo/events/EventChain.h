#pragma once

#include "fluidloom/core/backend/IBackend.h"
#include <vector>
#include <memory>

#ifdef FLUIDLOOM_MPI_ENABLED
#include <mpi.h>
#endif

namespace fluidloom {
namespace halo {

/**
 * @brief Manages dependencies between OpenCL events and MPI requests
 * 
 * Ensures proper ordering:
 * 1. Pack kernels complete (OpenCL event)
 * 2. MPI sends posted (MPI_Request)
 * 3. MPI receives complete (MPI_Request)
 * 4. Unpack kernels start (Wait for MPI)
 */
class EventChain {
public:
    EventChain();
    ~EventChain();
    
    // Add OpenCL event to wait for (e.g., pack completion)
    void addPackEvent(void* event);
    
    // Add MPI request to track (e.g., Isend/Irecv)
    #ifdef FLUIDLOOM_MPI_ENABLED
    void addMpiRequest(MPI_Request request);
    #else
    // Mock for non-MPI builds
    void addMpiRequest(int request_handle);
    #endif
    
    // Wait for all pack events (before MPI send)
    void waitForPack();
    
    // Wait for all MPI requests (before unpack)
    void waitForMpi();
    
    // Get events that unpack kernel must wait on
    const std::vector<void*>& getUnpackWaitList() const;
    
    // Clear all events/requests for next cycle
    void reset();
    
private:
    std::vector<void*> pack_events;
    
    #ifdef FLUIDLOOM_MPI_ENABLED
    std::vector<MPI_Request> mpi_requests;
    #else
    std::vector<int> mpi_requests;
    #endif
    
    // Event created after MPI completion to unblock GPU
    void* mpi_complete_event;
};

} // namespace halo
} // namespace fluidloom
