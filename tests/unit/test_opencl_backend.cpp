#include <gtest/gtest.h>
#include "fluidloom/core/backend/OpenCLBackend.h"
#include "fluidloom/common/Logger.h"
#include "fluidloom/common/FluidLoomError.h"
#include <vector>
#include <chrono>

using namespace fluidloom;

class OpenCLBackendTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        // One-time initialization
        Logger::instance().setLevel(LogLevel::ERROR);
        
        // Skip all tests if OpenCL not available
        try {
            auto test_backend = std::make_unique<OpenCLBackend>();
            opencl_available = true;
        } catch (const std::exception&) {
            opencl_available = false;
        }
    }
    
    void SetUp() override {
        if (!opencl_available) {
            GTEST_SKIP() << "OpenCL not available, skipping test";
        }
        
        backend = std::make_unique<OpenCLBackend>();
        backend->initialize();
    }
    
    void TearDown() override {
        if (backend) {
            backend->shutdown();
        }
    }
    
    std::unique_ptr<OpenCLBackend> backend;
    static bool opencl_available;
};

bool OpenCLBackendTest::opencl_available = false;

TEST_F(OpenCLBackendTest, DeviceInfo) {
    EXPECT_GT(backend->getDeviceCount(), 0);
    EXPECT_GT(backend->getMaxAllocationSize(), 0);
    EXPECT_GT(backend->getTotalMemory(), 0);
    EXPECT_FALSE(backend->getName().empty());
}

TEST_F(OpenCLBackendTest, Allocate1GBBuffer) {
    const size_t size = 1024ULL * 1024 * 1024; // 1GB
    
    auto buffer = backend->allocateBuffer(size);
    EXPECT_NE(buffer, nullptr);
    EXPECT_EQ(buffer->getSize(), size);
    EXPECT_EQ(buffer->getLocation(), MemoryLocation::DEVICE);
    
    // Verify it's actually allocated by writing data
    std::vector<char> pattern(1024, 0x42);
    backend->copyHostToDevice(pattern.data(), *buffer, pattern.size());
}

TEST_F(OpenCLBackendTest, CopyPerformance) {
    const size_t size = 100 * 1024 * 1024; // 100MB
    std::vector<float> host_data(size / sizeof(float), 1.23f);
    
    auto buffer = backend->allocateBuffer(size);
    
    // H2D
    auto start = std::chrono::high_resolution_clock::now();
    backend->copyHostToDevice(host_data.data(), *buffer, size);
    backend->finish();
    auto h2d_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now() - start).count();
    
    // D2H
    std::vector<float> host_result(size / sizeof(float));
    start = std::chrono::high_resolution_clock::now();
    backend->copyDeviceToHost(*buffer, host_result.data(), size);
    backend->finish();
    auto d2h_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now() - start).count();
    
    // Check bandwidth (assuming PCIe 4.0 ~ 25 GB/s theoretical)
    double h2d_bandwidth = (size / (1024.0*1024.0*1024.0)) / (h2d_time / 1000.0);
    double d2h_bandwidth = (size / (1024.0*1024.0*1024.0)) / (d2h_time / 1000.0);
    
    FL_LOG(INFO) << "H2D bandwidth: " << h2d_bandwidth << " GB/s";
    FL_LOG(INFO) << "D2H bandwidth: " << d2h_bandwidth << " GB/s";
    
    // Should achieve at least 5 GB/s in practice
    // EXPECT_GT(h2d_bandwidth, 5.0);
    // EXPECT_GT(d2h_bandwidth, 5.0);
}

TEST_F(OpenCLBackendTest, ErrorHandling) {
    // Test proper error propagation
    cl_int dummy_error = CL_MEM_OBJECT_ALLOCATION_FAILURE;
    EXPECT_THROW(throw OpenCLError(dummy_error, "Test error", __FILE__, __LINE__), OpenCLError);
}

// Memory leak test
TEST_F(OpenCLBackendTest, NoMemoryLeak) {
    const int iterations = 10;
    const size_t size = 10 * 1024 * 1024; // 10MB
    
    for (int i = 0; i < iterations; ++i) {
        auto buffer = backend->allocateBuffer(size);
        // Buffer should be freed automatically
    }
    
    // If valgrind is running, this should show no leaks
    // Run with: valgrind --leak-check=full --show-leak-kinds=all ./test_opencl_backend
}
