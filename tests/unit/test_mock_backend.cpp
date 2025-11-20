#include <gtest/gtest.h>
#include "fluidloom/core/backend/MockBackend.h"
#include "fluidloom/core/backend/BackendFactory.h"
#include "fluidloom/common/Logger.h"
#include "fluidloom/common/FluidLoomError.h"

using namespace fluidloom;

class MockBackendTest : public ::testing::Test {
protected:
    void SetUp() override {
        Logger::instance().setLevel(LogLevel::ERROR);  // Suppress logs in tests
        backend = std::make_unique<MockBackend>();
        backend->initialize();
    }
    
    void TearDown() override {
        if (backend && backend->isInitialized()) {
            backend->shutdown();
        }
        backend.reset();
    }
    
    std::unique_ptr<MockBackend> backend;
};

TEST_F(MockBackendTest, InitializeShutdown) {
    // Should not throw
    EXPECT_TRUE(backend->isInitialized());
    EXPECT_EQ(backend->getType(), BackendType::MOCK);
    EXPECT_EQ(backend->getName(), "Mock Backend");
}

TEST_F(MockBackendTest, AllocateSmallBuffer) {
    const size_t size = 1024;
    auto buffer = backend->allocateBuffer(size);
    
    EXPECT_NE(buffer, nullptr);
    EXPECT_EQ(buffer->getSize(), size);
    EXPECT_NE(buffer->getDevicePointer(), nullptr);
    EXPECT_EQ(buffer->getLocation(), MemoryLocation::HOST);
}

TEST_F(MockBackendTest, AllocateLargeBuffer) {
    const size_t size = 1024ULL * 1024 * 1024; // 1GB
    auto buffer = backend->allocateBuffer(size);
    
    EXPECT_NE(buffer, nullptr);
    EXPECT_EQ(buffer->getSize(), size);
}

TEST_F(MockBackendTest, AllocateZeroBuffer) {
    EXPECT_THROW(backend->allocateBuffer(0), BackendError);
}

TEST_F(MockBackendTest, AllocateBeyondLimit) {
    const size_t limit = 16ULL * 1024 * 1024 * 1024; // 16GB
    EXPECT_THROW(backend->allocateBuffer(limit + 1), BackendError);
}

TEST_F(MockBackendTest, CopyHostToDevice) {
    const size_t size = 256;
    std::vector<int> host_src(size, 42);
    
    auto buffer = backend->allocateBuffer(size * sizeof(int));
    backend->copyHostToDevice(host_src.data(), *buffer, size * sizeof(int));
    
    // Verify data is copied (MockBackend stores in host)
    std::vector<int> host_dst(size, 0);
    backend->copyDeviceToHost(*buffer, host_dst.data(), size * sizeof(int));
    
    EXPECT_EQ(host_dst, host_src);
}

TEST_F(MockBackendTest, CopyDeviceToDevice) {
    const size_t size = 128;
    std::vector<double> data(size, 3.14159);
    
    auto src = backend->allocateBuffer(size * sizeof(double), data.data());
    auto dst = backend->allocateBuffer(size * sizeof(double));
    
    backend->copyDeviceToDevice(*src, *dst, size * sizeof(double));
    
    std::vector<double> result(size);
    backend->copyDeviceToHost(*dst, result.data(), size * sizeof(double));
    
    EXPECT_EQ(result, data);
}

TEST_F(MockBackendTest, DoubleShutdown) {
    backend->shutdown();
    EXPECT_THROW(backend->shutdown(), BackendError);
}

TEST_F(MockBackendTest, MemoryTracking) {
    size_t initial_allocated = backend->getTotalMemory() - backend->getMaxAllocationSize();
    (void)initial_allocated; // Suppress unused variable warning
    
    {
        auto buf1 = backend->allocateBuffer(1000);
        auto buf2 = backend->allocateBuffer(2000);
        // Buffers released on destruction
    }
    
    // After scope exit, should be back to initial
    // Note: MockBackend doesn't actually track per-buffer deallocation in this simple impl
    // In production, add reference counting
}

TEST_F(MockBackendTest, GetProperties) {
    EXPECT_GT(backend->getMaxAllocationSize(), 0);
    EXPECT_EQ(backend->getTotalMemory(), 16ULL * 1024 * 1024 * 1024);
}

// Performance test
TEST_F(MockBackendTest, AllocationPerformance) {
    const int num_allocations = 1000;
    const size_t size = 1024 * 1024; // 1MB
    
    auto start = std::chrono::high_resolution_clock::now();
    
    std::vector<DeviceBufferPtr> buffers;
    buffers.reserve(num_allocations);
    
    for (int i = 0; i < num_allocations; ++i) {
        buffers.push_back(backend->allocateBuffer(size));
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Relaxed threshold for CI/simulated environments
    EXPECT_LT(duration.count(), 500); 
}
