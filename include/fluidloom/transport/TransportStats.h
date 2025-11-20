#pragma once

#include <cstdint>
#include <string>
#include <sstream>

namespace fluidloom {
namespace transport {

/**
 * @brief Per-exchange statistics for performance profiling
 * 
 * These metrics are exported to Chrome Trace JSON and must remain stable
 * for performance regression tracking across versions.
 */
struct TransportStats {
    // Communication volume
    uint64_t bytes_sent;
    uint64_t bytes_received;
    uint32_t num_messages_sent;
    uint32_t num_messages_received;
    
    // Timing breakdown (microseconds)
    uint64_t post_send_time_us;     // Time in MPI_Isend calls
    uint64_t post_recv_time_us;     // Time in MPI_Irecv calls
    uint64_t wait_time_us;          // Time in MPI_Waitall
    uint64_t p2p_copy_time_us;      // Time in clEnqueueCopyBuffer (if P2P)
    
    // GPU-aware MPI details
    bool used_gpu_aware;            // True if direct device memory transfer
    bool used_p2p;                  // True if P2P copy was used
    
    // Errors
    uint32_t mpi_error_count;
    uint32_t p2p_error_count;
    
    TransportStats() : bytes_sent(0), bytes_received(0), num_messages_sent(0),
                      num_messages_received(0), post_send_time_us(0),
                      post_recv_time_us(0), wait_time_us(0), p2p_copy_time_us(0),
                      used_gpu_aware(false), used_p2p(false),
                      mpi_error_count(0), p2p_error_count(0) {}
    
    std::string toJSON() const {
        std::stringstream ss;
        ss << "{"
           << "\"bytes_sent\": " << bytes_sent << ","
           << "\"bytes_received\": " << bytes_received << ","
           << "\"num_messages_sent\": " << num_messages_sent << ","
           << "\"num_messages_received\": " << num_messages_received << ","
           << "\"post_send_time_us\": " << post_send_time_us << ","
           << "\"post_recv_time_us\": " << post_recv_time_us << ","
           << "\"wait_time_us\": " << wait_time_us << ","
           << "\"p2p_copy_time_us\": " << p2p_copy_time_us << ","
           << "\"used_gpu_aware\": " << (used_gpu_aware ? "true" : "false") << ","
           << "\"used_p2p\": " << (used_p2p ? "true" : "false") << ","
           << "\"mpi_error_count\": " << mpi_error_count << ","
           << "\"p2p_error_count\": " << p2p_error_count
           << "}";
        return ss.str();
    }
    
    void reset() { *this = TransportStats(); }
};

} // namespace transport
} // namespace fluidloom
