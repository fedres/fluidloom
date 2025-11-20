#include "fluidloom/halo/events/EventChain.h"
#include "fluidloom/common/Logger.h"

namespace fluidloom {
namespace halo {

EventChain::EventChain() : mpi_complete_event(nullptr) {
    (void)mpi_complete_event; // Suppress unused private field warning
}

EventChain::~EventChain() {
    // Cleanup events
}

void EventChain::addPackEvent(void* event) {
    pack_events.push_back(event);
}

#ifdef FLUIDLOOM_MPI_ENABLED
void EventChain::addMpiRequest(MPI_Request request) {
    mpi_requests.push_back(request);
}
#else
void EventChain::addMpiRequest(int request_handle) {
    mpi_requests.push_back(request_handle);
}
#endif

void EventChain::waitForPack() {
    // In a real implementation, we might need to explicitly wait on host
    // if MPI implementation doesn't support CUDA-aware/OpenCL-aware MPI
    // For now, we assume we might need to synchronize
    // backend->waitForEvents(pack_events);
}

void EventChain::waitForMpi() {
    #ifdef FLUIDLOOM_MPI_ENABLED
    if (!mpi_requests.empty()) {
        MPI_Waitall(mpi_requests.size(), mpi_requests.data(), MPI_STATUSES_IGNORE);
    }
    #endif
    // After MPI wait, we would signal mpi_complete_event if using user events
}

const std::vector<void*>& EventChain::getUnpackWaitList() const {
    // In a real implementation, this would return mpi_complete_event
    // For now, return empty or pack events if direct copy
    return pack_events; 
}

void EventChain::reset() {
    pack_events.clear();
    mpi_requests.clear();
}

} // namespace halo
} // namespace fluidloom
