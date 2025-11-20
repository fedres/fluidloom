#include <gtest/gtest.h>
#include "fluidloom/adaptation/MergeEngine.h"
#include "fluidloom/adaptation/CellDescriptor.h"
#include <vector>

using namespace fluidloom;
using namespace fluidloom::adaptation;

class MergeEngineTest : public ::testing::Test {
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
        engine = std::make_unique<MergeEngine>(context, queue, config);
    }

    void TearDown() override {
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
    }

    cl_context context;
    cl_command_queue queue;
    AdaptationConfig config;
    std::unique_ptr<MergeEngine> engine;
};

TEST_F(MergeEngineTest, MergeSiblings) {
    // Setup 8 siblings at level 1
    size_t num_cells = 8;
    std::vector<int> h_x(8), h_y(8), h_z(8);
    std::vector<uint8_t> h_level(8, 1), h_state(8, 0);
    std::vector<int> h_flags(8, -1); // COARSEN
    std::vector<uint32_t> h_mat(8, 0);
    
    for(int i=0; i<8; ++i) {
        h_x[i] = (i & 1);
        h_y[i] = (i >> 1) & 1;
        h_z[i] = (i >> 2) & 1;
    }
    
    cl_int err;
    cl_mem x = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, 8*sizeof(int), h_x.data(), &err);
    cl_mem y = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, 8*sizeof(int), h_y.data(), &err);
    cl_mem z = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, 8*sizeof(int), h_z.data(), &err);
    cl_mem l = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, 8*sizeof(uint8_t), h_level.data(), &err);
    cl_mem s = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, 8*sizeof(uint8_t), h_state.data(), &err);
    cl_mem f = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, 8*sizeof(int), h_flags.data(), &err);
    cl_mem m = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, 8*sizeof(uint32_t), h_mat.data(), &err);
    
    MergeResult res = engine->merge(x, y, z, l, s, f, m, num_cells, nullptr, 0);
    
    EXPECT_TRUE(res.success);
    EXPECT_EQ(res.num_parents_created, 1);
    EXPECT_EQ(res.num_children_merged, 8);
    
    if (!res.parents.empty()) {
        EXPECT_EQ(res.parents[0].level, 0);
        EXPECT_EQ(res.parents[0].x, 0);
    }
    
    clReleaseMemObject(x); clReleaseMemObject(y); clReleaseMemObject(z);
    clReleaseMemObject(l); clReleaseMemObject(s); clReleaseMemObject(f); clReleaseMemObject(m);
}
