#include <gtest/gtest.h>
#include "fluidloom/core/hashmap/HashTable.h"
#include "fluidloom/core/hashmap/CompactionEngine.h"
#include "fluidloom/core/hashmap/HashTableManager.h"
#include "fluidloom/core/backend/BackendFactory.h"
#include <vector>
#include <algorithm>
#include <random>

using namespace fluidloom;
using namespace fluidloom::hashmap;

class HashMapTest : public ::testing::Test {
protected:
    void SetUp() override {
        backend_ptr = BackendFactory::createBackend(BackendChoice::MOCK);
        backend = backend_ptr.get();
        backend->initialize();
    }

    void TearDown() override {
        backend->shutdown();
    }

    std::unique_ptr<IBackend> backend_ptr;
    IBackend* backend;
};

// Test HashTable data structure constants
TEST_F(HashMapTest, HashTableConstants) {
    EXPECT_EQ(HASH_EMPTY_KEY, 0xFFFFFFFFFFFFFFFFULL);
    EXPECT_EQ(HASH_INVALID_VALUE, 0xFFFFFFFFU);
    EXPECT_FLOAT_EQ(MAX_LOAD_FACTOR, 0.6f);
}

// Test HashTable hash function
TEST_F(HashMapTest, HashFunction) {
    HashTable table;
    table.capacity = 1024;  // Power of 2
    
    // Test that different keys produce different slots
    uint64_t key1 = 12345;
    uint64_t key2 = 54321;
    
    uint64_t slot1 = table.getSlot(key1);
    uint64_t slot2 = table.getSlot(key2);
    
    EXPECT_LT(slot1, table.capacity);
    EXPECT_LT(slot2, table.capacity);
    EXPECT_NE(slot1, slot2);  // Different keys should hash to different slots (usually)
}

// Test CompactionEngine with simple data
TEST_F(HashMapTest, CompactionEngineBasic) {
    CompactionEngine compactor(backend);
    
    // Create test data: 10 cells, only odd indices active
    std::vector<uint64_t> hilbert_indices(10);
    std::vector<uint32_t> cell_state(10);
    
    for (size_t i = 0; i < 10; ++i) {
        hilbert_indices[i] = i * 100;  // Arbitrary Hilbert indices
        cell_state[i] = (i % 2 == 1) ? 1 : 0;  // Odd indices active
    }
    
    auto result = compactor.compactAndPermute(hilbert_indices, cell_state);
    
    // Should have 5 valid cells (indices 1, 3, 5, 7, 9)
    EXPECT_EQ(result.num_valid_cells, 5);
    
    // Check permutation vector size
    EXPECT_EQ(result.permutation.size(), 10);
    
    // Active cells should have valid permutation entries
    EXPECT_NE(result.permutation[1], HASH_INVALID_VALUE);
    EXPECT_NE(result.permutation[3], HASH_INVALID_VALUE);
    EXPECT_NE(result.permutation[5], HASH_INVALID_VALUE);
    
    // Inactive cells should have invalid permutation
    EXPECT_EQ(result.permutation[0], HASH_INVALID_VALUE);
    EXPECT_EQ(result.permutation[2], HASH_INVALID_VALUE);
}

// Test CompactionEngine with all active cells
TEST_F(HashMapTest, CompactionEngineAllActive) {
    CompactionEngine compactor(backend);
    
    std::vector<uint64_t> hilbert_indices = {100, 50, 200, 25};
    std::vector<uint32_t> cell_state = {1, 1, 1, 1};
    
    auto result = compactor.compactAndPermute(hilbert_indices, cell_state);
    
    EXPECT_EQ(result.num_valid_cells, 4);
    
    // All cells should have valid permutations
    for (size_t i = 0; i < 4; ++i) {
        EXPECT_NE(result.permutation[i], HASH_INVALID_VALUE);
    }
}

// Test CompactionEngine with no active cells
TEST_F(HashMapTest, CompactionEngineNoActive) {
    CompactionEngine compactor(backend);
    
    std::vector<uint64_t> hilbert_indices = {100, 200, 300};
    std::vector<uint32_t> cell_state = {0, 0, 0};
    
    auto result = compactor.compactAndPermute(hilbert_indices, cell_state);
    
    EXPECT_EQ(result.num_valid_cells, 0);
    
    // All permutations should be invalid
    for (size_t i = 0; i < 3; ++i) {
        EXPECT_EQ(result.permutation[i], HASH_INVALID_VALUE);
    }
}

