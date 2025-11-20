#pragma once

#include "fluidloom/core/backend/IBackend.h"
#include <cstdint>
#include <memory>

namespace fluidloom {
namespace hashmap {

/**
 * @brief OpenCL-based radix sort for 64-bit keys with 32-bit values
 * 
 * Implements LSB radix sort with 8-bit passes (8 passes total).
 * Uses histogram→prefix sum→scatter pattern.
 */
class RadixSort {
public:
    explicit RadixSort(IBackend* backend);
    ~RadixSort();
    
    // Delete copy/move
    RadixSort(const RadixSort&) = delete;
    RadixSort& operator=(const RadixSort&) = delete;
    
    /**
     * @brief Sort key-value pairs by keys in ascending order
     * 
     * @param keys_buffer Device buffer of uint64_t keys
     * @param values_buffer Device buffer of uint32_t values
     * @param count Number of elements
     * @return Sort time in milliseconds
     */
    double sortByKey(DeviceBufferPtr& keys_buffer,
                     DeviceBufferPtr& values_buffer,
                     size_t count);
    
    /**
     * @brief Get statistics from last sort
     */
    struct Stats {
        double total_time_ms;
        double histogram_time_ms;
        double prefix_sum_time_ms;
        double scatter_time_ms;
    };
    
    const Stats& getLastStats() const { return last_stats_; }
    
private:
    IBackend* backend_;
    
    // Scratch buffers for double-buffering
    DeviceBufferPtr temp_keys_;
    DeviceBufferPtr temp_values_;
    DeviceBufferPtr histogram_buffer_;  // 256 bins × num_groups
    
    // Kernels (compiled once) - placeholders for future implementation
    void* histogram_kernel_;
    
    Stats last_stats_;
    
    // Helper: compile kernels on first use
    void ensureKernelsCompiled();
    
    // Helper: allocate scratch buffers if needed
    void ensureScratchAllocated(size_t count);
};

} // namespace hashmap
} // namespace fluidloom
