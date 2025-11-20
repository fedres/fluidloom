#include "fluidloom/core/hashmap/HashTableManager.h"
#include "fluidloom/common/Logger.h"
#include <chrono>
#include <cmath>

namespace fluidloom {
namespace hashmap {

HashTableManager::HashTableManager(IBackend* backend)
    : backend_(backend),
      compactor_(std::make_unique<CompactionEngine>(backend)),
      hash_build_kernel_(nullptr) {
    if (!backend) {
        throw std::invalid_argument("Backend must not be null");
    }
    
    // Initialize empty table
    current_table_.capacity = 0;
    current_table_.size = 0;
    current_table_.log2_capacity = 0;
    current_table_.keys_device_ptr = nullptr;
    current_table_.values_device_ptr = nullptr;
    
    metadata_.num_cells = 0;
    metadata_.active_capacity = 0;
    metadata_.bytes_allocated = 0;
    metadata_.build_time_ms = 0;
    metadata_.average_probe_count = 0.0f;
    metadata_.max_probe_count = 0;
}

HashTableManager::~HashTableManager() {
    // RAII handles cleanup
}

void HashTableManager::ensureKernelsCompiled() {
    if (hash_build_kernel_) return;  // Already compiled
    
    FL_LOG(INFO) << "Compiling hash table kernels...";
    
    // TODO: Load and compile kernels from kernels/hashmap/
    // hash_build_kernel_ = backend_->compileKernel("hash_build", ...);
    // hash_clear_kernel_ = backend_->compileKernel("hash_clear", ...);
    // hash_query_kernel_ = backend_->compileKernel("hash_validate", ...);
}

uint64_t HashTableManager::computeCapacity(size_t num_cells) {
    // Capacity must be power of 2 and >= num_cells / MAX_LOAD_FACTOR
    double min_capacity = static_cast<double>(num_cells) / MAX_LOAD_FACTOR;
    uint32_t log2_cap = static_cast<uint32_t>(std::ceil(std::log2(min_capacity)));
    
    // Clamp to reasonable range
    log2_cap = std::max(log2_cap, INITIAL_CAPACITY_POW2);
    log2_cap = std::min(log2_cap, static_cast<uint32_t>(30));  // Max 1B slots
    
    return 1ULL << log2_cap;
}

void HashTableManager::allocateTable(size_t num_cells) {
    uint64_t new_capacity = computeCapacity(num_cells);
    
    if (new_capacity == current_table_.capacity) {
        return;  // Already allocated
    }
    
    FL_LOG(INFO) << "Allocating hash table: capacity=" << new_capacity 
                 << " for " << num_cells << " cells";
    
    // Allocate device buffers
    table_keys_ = backend_->allocateBuffer(new_capacity * sizeof(uint64_t));
    table_values_ = backend_->allocateBuffer(new_capacity * sizeof(uint32_t));
    
    // Update table struct
    current_table_.capacity = new_capacity;
    current_table_.log2_capacity = static_cast<uint32_t>(std::log2(new_capacity));
    current_table_.keys_device_ptr = table_keys_->getDevicePointer();
    current_table_.values_device_ptr = table_values_->getDevicePointer();
    
    metadata_.bytes_allocated = new_capacity * (sizeof(uint64_t) + sizeof(uint32_t));
    metadata_.active_capacity = static_cast<size_t>(new_capacity * MAX_LOAD_FACTOR);
    
    FL_LOG(INFO) << "Hash table allocated: " << metadata_.bytes_allocated / (1024*1024) << " MB";
}

void HashTableManager::clearTable() {
    if (!table_keys_) return;
    
    // Launch clear kernel
    // For now, use CPU clear (slow but works)
    std::vector<uint64_t> empty_keys(current_table_.capacity, HASH_EMPTY_KEY);
    std::vector<uint32_t> empty_values(current_table_.capacity, HASH_INVALID_VALUE);
    
    backend_->copyHostToDevice(empty_keys.data(), *table_keys_, 
                               current_table_.capacity * sizeof(uint64_t));
    backend_->copyHostToDevice(empty_values.data(), *table_values_,
                               current_table_.capacity * sizeof(uint32_t));
    
    current_table_.size = 0;
}

double HashTableManager::rebuild(const std::vector<uint64_t>& hilbert_indices,
                                  const std::vector<uint32_t>& array_indices) {
    if (hilbert_indices.size() != array_indices.size()) {
        throw std::invalid_argument("Mismatched input sizes");
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    size_t num_cells = hilbert_indices.size();
    FL_LOG(INFO) << "Rebuilding hash table for " << num_cells << " cells";
    
    // Allocate table if needed
    allocateTable(num_cells);
    
    // Clear table
    clearTable();
    
    // Copy input data to device (temporary buffers)
    auto input_keys = backend_->allocateBuffer(num_cells * sizeof(uint64_t));
    auto input_values = backend_->allocateBuffer(num_cells * sizeof(uint32_t));
    
    backend_->copyHostToDevice(hilbert_indices.data(), *input_keys, 
                               num_cells * sizeof(uint64_t));
    backend_->copyHostToDevice(array_indices.data(), *input_values,
                               num_cells * sizeof(uint32_t));
    
    // Launch build kernel
    // TODO: backend_->launchKernel(hash_build_kernel_, ...);
    
    // For now, just log a placeholder
    FL_LOG(WARN) << "Hash table build kernel not yet implemented";
    
    backend_->finish();
    
    auto end_time = std::chrono::high_resolution_clock::now();
    double duration_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    
    metadata_.num_cells = num_cells;
    metadata_.build_time_ms = static_cast<uint32_t>(duration_ms);
    
    FL_LOG(INFO) << "Hash table rebuild completed in " << duration_ms << " ms";
    
    return duration_ms;
}

std::vector<uint32_t> HashTableManager::query(const std::vector<uint64_t>& query_keys) {
    size_t num_queries = query_keys.size();
    std::vector<uint32_t> results(num_queries, HASH_INVALID_VALUE);
    
    if (num_queries == 0 || !table_keys_) {
        return results;
    }
    
    // Launch query kernel
    // TODO: Implement GPU batch query
    FL_LOG(WARN) << "Hash table query not yet implemented";
    
    return results;
}

} // namespace hashmap
} // namespace fluidloom
