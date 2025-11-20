#include <gtest/gtest.h>
#include "fluidloom/transport/MPITransport.h"
#include "fluidloom/core/backend/MockBackend.h"

// Only use mock MPI if real MPI is not available
#ifndef FLUIDLOOM_MPI_ENABLED
#define FLUIDLOOM_MOCK_MPI
#include "../../../tests/mock/mpi/mock_mpi.h"

// Define globals for mock MPI
int mock_mpi_rank = 0;
int mock_mpi_size = 1;
std::unordered_map<MPI_Request, bool> mock_mpi_request_complete;
#endif

using namespace fluidloom;
using namespace fluidloom::transport;

class MPITransportTest : public ::testing::Test {
protected:
    std::unique_ptr<MockBackend> backend;
    std::unique_ptr<MPITransport> transport;
    
    void SetUp() override {
        backend = std::make_unique<MockBackend>();
        backend->initialize();
        transport = std::make_unique<MPITransport>(backend.get());
    }
    
    void TearDown() override {
        transport.reset();
        backend->shutdown();
    }
};

TEST_F(MPITransportTest, Initialization) {
    EXPECT_EQ(transport->getRank(), 0);
    EXPECT_EQ(transport->getSize(), 1);
}

TEST_F(MPITransportTest, SendRecvAsync) {
    auto buffer = createGPUAwareBuffer(backend.get(), 1024);
    
    // Send
    auto send_req = transport->send_async(0, buffer.get(), 0, 1024, 0);
    EXPECT_NE(send_req, nullptr);
    EXPECT_FALSE(buffer->isReady()); // Should be bound
    
    // Wait
    send_req->wait();
    EXPECT_TRUE(buffer->isReady()); // Should be unbound
    
    // Recv
    auto recv_req = transport->recv_async(0, buffer.get(), 0, 1024, 0);
    EXPECT_NE(recv_req, nullptr);
    EXPECT_FALSE(buffer->isReady());
    
    recv_req->wait();
    EXPECT_TRUE(buffer->isReady());
}

TEST_F(MPITransportTest, Stats) {
    auto buffer = createGPUAwareBuffer(backend.get(), 1024);
    
    transport->send_async(0, buffer.get(), 0, 1024, 0)->wait();
    
    auto stats = transport->getStats();
    EXPECT_EQ(stats.num_messages_sent, 1);
    EXPECT_EQ(stats.bytes_sent, 1024);
}
