#include <gtest/gtest.h>
#include "fluidloom/adaptation/BalanceEnforcer.h"
#include "fluidloom/adaptation/CellDescriptor.h"
#include <vector>

using namespace fluidloom;
using namespace fluidloom::adaptation;

class BalanceEnforcerTest : public ::testing::Test {
protected:
    void SetUp() override {
        cl_int err;
        cl_platform_id platform;
        clGetPlatformIDs(1, &platform, nullptr);
        cl_device_id device;
        clGetDeviceIDs(platform, CL_DEVICE_TYPE_DEFAULT, 1, &device, nullptr);
        context = clCreateContext(nullptr, 1, &device, nullptr, nullptr, &err);
        queue = clCreateCommandQueue(context, device, 0, &err);
        
        config.max_refinement_level = 8;
        config.enforce_2_1_balance = true;
        config.cascade_refinement = true;
        engine = std::make_unique<BalanceEnforcer>(context, queue, config);
    }

    void TearDown() override {
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
    }

    cl_context context;
    cl_command_queue queue;
    AdaptationConfig config;
    std::unique_ptr<BalanceEnforcer> engine;
};

TEST_F(BalanceEnforcerTest, DetectViolation) {
    // Setup 2 cells: Cell 0 at (0,0,0) L=0, Cell 1 at (2,0,0) L=2
    // Cell 1 is fine (size 1/4), Cell 0 is coarse (size 1).
    // Cell 1 covers [2,2.25]. Cell 0 covers [0,1].
    // Distance is 1. They are not adjacent?
    // Wait, coordinates are integers.
    // L=0: unit=1. Range [0,1).
    // L=2: unit=0.25. Range [0.5, 0.5625).
    // Let's use integer coords.
    // Cell 0: x=0, L=0. Covers [0, 1) in normalized space.
    // Cell 1: x=4, L=2. (4 * 2^-2 = 1.0). Covers [1.0, 1.25).
    // They are adjacent!
    // L=0 cell at x=0 is adjacent to L=2 cell at x=4 (in L=2 units).
    // Coords stored are "global integer coordinates at cell's native resolution".
    // Cell 0: x=0, L=0.
    // Cell 1: x=4, L=2.
    // Neighbor check:
    // Cell 0 neighbor +x is x=1 (at L=0).
    // Cell 1 neighbor -x is x=3 (at L=2).
    // Are they adjacent?
    // Cell 0 boundary is x=1.
    // Cell 1 boundary is x=4/4 = 1.
    // Yes.
    // Level diff is 2. Violation!
    
    size_t num_cells = 2;
    std::vector<int> h_x = {0, 128};
    std::vector<int> h_y = {0, 0};
    std::vector<int> h_z = {0, 0};
    std::vector<uint8_t> h_level = {1, 3};
    std::vector<uint8_t> h_state = {0, 0};
    std::vector<int> h_flags = {0, 0};
    
    cl_int err;
    cl_mem x = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, 2*sizeof(int), h_x.data(), &err);
    cl_mem y = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, 2*sizeof(int), h_y.data(), &err);
    cl_mem z = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, 2*sizeof(int), h_z.data(), &err);
    cl_mem l = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, 2*sizeof(uint8_t), h_level.data(), &err);
    cl_mem s = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, 2*sizeof(uint8_t), h_state.data(), &err);
    cl_mem f = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, 2*sizeof(int), h_flags.data(), &err);
    
    BalanceResult res = engine->enforce(x, y, z, l, s, f, num_cells);
    
    EXPECT_TRUE(res.converged);
    EXPECT_GT(res.total_violations_detected, 0);
    EXPECT_GT(res.total_cells_marked_for_balance, 0);
    
    // Check if Cell 0 was marked
    std::vector<int> out_flags(2);
    clEnqueueReadBuffer(queue, f, CL_TRUE, 0, 2*sizeof(int), out_flags.data(), 0, nullptr, nullptr);
    
    EXPECT_EQ(out_flags[0], 1); // Should be marked for split
    EXPECT_EQ(out_flags[1], 0);
    
    clReleaseMemObject(x); clReleaseMemObject(y); clReleaseMemObject(z);
    clReleaseMemObject(l); clReleaseMemObject(s); clReleaseMemObject(f);
}
