#pragma once

// Mock MPI implementation for testing without MPI library
#ifdef FLUIDLOOM_MOCK_MPI

#include <unordered_map>

#define MPI_SUCCESS 0
#define MPI_THREAD_MULTIPLE 3
#define MPI_BYTE 0x4c00010d

typedef struct ompi_communicator_t* MPI_Comm;
typedef struct ompi_request_t* MPI_Request;
typedef struct { int MPI_SOURCE; int MPI_TAG; int MPI_ERROR; } MPI_Status;
typedef int MPI_Datatype;

#define MPI_COMM_WORLD ((MPI_Comm)0x44000000)
#define MPI_STATUS_IGNORE ((MPI_Status*)0)
#define MPI_STATUSES_IGNORE ((MPI_Status*)0)

// Global mock state
extern int mock_mpi_rank;
extern int mock_mpi_size;
extern std::unordered_map<MPI_Request, bool> mock_mpi_request_complete;

// Mock functions
inline int MPI_Initialized(int* flag) { *flag = 1; return MPI_SUCCESS; }
inline int MPI_Init_thread(void* a, void* b, int c, int* d) { 
    mock_mpi_rank = 0; mock_mpi_size = 1; *d = MPI_THREAD_MULTIPLE; return MPI_SUCCESS; 
}
inline int MPI_Comm_rank(MPI_Comm, int* rank) { *rank = mock_mpi_rank; return MPI_SUCCESS; }
inline int MPI_Comm_size(MPI_Comm, int* size) { *size = mock_mpi_size; return MPI_SUCCESS; }
inline int MPI_Isend(void* buf, int count, MPI_Datatype, int dest, int tag, MPI_Comm, MPI_Request* req) {
    *req = (MPI_Request)(new char[1]); // Dummy allocation
    mock_mpi_request_complete[*req] = false;
    return MPI_SUCCESS;
}
inline int MPI_Irecv(void* buf, int count, MPI_Datatype, int src, int tag, MPI_Comm, MPI_Request* req) {
    *req = (MPI_Request)(new char[1]);
    mock_mpi_request_complete[*req] = false;
    return MPI_SUCCESS;
}
inline int MPI_Waitall(int count, MPI_Request* reqs, MPI_Status*) {
    for (int i = 0; i < count; ++i) {
        mock_mpi_request_complete[reqs[i]] = true;
        delete[] (char*)reqs[i];
    }
    return MPI_SUCCESS;
}
inline int MPI_Wait(MPI_Request* req, MPI_Status*) {
    mock_mpi_request_complete[*req] = true;
    delete[] (char*)*req;
    return MPI_SUCCESS;
}
inline int MPI_Test(MPI_Request* req, int* flag, MPI_Status*) {
    *flag = mock_mpi_request_complete[*req];
    if (*flag) {
        delete[] (char*)*req;
    }
    return MPI_SUCCESS;
}
inline int MPI_Cancel(MPI_Request* req) {
    return MPI_SUCCESS;
}
inline int MPI_Request_free(MPI_Request* req) {
    delete[] (char*)*req;
    return MPI_SUCCESS;
}
inline int MPI_Finalize() { return MPI_SUCCESS; }
inline int MPI_Barrier(MPI_Comm) { return MPI_SUCCESS; }
inline int MPI_Allgather(const void*, int, MPI_Datatype, void*, int, MPI_Datatype, MPI_Comm) { return MPI_SUCCESS; }

#endif // FLUIDLOOM_MOCK_MPI
