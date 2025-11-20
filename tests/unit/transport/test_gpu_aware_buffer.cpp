#include <gtest/gtest.h>
#include "fluidloom/transport/GPUAwareBuffer.h"
#include "fluidloom/core/backend/MockBackend.h"

using namespace fluidloom;
using namespace fluidloom::transport;

class GPUAwareBufferTest : public ::testing::Test {
protected:
    std::unique_ptr<MockBackend> backend;
    
    void SetUp() override {
        backend = std::make_unique<MockBackend>();
        backend->initialize();
    }
    
    void TearDown() override {
        backend->shutdown();
    }
};

TEST_F(GPUAwareBufferTest, Allocation) {
    auto buffer = createGPUAwareBuffer(backend.get(), 1024 * 1024); // 1MB
    
    EXPECT_NE(buffer->getCLMem(), nullptr);
    EXPECT_GE(buffer->size_bytes, 1024 * 1024);
    EXPECT_TRUE(buffer->isReady());
    EXPECT_FALSE(buffer->is_bound_to_mpi);
}

TEST_F(GPUAwareBufferTest, ReferenceCounting) {
    auto buffer = createGPUAwareBuffer(backend.get(), 1024);
    
    buffer->markBound();
    EXPECT_FALSE(buffer->isReady());
    EXPECT_EQ(buffer->ref_count.load(), 1);
    
    buffer->markUnbound();
    EXPECT_TRUE(buffer->isReady());
    EXPECT_EQ(buffer->ref_count.load(), 0);
}

TEST_F(GPUAwareBufferTest, ThreadSafety) {
    auto buffer = createGPUAwareBuffer(backend.get(), 1024);
    
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&buffer]() {
            buffer->markBound();
            // Reduced sleep to make test faster
            std::this_thread::sleep_for(std::chrono::microseconds(1));
            buffer->markUnbound();
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // Wait for all markUnbound operations to complete
    // The atomic operations should be immediate after join, but give a small buffer
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    EXPECT_TRUE(buffer->isReady());
    EXPECT_EQ(buffer->ref_count.load(), 0);
}
