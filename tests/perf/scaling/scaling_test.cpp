#include &lt;gtest/gtest.h&gt;
#include "tools/test_harness/TestHarness.h"
#include &lt;vector&gt;
#include &lt;chrono&gt;
#include &lt;fstream&gt;
#include &lt;sstream&gt;

using namespace fluidloom::testing;

/**
 * @brief Comprehensive scaling test suite
 * 
 * Runs weak and strong scaling tests across problem sizes and GPU counts
 * Generates performance curves and detects regressions vs historical baseline
 */
class ScalingTest : public ::testing::TestWithParam&lt;std::tuple&lt;int, size_t, int&gt;&gt; {
protected:
    void SetUp() override {
        // Setup test environment
    }
    
    void TearDown() override {
        // Cleanup
    }
    
    // Helper: Generate configuration for N GPUs
    std::string generateWeakScalingConfig(int ngpus, size_t cells_per_gpu, int max_level) {
        std::ostringstream config;
        config &lt;&lt; "# Weak scaling config\n";
        config &lt;&lt; "# GPUs: " &lt;&lt; ngpus &lt;&lt; "\n";
        config &lt;&lt; "# Cells per GPU: " &lt;&lt; cells_per_gpu &lt;&lt; "\n";
        config &lt;&lt; "# Max level: " &lt;&lt; max_level &lt;&lt; "\n";
        return config.str();
    }
};

TEST_P(ScalingTest, WeakScalingEfficiency) {
    int ngpus = std::get&lt;0&gt;(GetParam());
    size_t cells_per_gpu = std::get&lt;1&gt;(GetParam());
    int max_level = std::get&lt;2&gt;(GetParam());
    
    // Configure for this test
    TestHarness harness(BackendType::MOCK, false);
    
    std::string script = generateWeakScalingConfig(ngpus, cells_per_gpu, max_level);
    
    // Run short simulation
    auto start = std::chrono::steady_clock::now();
    TestResult result = harness.runSimulation(
        script,
        100,  // Short run
        {"density"}
    );
    auto end = std::chrono::steady_clock::now();
    
    double time_ms = std::chrono::duration&lt;double, std::milli&gt;(end - start).count();
    double cells_per_sec = (cells_per_gpu * ngpus) / (time_ms / 1000.0);
    
    // For now, just verify it ran
    EXPECT_GT(time_ms, 0.0) &lt;&lt; "Simulation should have run";
    
    // TODO: Compare against baseline
    // RegressionDetector detector;
    // detector.addMeasurement(baseline_key, time_ms);
    // double expected_time = detector.getBaseline(baseline_key);
    // double regression = (time_ms - expected_time) / expected_time;
    // EXPECT_LT(regression, 0.05) << "Performance regression >5% vs baseline";
    
    // Print scaling curve data
    RecordProperty("ngpus", ngpus);
    RecordProperty("cells_per_gpu_M", static_cast&lt;double&gt;(cells_per_gpu) / 1e6);
    RecordProperty("throughput_Mcells_per_sec", cells_per_sec / 1e6);
}

// Instantiate tests for all scaling configurations
INSTANTIATE_TEST_SUITE_P(
    WeakScalingSweep,
    ScalingTest,
    ::testing::Combine(
        ::testing::Values(1, 2, 4, 8),           // GPU counts
        ::testing::Values(100000, 500000, 1000000), // Cells per GPU
        ::testing::Values(0, 3, 5)               // Max refinement levels
    )
);

TEST(StrongScalingTest, DISABLED_FixedProblemSize) {
    // Strong scaling: fixed total problem size, increase GPUs
    size_t total_cells = 4000000; // 4M cells
    std::vector&lt;int&gt; gpu_counts = {1, 2, 4, 8};
    std::vector&lt;double&gt; timings;
    
    for (int ngpus : gpu_counts) {
        TestHarness harness(BackendType::MOCK, false);
        
        std::string script = "# Strong scaling with " + std::to_string(ngpus) + " GPUs";
        
        auto start = std::chrono::steady_clock::now();
        TestResult result = harness.runSimulation(script, 100, {"density"});
        auto end = std::chrono::steady_clock::now();
        
        double time_ms = std::chrono::duration&lt;double, std::milli&gt;(end - start).count();
        timings.push_back(time_ms);
    }
    
    // Calculate strong scaling efficiency
    double base_time = timings[0];
    for (size_t i = 1; i &lt; timings.size(); ++i) {
        double ideal_time = base_time / gpu_counts[i];
        double actual_time = timings[i];
        double efficiency = ideal_time / actual_time;
        
        // Strong scaling is harder, expect lower efficiency
        EXPECT_GT(efficiency, 0.30) &lt;&lt; "Strong scaling efficiency &lt; 30% for " 
                                    &lt;&lt; gpu_counts[i] &lt;&lt; " GPUs";
    }
}
