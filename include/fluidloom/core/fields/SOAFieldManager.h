#pragma once

#include "fluidloom/core/fields/FieldDescriptor.h"
#include "fluidloom/core/backend/IBackend.h"
#include <memory>
#include <unordered_map>
#include <mutex>
#include <vector>

namespace fluidloom {
namespace fields {

/**
 * @brief Manages SOA field allocations with 64-byte alignment
 * Central memory manager for all field data in the engine
 */
class SOAFieldManager {
public:
    explicit SOAFieldManager(IBackend* backend);
    
    // Delete copy/move
    SOAFieldManager(const SOAFieldManager&) = delete;
    SOAFieldManager& operator=(const SOAFieldManager&) = delete;
    
    // Allocate field for N cells
    FieldHandle allocate(const FieldDescriptor& desc, size_t num_cells);
    
    // Resize existing field (preserves data up to min(old, new))
    void resize(FieldHandle handle, size_t new_num_cells);
    
    // Deallocate field
    void deallocate(FieldHandle handle);
    
    // Get device pointer for kernel launch
    void* getDevicePtr(FieldHandle handle, uint16_t component = 0) const;
    
    // Get descriptor
    const FieldDescriptor& getDescriptor(FieldHandle handle) const;
    
    // Get allocation size
    size_t getAllocationSize(FieldHandle handle) const;
    
    // Version tracking
    void markDirty(FieldHandle handle);
    void markClean(FieldHandle handle);
    bool isDirty(FieldHandle handle) const;
    uint64_t getVersion(FieldHandle handle) const;
    
    // Memory usage
    size_t getTotalMemoryUsage() const;
    size_t getMemoryUsage(FieldHandle handle) const;
    
private:
    struct FieldState {
        FieldDescriptor descriptor;
        size_t num_cells{0};
        std::vector<void*> device_ptrs;  // One per component
        DeviceBufferPtr device_buffer;
        uint64_t version{1};  // Start at 1
        size_t pitch{0};  // Bytes between components (SOA stride)
    };
    
    IBackend* backend_;
    std::unordered_map<uint64_t, FieldState> fields_;
    mutable std::mutex mutex_;
    
    // Helper: compute aligned size
    static size_t computeAlignedSize(size_t bytes_per_cell, size_t num_cells);
    
    // Helper: validate and get field state
    FieldState& getFieldState(FieldHandle handle);
    const FieldState& getFieldState(FieldHandle handle) const;
};

} // namespace fields
} // namespace fluidloom
