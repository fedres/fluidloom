#include <gtest/gtest.h>
#include "fluidloom/core/backend/OpenCLBackend.h"
#include "fluidloom/halo/HaloExchanger.h"
#include "fluidloom/core/registry/FieldRegistry.h"
#include "fluidloom/core/fields/SOAFieldManager.h"
#include <vector>
#include <random>

using namespace fluidloom;
using namespace fluidloom::halo;

// Testable subclass to expose protected members
class TestableHaloExchanger : public HaloExchanger {
public:
    using HaloExchanger::HaloExchanger;
    
    void addNeighbor(int rank, size_t num_cells) {
        NeighborBuffers buffers;
        // buffers.layout.num_cells = num_cells; // Removed
        
        // Add fields from registry
        auto& registry = registry::FieldRegistry::instance();
        auto field_names = registry.getAllNames();
        for (const auto& name : field_names) {
            auto desc = registry.lookupByName(name);
            if (desc) {
                buffers.layout.addField(name, desc->num_components, desc->bytesPerCell() / desc->num_components);
            }
        }
        
        buffers.layout.capacity_bytes = num_cells * buffers.layout.cell_size_bytes;
        size_t buffer_size = buffers.layout.capacity_bytes;
        buffers.send_buffer_host.resize(buffer_size);
        buffers.recv_buffer_host.resize(buffer_size);
        buffers.send_buffer_device = m_backend->allocateBuffer(buffer_size);
        buffers.recv_buffer_device = m_backend->allocateBuffer(buffer_size);
        
        m_neighbor_buffers[rank] = std::move(buffers);
    }
    
    void testPack(int rank) {
        packData(rank);
    }
    
    void testUnpack(int rank) {
        unpackData(rank);
    }
    
    const NeighborBuffers& getBuffers(int rank) {
        return m_neighbor_buffers.at(rank);
    }
};

class HaloExchangerTest : public ::testing::Test {
protected:
    void SetUp() override {
        backend = std::make_shared<OpenCLBackend>();
        backend->initialize(0);
        
        // Compile kernels
        try {
            pack_kernel = backend->compileKernel("kernels/halo_pack.cl", "pack_fields");
            unpack_kernel = backend->compileKernel("kernels/halo_pack.cl", "unpack_fields");
        } catch (const std::exception& e) {
            FAIL() << "Kernel compilation failed: " << e.what();
        }
    }

    void TearDown() override {
        if (backend) {
            if (pack_kernel.handle) backend->releaseKernel(pack_kernel);
            if (unpack_kernel.handle) backend->releaseKernel(unpack_kernel);
            backend->shutdown();
        }
    }

    std::shared_ptr<OpenCLBackend> backend;
    IBackend::KernelHandle pack_kernel{nullptr};
    IBackend::KernelHandle unpack_kernel{nullptr};
};

TEST_F(HaloExchangerTest, PackUnpackFloat) {
    const size_t num_cells = 1024;
    const size_t data_size = num_cells * sizeof(float);
    
    // Host data
    std::vector<float> host_input(num_cells);
    std::vector<float> host_output(num_cells);
    std::vector<uint8_t> host_packed(data_size);
    
    // Initialize input
    for (size_t i = 0; i < num_cells; ++i) {
        host_input[i] = static_cast<float>(i) * 1.5f;
    }
    
    // Device buffers
    auto input_buf = backend->allocateBuffer(data_size);
    auto packed_buf = backend->allocateBuffer(data_size);
    auto output_buf = backend->allocateBuffer(data_size);
    
    // Copy input to device
    backend->copyHostToDevice(host_input.data(), *input_buf, data_size);
    
    // Pack
    backend->launchKernel(pack_kernel, num_cells, 0, {
        IBackend::KernelArg::fromBuffer(input_buf->getDevicePointer()),
        IBackend::KernelArg::fromBuffer(packed_buf->getDevicePointer()),
        IBackend::KernelArg::fromScalar((cl_ulong)num_cells),
        IBackend::KernelArg::fromScalar((cl_ulong)0) // offset 0
    });
    
    // Unpack
    backend->launchKernel(unpack_kernel, num_cells, 0, {
        IBackend::KernelArg::fromBuffer(packed_buf->getDevicePointer()),
        IBackend::KernelArg::fromBuffer(output_buf->getDevicePointer()),
        IBackend::KernelArg::fromScalar((cl_ulong)num_cells),
        IBackend::KernelArg::fromScalar((cl_ulong)0) // offset 0
    });
    
    // Read back
    backend->copyDeviceToHost(*output_buf, host_output.data(), data_size);
    
    // Verify
    for (size_t i = 0; i < num_cells; ++i) {
        EXPECT_FLOAT_EQ(host_input[i], host_output[i]) << "Mismatch at index " << i;
    }
}

TEST_F(HaloExchangerTest, IntegrationWithFieldManager) {
    // 1. Register a field
    fields::FieldDescriptor desc("density", fields::FieldType::FLOAT32, 1);
    
    auto& registry = registry::FieldRegistry::instance();
    if (!registry.exists("density")) {
        registry.registerField(desc);
    }
    
    // 2. Allocate field
    auto field_manager = std::make_shared<fields::SOAFieldManager>(backend.get());
    size_t num_cells = 1024;
    auto handle = field_manager->allocate(desc, num_cells);
    
    // Initialize field data (optional, just ensuring allocation works)
    
    auto ghost_builder = std::make_shared<GhostRangeBuilder>();
    
    TestableHaloExchanger exchanger(backend, field_manager, ghost_builder);
    
    // 3. Add neighbor manually
    exchanger.addNeighbor(1, num_cells);
    
    // 4. Test Pack
    EXPECT_NO_THROW(exchanger.testPack(1));
    
    // 5. Test Unpack
    EXPECT_NO_THROW(exchanger.testUnpack(1));
    
    // Cleanup
    field_manager->deallocate(handle);
}
