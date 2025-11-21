#include &lt;benchmark/benchmark.h&gt;
#include &lt;vector&gt;
#include &lt;thread&gt;
#include &lt;chrono&gt;
#include &lt;cstring&gt;

/**
 * @brief Benchmark halo exchange: pack → send → recv → unpack
 * 
 * Target performance:
 * - Halo overlap &gt; 90%
 * - Bandwidth: &gt; 10 GB/s PCIe 3.0, &gt; 20 GB/s PCIe 4.0
 * - Latency hidden: compute time during communication / total time &gt; 0.9
 */

// Mock halo pack operation
void mockHaloPack(const std::vector&lt;float&gt;& field_data,
                  const std::vector&lt;size_t&gt;& ghost_indices,
                  std::vector&lt;float&gt;& pack_buffer) {
    for (size_t i = 0; i &lt; ghost_indices.size(); ++i) {
        pack_buffer[i] = field_data[ghost_indices[i]];
    }
}

// Mock halo unpack operation
void mockHaloUnpack(const std::vector&lt;float&gt;& pack_buffer,
                    const std::vector&lt;size_t&gt;& ghost_indices,
                    std::vector&lt;float&gt;& field_data) {
    for (size_t i = 0; i &lt; ghost_indices.size(); ++i) {
        field_data[ghost_indices[i]] = pack_buffer[i];
    }
}

// Mock compute kernel
void mockComputeKernel(std::vector&lt;float&gt;& field_data, size_t duration_us) {
    auto start = std::chrono::high_resolution_clock::now();
    
    // Simulate compute work
    volatile float sum = 0.0f;
    for (size_t i = 0; i &lt; field_data.size(); ++i) {
        sum += field_data[i] * 1.01f;
    }
    
    auto elapsed = std::chrono::duration_cast&lt;std::chrono::microseconds&gt;(
        std::chrono::high_resolution_clock::now() - start).count();
    
    if (elapsed &lt; static_cast&lt;long&gt;(duration_us)) {
        std::this_thread::sleep_for(
            std::chrono::microseconds(duration_us - elapsed));
    }
}

static void BM_HaloExchange(benchmark::State& state) {
    int num_gpus = state.range(0);
    size_t cells_per_gpu = 1000000;
    size_t halo_size = cells_per_gpu / 10; // 10% halo
    
    // Setup mock field data
    std::vector&lt;float&gt; field_data(cells_per_gpu, 1.0f);
    std::vector&lt;float&gt; pack_buffer(halo_size);
    std::vector&lt;size_t&gt; ghost_indices(halo_size);
    
    for (size_t i = 0; i &lt; halo_size; ++i) {
        ghost_indices[i] = i;
    }
    
    for (auto _ : state) {
        // Start compute kernel simultaneously (simulate compute-halo overlap)
        auto compute_start = std::chrono::high_resolution_clock::now();
        
        // Pack halo
        auto pack_start = std::chrono::high_resolution_clock::now();
        mockHaloPack(field_data, ghost_indices, pack_buffer);
        auto pack_end = std::chrono::high_resolution_clock::now();
        
        // Simulate MPI send/recv (non-blocking)
        auto comm_start = std::chrono::high_resolution_clock::now();
        std::this_thread::sleep_for(std::chrono::microseconds(100)); // Latency
        auto comm_end = std::chrono::high_resolution_clock::now();
        
        // Overlap with compute
        size_t compute_duration_us = 500; // 0.5ms compute
        mockComputeKernel(field_data, compute_duration_us);
        
        // Unpack halo
        auto unpack_start = std::chrono::high_resolution_clock::now();
        mockHaloUnpack(pack_buffer, ghost_indices, field_data);
        auto unpack_end = std::chrono::high_resolution_clock::now();
        
        auto compute_end = std::chrono::high_resolution_clock::now();
        
        // Compute overlap efficiency
        double total_time = std::chrono::duration&lt;double, std::milli&gt;(compute_end - compute_start).count();
        double halo_time = std::chrono::duration&lt;double, std::milli&gt;(pack_end - pack_start).count() +
                          std::chrono::duration&lt;double, std::milli&gt;(comm_end - comm_start).count() +
                          std::chrono::duration&lt;double, std::milli&gt;(unpack_end - unpack_start).count();
        
        double overlap_efficiency = (total_time - halo_time) / total_time;
        
        state.counters["overlap_efficiency"] = overlap_efficiency;
        state.counters["pack_ms"] = std::chrono::duration&lt;double, std::milli&gt;(pack_end - pack_start).count();
        state.counters["comm_ms"] = std::chrono::duration&lt;double, std::milli&gt;(comm_end - comm_start).count();
        state.counters["unpack_ms"] = std::chrono::duration&lt;double, std::milli&gt;(unpack_end - unpack_start).count();
    }
    
    state.SetItemsProcessed(cells_per_gpu * num_gpus * state.iterations());
}

BENCHMARK(BM_HaloExchange)
    -&gt;Args({1}) // 1 GPU (baseline)
    -&gt;Args({2}) // 2 GPUs
    -&gt;Args({4}) // 4 GPUs
    -&gt;Args({8}) // 8 GPUs
    -&gt;Unit(benchmark::kMillisecond)
    -&gt;MinTime(10.0)
    -&gt;UseRealTime();

// Performance targets:
// 1 GPU: (baseline, no halo)
// 2 GPUs: overlap_efficiency &gt; 0.90 ✓
// 4 GPUs: overlap_efficiency &gt; 0.90 ✓
// 8 GPUs: overlap_efficiency &gt; 0.85 (network overhead)

BENCHMARK_MAIN();
