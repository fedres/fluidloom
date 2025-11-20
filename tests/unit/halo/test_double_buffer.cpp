#include <gtest/gtest.h>
#include <gtest/gtest.h>
#include "fluidloom/halo/buffers/DoubleBufferController.h"
#include "fluidloom/core/backend/MockBackend.h"

using namespace fluidloom;
using namespace fluidloom::halo;

class DoubleBufferTest : public ::testing::Test {
protected:
    void SetUp() override {
        backend = std::make_unique<MockBackend>();
        backend->initialize();
        controller = std::make_unique<DoubleBufferController>(backend.get(), 10); // 10MB
    }
    
    std::unique_ptr<MockBackend> backend;
    std::unique_ptr<DoubleBufferController> controller;
};

TEST_F(DoubleBufferTest, Initialization) {
    EXPECT_EQ(controller->getCurrentCapacity(), 10 * 1024 * 1024);
    EXPECT_EQ(controller->getSwapCount(), 0);
    
    Buffer* pack = controller->getPackBuffer();
    Buffer* comm = controller->getCommBuffer();
    
    EXPECT_NE(pack, nullptr);
    EXPECT_NE(comm, nullptr);
    EXPECT_NE(pack->device_ptr, comm->device_ptr);
}

TEST_F(DoubleBufferTest, SwapBuffers) {
    Buffer* initial_pack = controller->getPackBuffer();
    Buffer* initial_comm = controller->getCommBuffer();
    
    controller->swap();
    
    EXPECT_EQ(controller->getSwapCount(), 1);
    EXPECT_EQ(controller->getPackBuffer()->device_ptr, initial_comm->device_ptr);
    EXPECT_EQ(controller->getCommBuffer()->device_ptr, initial_pack->device_ptr);
}

TEST_F(DoubleBufferTest, Resize) {
    controller->resize(20);
    EXPECT_EQ(controller->getCurrentCapacity(), 20 * 1024 * 1024);
    
    // Check that buffers were reallocated (mock backend handles are just pointers/ids)
    // In a real test we'd check if old pointers are invalid, but here we just check capacity
}
