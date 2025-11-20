#include <gtest/gtest.h>
#include "fluidloom/halo/packers/HaloPackKernel.h"
#include "fluidloom/halo/packers/HaloUnpackKernel.h"
#include "fluidloom/core/backend/MockBackend.h"
#include "fluidloom/halo/GhostRange.h"
#include "fluidloom/halo/PackBufferLayout.h"

using namespace fluidloom;
using namespace fluidloom::halo;

class KernelTest : public ::testing::Test {
protected:
    void SetUp() override {
        backend = std::make_unique<MockBackend>();
        backend->initialize();
        pack_kernel = std::make_unique<HaloPackKernel>(backend.get());
        unpack_kernel = std::make_unique<HaloUnpackKernel>(backend.get());
    }

    void TearDown() override {
        backend->shutdown();
    }

    std::unique_ptr<MockBackend> backend;
    std::unique_ptr<HaloPackKernel> pack_kernel;
    std::unique_ptr<HaloUnpackKernel> unpack_kernel;
};

TEST_F(KernelTest, PackKernelInitialization) {
    EXPECT_NO_THROW(pack_kernel->initialize());
}

TEST_F(KernelTest, UnpackKernelInitialization) {
    EXPECT_NO_THROW(unpack_kernel->initialize());
}

TEST_F(KernelTest, PackKernelExecution) {
    pack_kernel->initialize();
    
    // Setup dummy buffers
    auto field_buffer = backend->allocateBuffer(1024);
    auto indices_buffer = backend->allocateBuffer(1024);
    auto levels_buffer = backend->allocateBuffer(1024);
    auto ranges_buffer = backend->allocateBuffer(1024);
    auto pack_buffer = backend->allocateBuffer(1024);
    auto params_buffer = backend->allocateBuffer(1024);
    
    GhostRange range;
    range.hilbert_start = 0;
    range.hilbert_end = 64;
    range.pack_offset = 0;
    range.pack_size_bytes = 64 * 4; // 64 cells * 4 bytes
    
    PackBufferLayout layout;
    layout.capacity_bytes = 1024;
    layout.cell_size_bytes = 4;
    layout.addField("test_field", 1, 4);
    
    // Helper to convert DeviceBuffer to Buffer
    auto toBuffer = [](DeviceBuffer* db) -> Buffer {
        return Buffer{db->getDevicePointer(), db->getSize(), nullptr};
    };

    // Let's fix the call properly
    Buffer field_b = toBuffer(field_buffer.get());
    Buffer indices_b = toBuffer(indices_buffer.get());
    Buffer levels_b = toBuffer(levels_buffer.get());
    Buffer ranges_b = toBuffer(ranges_buffer.get());
    Buffer pack_b = toBuffer(pack_buffer.get());
    Buffer params_b = toBuffer(params_buffer.get());
    
    EXPECT_NO_THROW(pack_kernel->execute(
        field_b,
        indices_b,
        levels_b,
        ranges_b,
        pack_b,
        params_b,
        0, // range_id
        0, // field_idx
        0, // component_idx
        64 // num_cells
    ));
}

TEST_F(KernelTest, UnpackKernelExecution) {
    unpack_kernel->initialize();
    
    // Setup dummy buffers
    auto field_buffer = backend->allocateBuffer(1024);
    auto indices_buffer = backend->allocateBuffer(1024);
    auto levels_buffer = backend->allocateBuffer(1024);
    auto ranges_buffer = backend->allocateBuffer(1024);
    auto unpack_buffer = backend->allocateBuffer(1024);
    auto params_buffer = backend->allocateBuffer(1024);
    
    GhostRange range;
    range.hilbert_start = 0;
    range.hilbert_end = 64;
    range.pack_offset = 0;
    range.pack_size_bytes = 64 * 4;
    
    PackBufferLayout layout;
    layout.capacity_bytes = 1024;
    layout.cell_size_bytes = 4;
    layout.addField("test_field", 1, 4);
    
    // Helper to convert DeviceBuffer to Buffer
    auto toBuffer = [](DeviceBuffer* db) -> Buffer {
        return Buffer{db->getDevicePointer(), db->getSize(), nullptr};
    };
    
    Buffer field_b = toBuffer(field_buffer.get());
    Buffer indices_b = toBuffer(indices_buffer.get());
    Buffer levels_b = toBuffer(levels_buffer.get());
    Buffer ranges_b = toBuffer(ranges_buffer.get());
    Buffer unpack_b = toBuffer(unpack_buffer.get());
    Buffer params_b = toBuffer(params_buffer.get());
    
    // Execute should not throw
    EXPECT_NO_THROW(unpack_kernel->execute(
        field_b, // Output for unpack? No, unpack writes to field_buffer?
        // HaloUnpackKernel::execute(Buffer& field_data, ...)
        // Yes, field_data is output.
        indices_b,
        levels_b,
        ranges_b,
        unpack_b,
        params_b,
        0, // range_id
        0, // field_idx
        0, // component_idx
        64 // num_cells
    ));
}
