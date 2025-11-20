#include <gtest/gtest.h>
#include "fluidloom/adaptation/SplitEngine.h"
#include "fluidloom/adaptation/CellDescriptor.h"
#include <vector>

using namespace fluidloom;
using namespace fluidloom::adaptation;

class SplitEngineTest : public ::testing::Test {
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
        engine = std::make_unique<SplitEngine>(context, queue, config);
    }

    void TearDown() override {
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
    }

    cl_context context;
    cl_command_queue queue;
    AdaptationConfig config;
    std::unique_ptr<SplitEngine> engine;
};

TEST_F(SplitEngineTest, SplitSingleCell) {
    std::vector<int> h_x = {0}, h_y = {0}, h_z = {0};
    std::vector<uint8_t> h_level = {0}, h_state = {0};
    std::vector<int> h_flags = {1}; // SPLIT
    std::vector<uint32_t> h_mat = {0};
    size_t num_cells = 1;
    
    cl_int err;
    cl_mem x = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(int), h_x.data(), &err);
    cl_mem y = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(int), h_y.data(), &err);
    cl_mem z = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(int), h_z.data(), &err);
    cl_mem l = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(uint8_t), h_level.data(), &err);
    cl_mem s = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(uint8_t), h_state.data(), &err);
    cl_mem f = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(int), h_flags.data(), &err);
    cl_mem m = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(uint32_t), h_mat.data(), &err);
    
    SplitResult res = engine->split(x, y, z, l, s, f, m, num_cells, nullptr, 0);
    
    EXPECT_TRUE(res.success);
    EXPECT_EQ(res.num_children, 8);
    EXPECT_EQ(res.num_parents_split, 1);
    
    for(const auto& child : res.children) {
        EXPECT_EQ(child.level, 1);
    }
    
    clReleaseMemObject(x); clReleaseMemObject(y); clReleaseMemObject(z);
    clReleaseMemObject(l); clReleaseMemObject(s); clReleaseMemObject(f); clReleaseMemObject(m);
}
