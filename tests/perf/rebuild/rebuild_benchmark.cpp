#include &lt;benchmark/benchmark.h&gt;
#include &lt;vector&gt;
#include &lt;algorithm&gt;
#include &lt;random&gt;
#include &lt;chrono&gt;

/**
 * @brief Benchmark hash rebuild pipeline: sort → compact → build
 * 
 * Target performance (from Plan.md Section 9):
 * - 1M cells: &lt; 10ms total
 * - Sort: &lt; 5ms
 * - Compact: &lt; 2ms
 * - Hash build: &lt; 3ms
 */

// Mock data generator
std::vector&lt;uint64_t&gt; generateRandomHilbertKeys(size_t num_cells, uint8_t max_level) {
    std::vector&lt;uint64_t&gt; keys(num_cells);
    std::mt19937_64 rng(42); // Fixed seed for reproducibility
    std::uniform_int_distribution&lt;uint64_t&gt; dist(0, (1ULL &lt;&lt; (max_level * 3)) - 1);
    
    for (size_t i = 0; i &lt; num_cells; ++i) {
        keys[i] = dist(rng);
    }
    
    return keys;
}

// Mock sort operation
std::vector&lt;uint64_t&gt; mockRadixSort(const std::vector&lt;uint64_t&gt;& keys) {
    std::vector&lt;uint64_t&gt; sorted = keys;
    std::sort(sorted.begin(), sorted.end());
    return sorted;
}

// Mock compaction
std::pair&lt;std::vector&lt;uint64_t&gt;, std::vector&lt;uint32_t&gt;&gt; mockCompact(
    const std::vector&lt;uint64_t&gt;& sorted_keys) {
    
    std::vector&lt;uint64_t&gt; dense_keys;
    std::vector&lt;uint32_t&gt; permutation;
    
    dense_keys.reserve(sorted_keys.size());
    permutation.reserve(sorted_keys.size());
    
    for (size_t i = 0; i &lt; sorted_keys.size(); ++i) {
        if (sorted_keys[i] != 0xFFFFFFFFFFFFFFFFULL) { // Not EMPTY
            dense_keys.push_back(sorted_keys[i]);
            permutation.push_back(static_cast&lt;uint32_t&gt;(i));
        }
    }
    
    return {dense_keys, permutation};
}

// Mock hash build
void mockHashBuild(const std::vector&lt;uint64_t&gt;& keys) {
    // Simulate hash table construction
    std::vector&lt;uint64_t&gt; hash_table(keys.size() * 2, 0xFFFFFFFFFFFFFFFFULL);
    
    for (size_t i = 0; i &lt; keys.size(); ++i) {
        uint64_t key = keys[i];
        size_t slot = key % hash_table.size();
        
        // Linear probing
        while (hash_table[slot] != 0xFFFFFFFFFFFFFFFFULL) {
            slot = (slot + 1) % hash_table.size();
        }
        hash_table[slot] = key;
    }
}

static void BM_RebuildPipeline(benchmark::State& state) {
    size_t num_cells = state.range(0);
    
    // Generate synthetic cell data
    auto hilbert_keys = generateRandomHilbertKeys(num_cells, 5);
    
    for (auto _ : state) {
        // Measure sort
        auto sort_start = std::chrono::high_resolution_clock::now();
        auto sorted_keys = mockRadixSort(hilbert_keys);
        auto sort_end = std::chrono::high_resolution_clock::now();
        
        // Measure compaction
        auto compact_start = std::chrono::high_resolution_clock::now();
        auto [dense_keys, permutation] = mockCompact(sorted_keys);
        auto compact_end = std::chrono::high_resolution_clock::now();
        
        // Measure hash build
        auto build_start = std::chrono::high_resolution_clock::now();
        mockHashBuild(dense_keys);
        auto build_end = std::chrono::high_resolution_clock::now();
        
        // Report sub-timings
        state.counters["sort_ms"] = std::chrono::duration&lt;double, std::milli&gt;(sort_end - sort_start).count();
        state.counters["compact_ms"] = std::chrono::duration&lt;double, std::milli&gt;(compact_end - compact_start).count();
        state.counters["build_ms"] = std::chrono::duration&lt;double, std::milli&gt;(build_end - build_start).count();
    }
    
    state.SetItemsProcessed(num_cells * state.iterations());
}

BENCHMARK(BM_RebuildPipeline)
    -&gt;Args({100000})    // 100K cells
    -&gt;Args({1000000})   // 1M cells (target)
    -&gt;Args({10000000})  // 10M cells (stress test)
    -&gt;Unit(benchmark::kMillisecond)
    -&gt;MinTime(5.0)
    -&gt;UseRealTime();

// Performance targets:
// 100K cells: &lt; 5ms
// 1M cells: &lt; 10ms ✓
// 10M cells: &lt; 100ms

BENCHMARK_MAIN();
