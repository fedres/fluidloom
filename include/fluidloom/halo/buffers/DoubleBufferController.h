#pragma once

#include <memory>
#include "fluidloom/core/backend/IBackend.h"
#include "fluidloom/core/soa/Buffer.h"
#include "fluidloom/halo/GhostRange.h"
#include "fluidloom/halo/InterpolationParams.h"

namespace fluidloom {
namespace halo {

/**
 * @brief Manages ping-pong buffering for non-blocking halo exchange
 * 
 * State machine:
 *   Buffer A: Packing → Posting Send/Recv → Computing → Waiting
 *   Buffer B: Available for next iteration
 *   On completion: swap roles
 */
class DoubleBufferController {
public:
    struct BufferPair {
        Buffer buffer_a;
        Buffer buffer_b;
        size_t capacity_bytes{0};
    };
    
    DoubleBufferController(IBackend* backend, size_t capacity_mb);
    ~DoubleBufferController();
    
    // Non-copyable
    DoubleBufferController(const DoubleBufferController&) = delete;
    DoubleBufferController& operator=(const DoubleBufferController&) = delete;
    
    // Get current pack buffer (for packing)
    Buffer* getPackBuffer() {
        return is_using_a ? &buffer_pair.buffer_a : &buffer_pair.buffer_b;
    }
    
    // Get opposite buffer (for communication)
    Buffer* getCommBuffer() {
        return is_using_a ? &buffer_pair.buffer_b : &buffer_pair.buffer_a;
    }
    
    // Swap buffers after communication completes
    void swap() {
        is_using_a = !is_using_a;
        swap_count++;
    }
    
    // Resize buffers if needed (e.g., after AMR)
    void resize(size_t new_capacity_mb);
    
    // Synchronization
    void fence();  // Wait for all pending operations on both buffers
    
    // Statistics
    size_t getSwapCount() const { return swap_count; }
    size_t getCurrentCapacity() const { return buffer_pair.capacity_bytes; }
    
private:
    IBackend* backend;
    BufferPair buffer_pair;
    bool is_using_a;  // Current pack target
    size_t swap_count;
};

} // namespace halo
} // namespace fluidloom