// Test HashTableManager creation
TEST_F(HashMapTest, HashTableManagerCreate) {
    HashTableManager manager(backend);
    
    const auto& table = manager.getHashTable();
    EXPECT_EQ(table.capacity, 0);  // Not yet allocated
    EXPECT_EQ(table.size, 0);
}

// Test HashTableManager rebuild with small dataset
TEST_F(HashMapTest, HashTableManagerRebuild) {
    HashTableManager manager(backend);
    
    // Create test data
    std::vector<uint64_t> hilbert_indices = {100, 200, 300, 400, 500};
    std::vector<uint32_t> array_indices = {0, 1, 2, 3, 4};
    
    double rebuild_time = manager.rebuild(hilbert_indices, array_indices);
    
    EXPECT_GT(rebuild_time, 0.0);  // Should take some time
    
    const auto& metadata = manager.getMetadata();
    EXPECT_EQ(metadata.num_cells, 5);
    EXPECT_GT(metadata.bytes_allocated, 0);
    
    const auto& table = manager.getHashTable();
    EXPECT_GT(table.capacity, 5);  // Capacity should be larger than num_cells
    EXPECT_TRUE((table.capacity & (table.capacity - 1)) == 0);  // Power of 2
}

// Test HashTableManager capacity calculation
TEST_F(HashMapTest, HashTableCapacityPowerOfTwo) {
    HashTableManager manager(backend);
    
    // Test various sizes
    std::vector<size_t> test_sizes = {10, 100, 1000, 10000};
    
    for (size_t size : test_sizes) {
        std::vector<uint64_t> hilbert_indices(size);
        std::vector<uint32_t> array_indices(size);
        
        // Fill with unique values
        for (size_t i = 0; i < size; ++i) {
            hilbert_indices[i] = i * 1000;
            array_indices[i] = static_cast<uint32_t>(i);
        }
        
        manager.rebuild(hilbert_indices, array_indices);
        
        const auto& table = manager.getHashTable();
        
        // Capacity must be power of 2
        EXPECT_TRUE(((table.capacity & (table.capacity - 1)) == 0));
        
        // Capacity should respect load factor
        EXPECT_GE(table.capacity, static_cast<uint64_t>(size / MAX_LOAD_FACTOR));
    }
}

// Test kernel compilation (using MockBackend)
TEST_F(HashMapTest, KernelCompilation) {
    // This tests that the kernel API works with MockBackend
    auto kernel = backend->compileKernel("dummy.cl", "dummy_kernel");
    
    EXPECT_NE(kernel.handle, nullptr);
    
    backend->releaseKernel(kernel);
}

// Test kernel launch
TEST_F(HashMapTest, KernelLaunch) {
    auto kernel = backend->compileKernel("dummy.cl", "test_kernel");
    
    std::vector<IBackend::KernelArg> args;  // Empty args for mock
    
    // Should not throw
    EXPECT_NO_THROW({
        backend->launchKernel(kernel, 1024, 64, args);
    });
    
    backend->releaseKernel(kernel);
}

// Performance test: Large compaction
TEST_F(HashMapTest, CompactionPerformance) {
    CompactionEngine compactor(backend);
    
    const size_t num_cells = 10000;
    std::vector<uint64_t> hilbert_indices(num_cells);
    std::vector<uint32_t> cell_state(num_cells);
    
    // Random distribution: ~50% active
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(0, 1);
    
    for (size_t i = 0; i < num_cells; ++i) {
        hilbert_indices[i] = i * 1000;
        cell_state[i] = dist(rng);
    }
    
    auto result = compactor.compactAndPermute(hilbert_indices, cell_state);
    
    // Should complete in reasonable time
    EXPECT_LT(result.compaction_time_ms, 1000.0);  // < 1 second
    
    // Roughly half should be active
    EXPECT_GT(result.num_valid_cells, num_cells / 3);
    EXPECT_LT(result.num_valid_cells, num_cells * 2 / 3);
}

// Test hash table metadata
TEST_F(HashMapTest, HashTableMetadata) {
    HashTableManager manager(backend);
    
    std::vector<uint64_t> hilbert_indices = {1, 2, 3, 4, 5};
    std::vector<uint32_t> array_indices = {0, 1, 2, 3, 4};
    
    manager.rebuild(hilbert_indices, array_indices);
    
    const auto& metadata = manager.getMetadata();
    
    EXPECT_EQ(metadata.num_cells, 5);
    EXPECT_GT(metadata.active_capacity, 0);
    EXPECT_GT(metadata.bytes_allocated, 0);
    EXPECT_GE(metadata.build_time_ms, 0);
}
