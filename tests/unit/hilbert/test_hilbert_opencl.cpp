#include <gtest/gtest.h>
#include "fluidloom/core/backend/OpenCLBackend.h"
#include "fluidloom/core/hilbert/HilbertCodec.h"
#include <random>
#include <vector>

using namespace fluidloom;

class HilbertOpenCLTest : public ::testing::Test {
protected:
    void SetUp() override {
        backend = std::make_unique<OpenCLBackend>();
        backend->initialize(0);
        
        // Compile kernels
        try {
            encode_kernel = backend->compileKernel(
                "kernels/hilbert.cl",
                "test_hilbert_encode"
            );
            
            decode_kernel = backend->compileKernel(
                "kernels/hilbert.cl",
                "test_hilbert_decode"
            );
        } catch (const std::exception& e) {
            FAIL() << "Kernel compilation failed: " << e.what();
        }
    }

    void TearDown() override {
        if (backend) {
            if (encode_kernel.handle) backend->releaseKernel(encode_kernel);
            if (decode_kernel.handle) backend->releaseKernel(decode_kernel);
            backend->shutdown();
        }
    }

    std::unique_ptr<OpenCLBackend> backend;
    IBackend::KernelHandle encode_kernel{nullptr};
    IBackend::KernelHandle decode_kernel{nullptr};
};

TEST_F(HilbertOpenCLTest, EncodeCorrectness) {
    const size_t num_points = 1000;
    const uint8_t level = 8; // Max level
    
    // Generate random coordinates
    std::vector<cl_int3> host_coords(num_points);
    std::vector<hilbert::HilbertIndex> host_expected(num_points);
    
    std::mt19937 gen(42);
    std::uniform_int_distribution<int> dist(0, (1 << level) - 1);
    
    for (size_t i = 0; i < num_points; ++i) {
        host_coords[i].s[0] = dist(gen);
        host_coords[i].s[1] = dist(gen);
        host_coords[i].s[2] = dist(gen);
        host_expected[i] = hilbert::encode(host_coords[i].s[0], host_coords[i].s[1], host_coords[i].s[2], level);
    }
    
    // Allocate device buffers
    auto coords_buf = backend->allocateBuffer(num_points * sizeof(cl_int3));
    auto results_buf = backend->allocateBuffer(num_points * sizeof(cl_ulong));
    
    // Copy input
    backend->copyHostToDevice(host_coords.data(), *coords_buf, num_points * sizeof(cl_int3));
    
    // Launch kernel
    std::vector<IBackend::KernelArg> args = {
        IBackend::KernelArg::fromBuffer(coords_buf->getDevicePointer()),
        IBackend::KernelArg::fromBuffer(results_buf->getDevicePointer()),
        IBackend::KernelArg::fromScalar(num_points),
        IBackend::KernelArg::fromScalar((cl_uchar)level)
    };
    
    backend->launchKernel(encode_kernel, num_points, 0, args);
    
    // Read back
    std::vector<hilbert::HilbertIndex> host_actual(num_points);
    backend->copyDeviceToHost(*results_buf, host_actual.data(), num_points * sizeof(cl_ulong));
    
    // Verify
    for (size_t i = 0; i < num_points; ++i) {
        EXPECT_EQ(host_actual[i], host_expected[i]) << "Mismatch at index " << i;
    }
}

TEST_F(HilbertOpenCLTest, DecodeCorrectness) {
    const size_t num_points = 1000;
    const uint8_t level = 8;
    
    std::vector<hilbert::HilbertIndex> host_input(num_points);
    std::vector<cl_int3> host_expected(num_points);
    
    std::mt19937 gen(42);
    std::uniform_int_distribution<int> dist(0, (1 << level) - 1);
    
    for (size_t i = 0; i < num_points; ++i) {
        int x = dist(gen);
        int y = dist(gen);
        int z = dist(gen);
        host_input[i] = hilbert::encode(x, y, z, level);
        host_expected[i].s[0] = x;
        host_expected[i].s[1] = y;
        host_expected[i].s[2] = z;
    }
    
    auto input_buf = backend->allocateBuffer(num_points * sizeof(cl_ulong));
    auto output_buf = backend->allocateBuffer(num_points * sizeof(cl_int3));
    
    backend->copyHostToDevice(host_input.data(), *input_buf, num_points * sizeof(cl_ulong));
    
    backend->launchKernel(decode_kernel, num_points, 0, {
        IBackend::KernelArg::fromBuffer(input_buf->getDevicePointer()),
        IBackend::KernelArg::fromBuffer(output_buf->getDevicePointer()),
        IBackend::KernelArg::fromScalar(num_points),
        IBackend::KernelArg::fromScalar((cl_uchar)level)
    });
    
    std::vector<cl_int3> host_actual(num_points);
    backend->copyDeviceToHost(*output_buf, host_actual.data(), num_points * sizeof(cl_int3));
    
    for (size_t i = 0; i < num_points; ++i) {
        EXPECT_EQ(host_actual[i].s[0], host_expected[i].s[0]);
        EXPECT_EQ(host_actual[i].s[1], host_expected[i].s[1]);
        EXPECT_EQ(host_actual[i].s[2], host_expected[i].s[2]);
    }
}
