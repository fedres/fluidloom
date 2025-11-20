#include "fluidloom/common/mpi/MPIEnvironment.h"
#include "fluidloom/common/Logger.h"
#include <stdexcept>

namespace fluidloom {
namespace mpi {

MPIEnvironment& MPIEnvironment::getInstance() {
    static MPIEnvironment instance;
    return instance;
}

MPIEnvironment::MPIEnvironment() {
    int flag = 0;
    MPI_Initialized(&flag);
    
    if (!flag) {
        int argc = 0;
        char** argv = nullptr;
        int provided;
        MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &provided);
        m_owned = true;
        m_initialized = true;
        
        if (provided < MPI_THREAD_FUNNELED) {
            FL_LOG(WARN) << "MPI provided thread support level " << provided 
                           << " is less than requested " << MPI_THREAD_FUNNELED;
        }
    } else {
        m_initialized = true;
        m_owned = false;
    }
    
    MPI_Comm_rank(MPI_COMM_WORLD, &m_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &m_size);
    
    FL_LOG(INFO) << "MPI Initialized. Rank " << m_rank << " of " << m_size;
}

MPIEnvironment::~MPIEnvironment() {
    if (m_owned && m_initialized) {
        FL_LOG(INFO) << "Finalizing MPI (Rank " << m_rank << ")";
        MPI_Finalize();
    }
}

void MPIEnvironment::barrier() const {
    if (m_initialized) {
        MPI_Barrier(MPI_COMM_WORLD);
    }
}

void MPIEnvironment::abort(int error_code) const {
    if (m_initialized) {
        FL_LOG(ERROR) << "Aborting MPI execution with code " << error_code;
        MPI_Abort(MPI_COMM_WORLD, error_code);
    }
}

} // namespace mpi
} // namespace fluidloom
