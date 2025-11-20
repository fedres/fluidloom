#pragma once

#include "fluidloom/core/hashmap/HashTable.h"
#include "fluidloom/core/hashmap/CompactionEngine.h"
#include "fluidloom/core/backend/IBackend.h"
#include <memory>
#include <vector>

namespace fluidloom {
namespace hashmap {

/**
 * @brief Manages hash table lifecycle and rebuilds
 * 
 * Orchestrates CPU-side operations for building and maintaining
 * the GPU-resident hash table.
 */
class HashTableManager {
public:
    explicit HashTableManager(IBackend* backend);
    ~HashTableManager();
    
    // Delete copy/move
    HashTableManager(const HashTableManager&) = delete;
    HashTableManager& operator=(const HashTableManager&) = delete;
    
    /**
     * @brief Rebuild hash table from cell list
     * 
     * @param hilbert_indices Sorted Hilbert indices
     * @param array_indices Corresponding SOA array indices
     * @return Rebuild time in milliseconds
     */
    double rebuild(const std::vector<uint64_t>& hilbert_indices,
                   const std::vector<uint32_t>& array_indices);
    
    /**
     * @brief Query hash table (CPU-side batch query for testing)
     * 
     * @param query_keys Hilbert indices to look up
     * @return Array indices (HASH_INVALID_VALUE if not found)
     */
    std::vector<uint32_t> query(const std::vector<uint64_t>& query_keys);
    
    /**
     * @brief Get device pointers for kernel use
     */
    void* getKeysDevicePtr() const { return table_keys_->getDevicePointer(); }
    void* getValuesDevicePtr() const { return table_values_->getDevicePointer(); }
    uint64_t getCapacity() const { return current_table_.capacity; }
    
    /**
     * @brief Get hash table metadata
     */
    const HashTable& getHashTable() const { return current_table_; }
    const HashTableMetadata& getMetadata() const { return metadata_; }
    
private:
    IBackend* backend_;
    std::unique_ptr<CompactionEngine> compactor_;
    
    // Current hash table
    HashTable current_table_;
    HashTableMetadata metadata_;
    
    // Device buffers
    DeviceBufferPtr table_keys_;
    DeviceBufferPtr table_values_;
    
    // Kernels (placeholders for future implementation)
    void* hash_build_kernel_;
    
    // Helpers
    void ensureKernelsCompiled();
    void allocateTable(size_t num_cells);
    void clearTable();
    uint64_t computeCapacity(size_t num_cells);
};

} // namespace hashmap
} // namespace fluidloom
