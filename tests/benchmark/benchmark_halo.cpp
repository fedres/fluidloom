#include <benchmark/benchmark.h>
#include "fluidloom/halo/packers/HaloPackKernel.h"
#include "fluidloom/halo/packers/HaloUnpackKernel.h"
#include "fluidloom/core/backend/MockBackend.h"
#include "fluidloom/halo/GhostRange.h"
#include "fluidloom/halo/PackBufferLayout.h"

using namespace fluidloom;
using namespace fluidloom::halo;

class HaloBenchmark : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        backend = std::make_unique<MockBackend>();
        backend->initialize();
        pack_kernel = std::make_unique<HaloPackKernel>(backend.get());
        unpack_kernel = std::make_unique<HaloUnpackKernel>(backend.get());
        
        pack_kernel->initialize();
        unpack_kernel->initialize();
        
        // Setup buffers
        size_t num_cells = state.range(0);
        field_buffer = backend->allocateBuffer(num_cells * 4);
        pack_buffer = backend->allocateBuffer(num_cells * 4);
        
        // Dummy auxiliary buffers
        indices_buffer = backend->allocateBuffer(num_cells * 4);
        levels_buffer = backend->allocateBuffer(num_cells * 4);
        ranges_buffer = backend->allocateBuffer(1024);
        params_buffer = backend->allocateBuffer(1024);
        
        // Setup layout
        layout.capacity_bytes = num_cells * 4;
        layout.cell_size_bytes = 4;
        layout.addField("test_field", 1, 4);
    }

    void TearDown(const ::benchmark::State& state) override {
        (void)state;
        backend->shutdown();
    }

    std::unique_ptr<MockBackend> backend;
    std::unique_ptr<HaloPackKernel> pack_kernel;
    std::unique_ptr<HaloUnpackKernel> unpack_kernel;
    
    DeviceBufferPtr field_buffer;
    DeviceBufferPtr pack_buffer;
    DeviceBufferPtr indices_buffer;
    DeviceBufferPtr levels_buffer;
    DeviceBufferPtr ranges_buffer;
    DeviceBufferPtr params_buffer;
    
    PackBufferLayout layout;
};

BENCHMARK_DEFINE_F(HaloBenchmark, PackKernel)(benchmark::State& state) {
    size_t num_cells = state.range(0);
    
    auto toBuffer = [](DeviceBuffer* db) -> Buffer {
        return Buffer{db->getDevicePointer(), db->getSize(), nullptr};
    };
    
    Buffer field_b = toBuffer(field_buffer.get());
    Buffer indices_b = toBuffer(indices_buffer.get());
    Buffer levels_b = toBuffer(levels_buffer.get());
    Buffer ranges_b = toBuffer(ranges_buffer.get());
    Buffer pack_b = toBuffer(pack_buffer.get());
    Buffer params_b = toBuffer(params_buffer.get());

    for (auto _ : state) {
        pack_kernel->execute(
            field_b,
            indices_b,
            levels_b,
            ranges_b,
            pack_b,
            params_b,
            0, 0, 0,
            num_cells
        );
    }
}

BENCHMARK_REGISTER_F(HaloBenchmark, PackKernel)->Range(1024, 1024*1024);

BENCHMARK_MAIN();
