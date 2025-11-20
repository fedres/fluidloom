#pragma once

#include <mpi.h>
#include <memory>
#include <string>
#include <vector>

namespace fluidloom {
namespace mpi {

class MPIEnvironment {
public:
    // Singleton access
    static MPIEnvironment& getInstance();
    
    // Deleted copy/move
    MPIEnvironment(const MPIEnvironment&) = delete;
    MPIEnvironment& operator=(const MPIEnvironment&) = delete;
    
    // Getters
    int getRank() const { return m_rank; }
    int getSize() const { return m_size; }
    bool isMaster() const { return m_rank == 0; }
    
    // Barrier synchronization
    void barrier() const;
    
    // Abort execution
    void abort(int error_code) const;
    
    // Check if initialized
    bool isInitialized() const { return m_initialized; }

private:
    MPIEnvironment();
    ~MPIEnvironment();
    
    int m_rank = 0;
    int m_size = 1;
    bool m_initialized = false;
    bool m_owned = false; // True if this class called MPI_Init
};

} // namespace mpi
} // namespace fluidloom
