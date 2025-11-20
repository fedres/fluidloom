#include "fluidloom/core/hashmap/CompactionEngine.h"
#include "fluidloom/common/Logger.h"
#include <algorithm>
#include <chrono>

namespace fluidloom {
namespace hashmap {

CompactionEngine::CompactionEngine(IBackend* backend)
    : backend_(backend),
      sorter_(std::make_unique<RadixSort>(backend)),
      peak_memory_usage_(0) {
    if (!backend) {
        throw std::invalid_argument("Backend must not be null");
    }
}

CompactionEngine::~CompactionEngine() {
    // RAII handles cleanup
}

void CompactionEngine::ensureScratchAllocated(size_t count) {
    if (scratch_keys_ && scratch_keys_->getSize() >= count * sizeof(uint64_t)) {
        return;
    }
    
    scratch_keys_ = backend_->allocateBuffer(count * sizeof(uint64_t));
    scratch_values_ = backend_->allocateBuffer(count * sizeof(uint32_t));
    
    peak_memory_usage_ = count * (sizeof(uint64_t) + sizeof(uint32_t));
    FL_LOG(INFO) << "Allocated compaction scratch: " << peak_memory_usage_ / (1024*1024) << " MB";
}

CompactionResult CompactionEngine::compactAndPermute(
    const std::vector<uint64_t>& hilbert_indices,
    const std::vector<uint32_t>& cell_state
) {
    if (hilbert_indices.size() != cell_state.size()) {
        throw std::invalid_argument("Mismatched input sizes");
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    const size_t total_cells = hilbert_indices.size();
    
    // Step 1: Filter active cells on CPU
    std::vector<uint64_t> valid_hilbert;
    std::vector<uint32_t> valid_indices;
    valid_hilbert.reserve(total_cells);
    valid_indices.reserve(total_cells);
    
    for (size_t i = 0; i < total_cells; ++i) {
        if (cell_state[i] != 0 && hilbert_indices[i] != HASH_EMPTY_KEY) {
            valid_hilbert.push_back(hilbert_indices[i]);
            valid_indices.push_back(static_cast<uint32_t>(i));
        }
    }
    
    size_t num_valid = valid_hilbert.size();
    FL_LOG(INFO) << "Compaction: " << num_valid << " / " << total_cells << " cells active";
    
    if (num_valid == 0) {
        return CompactionResult{0, std::vector<uint32_t>(total_cells, HASH_INVALID_VALUE), 0.0};
    }
    
    // Step 2: Copy to GPU
    ensureScratchAllocated(num_valid);
    backend_->copyHostToDevice(valid_hilbert.data(), *scratch_keys_, num_valid * sizeof(uint64_t));
    backend_->copyHostToDevice(valid_indices.data(), *scratch_values_, num_valid * sizeof(uint32_t));
    
    // Step 3: Sort by Hilbert index on GPU
    sorter_->sortByKey(scratch_keys_, scratch_values_, num_valid);
    
    // Step 4: Copy sorted data back
    std::vector<uint64_t> sorted_hilbert(num_valid);
    std::vector<uint32_t> sorted_old_indices(num_valid);
    backend_->copyDeviceToHost(*scratch_keys_, sorted_hilbert.data(), num_valid * sizeof(uint64_t));
    backend_->copyDeviceToHost(*scratch_values_, sorted_old_indices.data(), num_valid * sizeof(uint32_t));
    
    // Step 5: Generate permutation vector
    std::vector<uint32_t> permutation(total_cells, HASH_INVALID_VALUE);
    for (uint32_t new_idx = 0; new_idx < num_valid; ++new_idx) {
        uint32_t old_idx = sorted_old_indices[new_idx];
        permutation[old_idx] = new_idx;
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    double duration_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    
    FL_LOG(INFO) << "Compaction completed in " << duration_ms << " ms";
    
    return CompactionResult{
        num_valid,
        std::move(permutation),
        duration_ms
    };
}

void CompactionEngine::applyPermutationToFields(
    const std::vector<void*>& field_device_ptrs,
    size_t element_size,
    size_t old_count,
    const std::vector<uint32_t>& permutation
) {
    (void)field_device_ptrs;
    (void)element_size;
    (void)old_count;
    (void)permutation;
    
    // TODO: Launch scatter kernel to reorder fields
    // For now, this is a placeholder
    FL_LOG(WARN) << "applyPermutationToFields not yet implemented";
}

} // namespace hashmap
} // namespace fluidloom
