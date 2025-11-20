#include "fluidloom/core/fields/SOAFieldManager.h"
#include "fluidloom/common/Logger.h"
#include "fluidloom/common/FluidLoomError.h"
#include <cstring>
#include <algorithm>

namespace fluidloom {
namespace fields {

SOAFieldManager::SOAFieldManager(IBackend* backend)
    : backend_(backend) {
    if (!backend) {
        throw std::invalid_argument("Backend must not be null");
    }
}

FieldHandle SOAFieldManager::allocate(const FieldDescriptor& desc, size_t num_cells) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!desc.isValid()) {
        throw std::invalid_argument("Invalid field descriptor");
    }
    
    if (fields_.find(desc.id) != fields_.end()) {
        FL_LOG(ERROR) << "Field '" << desc.name << "' already allocated";
        throw std::runtime_error("Field already allocated");
    }
    
    // Compute allocation size with 64-byte alignment
    size_t bytes_per_cell = desc.bytesPerCell();
    size_t aligned_cell_size = (bytes_per_cell + 63) & ~63;
    size_t total_bytes = aligned_cell_size * num_cells;
    
    // Allocate device buffer
    auto buffer = backend_->allocateBuffer(total_bytes);
    if (!buffer) {
        FL_LOG(ERROR) << "Failed to allocate " << total_bytes << " bytes for field " << desc.name;
        throw std::runtime_error("Buffer allocation failed");
    }
    
    // Create field state
    FieldState state;
    state.descriptor = desc;
    state.num_cells = num_cells;
    state.device_buffer = std::move(buffer);
    state.pitch = aligned_cell_size;
    state.version = 1;
    
    // Compute component pointers (SOA layout)
    char* base_ptr = static_cast<char*>(state.device_buffer->getDevicePointer());
    for (uint16_t comp = 0; comp < desc.num_components; ++comp) {
        size_t offset = comp * bytes_per_cell;
        state.device_ptrs.push_back(base_ptr + offset);
    }
    
    fields_[desc.id] = std::move(state);
    
    FL_LOG(INFO) << "Allocated field '" << desc.name << "': " 
                 << num_cells << " cells × " << bytes_per_cell << " bytes = "
                 << total_bytes / (1024*1024) << " MB (pitch=" << aligned_cell_size << ")";
    
    return FieldHandle(desc.id);
}

void SOAFieldManager::resize(FieldHandle handle, size_t new_num_cells) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& state = getFieldState(handle);
    
    if (new_num_cells == state.num_cells) {
        return;  // No-op
    }
    
    size_t bytes_per_cell = state.descriptor.bytesPerCell();
    size_t aligned_cell_size = (bytes_per_cell + 63) & ~63;
    size_t new_total_bytes = aligned_cell_size * new_num_cells;
    
    // Allocate new buffer
    auto new_buffer = backend_->allocateBuffer(new_total_bytes);
    if (!new_buffer) {
        throw std::runtime_error("Failed to allocate resized buffer");
    }
    
    // Copy data (preserve up to min)
    size_t copy_cells = std::min(state.num_cells, new_num_cells);
    size_t copy_bytes = aligned_cell_size * copy_cells;
    
    if (copy_bytes > 0) {
        backend_->copyDeviceToDevice(*state.device_buffer, *new_buffer, copy_bytes);
    }
    
    // Replace buffer
    state.device_buffer = std::move(new_buffer);
    state.num_cells = new_num_cells;
    
    // Update component pointers
    char* base_ptr = static_cast<char*>(state.device_buffer->getDevicePointer());
    state.device_ptrs.clear();
    for (uint16_t comp = 0; comp < state.descriptor.num_components; ++comp) {
        size_t offset = comp * bytes_per_cell;
        state.device_ptrs.push_back(base_ptr + offset);
    }
    
    // Mark as dirty
    state.version++;
    state.descriptor.gpu_state.is_dirty = true;
    
    FL_LOG(INFO) << "Resized field '" << state.descriptor.name << "' to " << new_num_cells << " cells";
}

void SOAFieldManager::deallocate(FieldHandle handle) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = fields_.find(handle.id);
    if (it == fields_.end()) {
        FL_LOG(WARN) << "Deallocate called on non-existent field: " << handle.id;
        return;
    }
    
    FL_LOG(INFO) << "Deallocated field: " << it->second.descriptor.name;
    fields_.erase(it);
}

void* SOAFieldManager::getDevicePtr(FieldHandle handle, uint16_t component) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto& state = getFieldState(handle);
    
    if (component >= state.descriptor.num_components) {
        throw std::out_of_range("Component index out of range");
    }
    
    return state.device_ptrs[component];
}

const FieldDescriptor& SOAFieldManager::getDescriptor(FieldHandle handle) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return getFieldState(handle).descriptor;
}

size_t SOAFieldManager::getAllocationSize(FieldHandle handle) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return getFieldState(handle).num_cells;
}

void SOAFieldManager::markDirty(FieldHandle handle) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& state = getFieldState(handle);
    state.version++;
    state.descriptor.gpu_state.is_dirty = true;
}

void SOAFieldManager::markClean(FieldHandle handle) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& state = getFieldState(handle);
    state.descriptor.gpu_state.is_dirty = false;
}

bool SOAFieldManager::isDirty(FieldHandle handle) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return getFieldState(handle).descriptor.gpu_state.is_dirty;
}

uint64_t SOAFieldManager::getVersion(FieldHandle handle) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return getFieldState(handle).version;
}

size_t SOAFieldManager::getTotalMemoryUsage() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t total = 0;
    for (const auto& pair : fields_) {
        total += pair.second.device_buffer->getSize();
    }
    return total;
}

size_t SOAFieldManager::getMemoryUsage(FieldHandle handle) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return getFieldState(handle).device_buffer->getSize();
}

size_t SOAFieldManager::computeAlignedSize(size_t bytes_per_cell, size_t num_cells) {
    size_t aligned_cell = (bytes_per_cell + 63) & ~63;
    return aligned_cell * num_cells;
}

SOAFieldManager::FieldState& SOAFieldManager::getFieldState(FieldHandle handle) {
    auto it = fields_.find(handle.id);
    if (it == fields_.end()) {
        throw std::runtime_error("Field handle not found");
    }
    return it->second;
}

const SOAFieldManager::FieldState& SOAFieldManager::getFieldState(FieldHandle handle) const {
    auto it = fields_.find(handle.id);
    if (it == fields_.end()) {
        throw std::runtime_error("Field handle not found");
    }
    return it->second;
}

} // namespace fields
} // namespace fluidloom
