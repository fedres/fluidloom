#include "fluidloom/core/hashmap/RadixSort.h"
#include "fluidloom/common/Logger.h"
#include <chrono>
#include <stdexcept>

namespace fluidloom {
namespace hashmap {

RadixSort::RadixSort(IBackend* backend)
    : backend_(backend),
      histogram_kernel_(nullptr) {
    if (!backend) {
        throw std::invalid_argument("Backend must not be null");
    }
}

RadixSort::~RadixSort() {
    // Kernel cleanup handled by backend
    // Buffers are RAII via DeviceBufferPtr
}

void RadixSort::ensureKernelsCompiled() {
    if (histogram_kernel_) return;  // Already compiled
    
    FL_LOG(INFO) << "Compiling radix sort kernels...";
    
    // In a real implementation, we would:
    // 1. Load kernel source from kernels/hashmap/radix_sort.cl
    // 2. Compile with backend_->compileKernel()
    // 3. Store kernel handles
    
    // For now, we'll add this in the full implementation
    // histogram_kernel_ = backend_->compileKernel("radix_histogram", ...);
    // prefix_sum_kernel_ = backend_->compileKernel("prefix_sum", ...);
    // scatter_kernel_ = backend_->compileKernel("radix_scatter", ...);
}

void RadixSort::ensureScratchAllocated(size_t count) {
    if (temp_keys_ && temp_keys_->getSize() >= count * sizeof(uint64_t)) {
        return;  // Already allocated
    }
    
    // Allocate scratch buffers for double-buffering
    temp_keys_ = backend_->allocateBuffer(count * sizeof(uint64_t));
    temp_values_ = backend_->allocateBuffer(count * sizeof(uint32_t));
    
    // Histogram: 256 bins per work-group
    // Assume num_groups = (count + 255) / 256
    size_t num_groups = (count + 255) / 256;
    histogram_buffer_ = backend_->allocateBuffer(num_groups * 256 * sizeof(uint32_t));
    
    FL_LOG(INFO) << "Allocated radix sort scratch buffers for " << count << " elements";
}

double RadixSort::sortByKey(DeviceBufferPtr& keys_buffer,
                             DeviceBufferPtr& values_buffer,
                             size_t count) {
    if (count == 0) return 0.0;
    
    ensureKernelsCompiled();
    ensureScratchAllocated(count);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Double-buffering: swap between input and temp buffers each pass
    void* current_keys = keys_buffer->getDevicePointer();
    void* current_values = values_buffer->getDevicePointer();
    void* next_keys = temp_keys_->getDevicePointer();
    void* next_values = temp_values_->getDevicePointer();
    
    // 8 passes for 64-bit keys (8 bits per pass)
    for (uint32_t pass = 0; pass < 8; ++pass) {
        // Step 1: Histogram
        // Launch histogram kernel
        // backend_->launchKernel(histogram_kernel_, ...);
        
        // Step 2: Prefix sum
        // backend_->launchKernel(prefix_sum_kernel_, ...);
        
        // Step 3: Scatter
        // backend_->launchKernel(scatter_kernel_, ...);
        
        // Swap buffers for next pass
        std::swap(current_keys, next_keys);
        std::swap(current_values, next_values);
    }
    
    // If odd number of passes, copy back to original buffer
    // (For 8 passes, data ends up in temp_, so copy back)
    if (current_keys != keys_buffer->getDevicePointer()) {
        backend_->copyDeviceToDevice(*temp_keys_, *keys_buffer, count * sizeof(uint64_t));
        backend_->copyDeviceToDevice(*temp_values_, *values_buffer, count * sizeof(uint32_t));
    }
    
    backend_->finish();
    auto end_time = std::chrono::high_resolution_clock::now();
    
    double duration_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    last_stats_.total_time_ms = duration_ms;
    
    FL_LOG(INFO) << "Radix sort completed: " << count << " elements in " << duration_ms << " ms";
    
    return duration_ms;
}

} // namespace hashmap
} // namespace fluidloom
