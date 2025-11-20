#include "fluidloom/transport/MPIRequestWrapper.h"
#include "fluidloom/common/Logger.h"

namespace fluidloom {
namespace transport {

void MPIRequestWrapper::wait() {
    if (type == RequestType::MPI) {
        #ifdef FLUIDLOOM_MPI_ENABLED
        MPI_Wait(&mpi_request, MPI_STATUS_IGNORE);
        #endif
    } else if (type == RequestType::CL_EVENT || type == RequestType::P2P) {
        if (cl_event_handle) {
            clWaitForEvents(1, &cl_event_handle);
        }
    }
    markUnbound();
}

bool MPIRequestWrapper::test() {
    if (type == RequestType::MPI) {
        #ifdef FLUIDLOOM_MPI_ENABLED
        int flag = 0;
        MPI_Test(&mpi_request, &flag, MPI_STATUS_IGNORE);
        if (flag) {
            markUnbound();
            return true;
        }
        return false;
        #else
        return true;
        #endif
    } else if (type == RequestType::CL_EVENT || type == RequestType::P2P) {
        if (!cl_event_handle) return true;
        
        cl_int status;
        clGetEventInfo(cl_event_handle, CL_EVENT_COMMAND_EXECUTION_STATUS, sizeof(cl_int), &status, nullptr);
        if (status == CL_COMPLETE) {
            markUnbound();
            return true;
        }
        return false;
    }
    return true;
}

void MPIRequestWrapper::cancel() {
    if (type == RequestType::MPI) {
        #ifdef FLUIDLOOM_MPI_ENABLED
        MPI_Cancel(&mpi_request);
        MPI_Request_free(&mpi_request);
        #endif
    }
    // OpenCL events cannot be cancelled easily
    markUnbound();
}

} // namespace transport
} // namespace fluidloom
