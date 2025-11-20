#include <gtest/gtest.h>
#include "fluidloom/transport/MPITransport.h"
#include "fluidloom/transport/GPUAwareBuffer.h"
#include "fluidloom/core/backend/MockBackend.h"
#include <mpi.h>
#include <vector>
#include <cstring>

using namespace fluidloom;
using namespace fluidloom::transport;

class TwoGPUExchangeTest : public ::testing::Test {
protected:
    std::unique_ptr<MockBackend> backend;
    std::unique_ptr<MPITransport> transport;
    int rank;
    int size;
    
    void SetUp() override {
        // MPI should already be initialized by main()
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        MPI_Comm_size(MPI_COMM_WORLD, &size);
        
        // Ensure we have exactly 2 ranks
        ASSERT_EQ(size, 2) << "This test requires exactly 2 MPI ranks";
        
        backend = std::make_unique<MockBackend>();
        backend->initialize(0); // MockBackend only supports device_id 0
        
        transport = std::make_unique<MPITransport>(backend.get());
    }
    
    void TearDown() override {
        transport.reset();
        backend->shutdown();
    }
};

TEST_F(TwoGPUExchangeTest, SimpleExchange) {
    const size_t buffer_size = 1024; // 1KB
    
    // Create GPU-aware buffers
    auto send_buffer = createGPUAwareBuffer(backend.get(), buffer_size);
    auto recv_buffer = createGPUAwareBuffer(backend.get(), buffer_size);
    
    // Fill send buffer with rank-specific data
    std::vector<uint8_t> send_data(buffer_size, rank + 1);
    // In real implementation, we'd copy to device buffer
    // For mock, we just track the operation
    
    // Determine partner rank
    int partner_rank = (rank == 0) ? 1 : 0;
    
    // Post non-blocking send and receive
    auto send_req = transport->send_async(partner_rank, send_buffer.get(), 0, buffer_size, 0);
    auto recv_req = transport->recv_async(partner_rank, recv_buffer.get(), 0, buffer_size, 0);
    
    ASSERT_NE(send_req, nullptr);
    ASSERT_NE(recv_req, nullptr);
    
    // Wait for completion
    send_req->wait();
    recv_req->wait();
    
    // Verify buffers are ready
    EXPECT_TRUE(send_buffer->isReady());
    EXPECT_TRUE(recv_buffer->isReady());
    
    // Note: Stats tracking depends on MPI being properly initialized
    // In mock mode or single-rank mode, stats may not be updated
}

TEST_F(TwoGPUExchangeTest, AsymmetricExchange) {
    // Rank 0 sends 2KB, Rank 1 sends 1KB
    const size_t send_size = (rank == 0) ? 2048 : 1024;
    const size_t recv_size = (rank == 0) ? 1024 : 2048;
    
    auto send_buffer = createGPUAwareBuffer(backend.get(), send_size);
    auto recv_buffer = createGPUAwareBuffer(backend.get(), recv_size);
    
    int partner_rank = (rank == 0) ? 1 : 0;
    
    auto send_req = transport->send_async(partner_rank, send_buffer.get(), 0, send_size, 0);
    auto recv_req = transport->recv_async(partner_rank, recv_buffer.get(), 0, recv_size, 0);
    
    send_req->wait();
    recv_req->wait();
    
    EXPECT_TRUE(send_buffer->isReady());
    EXPECT_TRUE(recv_buffer->isReady());
}

TEST_F(TwoGPUExchangeTest, MultipleFields) {
    // Simulate exchanging 3 different fields
    const size_t field_size = 512;
    const int num_fields = 3;
    
    std::vector<std::unique_ptr<GPUAwareBuffer>> send_buffers;
    std::vector<std::unique_ptr<GPUAwareBuffer>> recv_buffers;
    std::vector<std::unique_ptr<MPIRequestWrapper>> requests;
    
    int partner_rank = (rank == 0) ? 1 : 0;
    
    // Create buffers and post operations for each field
    for (int i = 0; i < num_fields; ++i) {
        send_buffers.push_back(createGPUAwareBuffer(backend.get(), field_size));
        recv_buffers.push_back(createGPUAwareBuffer(backend.get(), field_size));
        
        requests.push_back(transport->send_async(partner_rank, send_buffers[i].get(), 0, field_size, i));
        requests.push_back(transport->recv_async(partner_rank, recv_buffers[i].get(), 0, field_size, i));
    }
    
    // Wait for all operations
    for (auto& req : requests) {
        req->wait();
    }
    
    // Verify all buffers are ready
    for (int i = 0; i < num_fields; ++i) {
        EXPECT_TRUE(send_buffers[i]->isReady());
        EXPECT_TRUE(recv_buffers[i]->isReady());
    }
}

TEST_F(TwoGPUExchangeTest, StressTest) {
    // Rapid successive exchanges
    const size_t buffer_size = 256;
    const int num_iterations = 10;
    
    auto send_buffer = createGPUAwareBuffer(backend.get(), buffer_size);
    auto recv_buffer = createGPUAwareBuffer(backend.get(), buffer_size);
    
    int partner_rank = (rank == 0) ? 1 : 0;
    
    for (int i = 0; i < num_iterations; ++i) {
        auto send_req = transport->send_async(partner_rank, send_buffer.get(), 0, buffer_size, i);
        auto recv_req = transport->recv_async(partner_rank, recv_buffer.get(), 0, buffer_size, i);
        
        send_req->wait();
        recv_req->wait();
        
        ASSERT_TRUE(send_buffer->isReady()) << "Iteration " << i;
        ASSERT_TRUE(recv_buffer->isReady()) << "Iteration " << i;
    }
    
    // Test passed if we got here without deadlock or errors
    SUCCEED();
}

// Main function to initialize MPI
int main(int argc, char** argv) {
    // Initialize MPI
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    
    if (provided != MPI_THREAD_MULTIPLE) {
        std::cerr << "Warning: MPI_THREAD_MULTIPLE not supported" << std::endl;
    }
    
    // Initialize Google Test
    ::testing::InitGoogleTest(&argc, argv);
    
    // Run tests
    int result = RUN_ALL_TESTS();
    
    // Finalize MPI
    MPI_Finalize();
    
    return result;
}
