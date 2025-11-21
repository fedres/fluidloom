#pragma once

#include &lt;string&gt;
#include &lt;vector&gt;
#include &lt;chrono&gt;
#include &lt;memory&gt;
#include &lt;functional&gt;
#include &lt;filesystem&gt;

#include "fluidloom/core/backend/IBackend.h"
#include "fluidloom/core/fields/SOAFieldManager.h"
#include "fluidloom/parsing/SimulationBuilder.h"

namespace fluidloom {
namespace testing {

/**
 * @brief Universal test harness for all FluidLoom system tests
 * 
 * Provides:
 * - Automatic backend selection (MOCK for CI, OpenCL for performance)
 * - Gold data loading and comparison with configurable tolerances
 * - Timing measurement with warm-up and multiple repetitions
 * - Memory leak detection via RAII wrappers
 * - Analytical solution validators
 * - Cross-backend result comparison
 */
class TestHarness {
public:
    struct TestResult {
        bool passed;
        std::string message;
        double l2_error;
        double linf_error;
        double duration_ms;
        size_t memory_peak_mb;
        std::string trace_file_path;
        std::vector&lt;std::string&gt; output_files;
    };
    
    struct Tolerance {
        double l2 = 0.01;      // 1% L2 error
        double linf = 0.05;    // 5% L∞ error
        double bitwise = 1e-6; // Relative error for cross-backend
        double restart = 1e-6; // Restart roundtrip error
    };
    
    // Constructor: initializes backend with config
    TestHarness(
        BackendType backend_type = BackendType::MOCK,
        bool enable_profiling = true
    );
    
    // Destructor: runs memory checks and cleanup
    ~TestHarness();
    
    // Run complete simulation and capture results
    TestResult runSimulation(
        const std::string&amp; simulation_script,
        uint64_t max_steps,
        const std::vector&lt;std::string&gt;&amp; fields_to_validate,
        const std::string&amp; gold_data_dir = ""
    );
    
    // Validate against analytical solution
    TestResult validateAnalytical(
        std::function&lt;double(double, double, double, double)&gt; analytical_func,
        const std::string&amp; field_name,
        double t,
        const Tolerance&amp; tol
    );
    
    // Performance benchmark with warm-up
    struct BenchmarkResult {
        double mean_ms;
        double stddev_ms;
        double min_ms;
        double max_ms;
        double throughput_mcells_sec;
    };
    
    BenchmarkResult benchmark(
        const std::string&amp; kernel_name,
        size_t num_warmup = 3,
        size_t num_runs = 10
    );
    
    // Memory leak detection
    size_t getMemoryUsageMB() const;
    bool checkMemoryLeak() const;
    
    // Gold data comparison
    bool compareWithGold(
        const std::string&amp; field_name,
        const std::string&amp; gold_file_path,
        const Tolerance&amp; tol,
        TestResult&amp; result
    );
    
    // Cross-backend validation
    static bool compareResultsCrossBackend(
        const TestResult&amp; result_mock,
        const TestResult&amp; result_gpu,
        const Tolerance&amp; tol
    );
    
    // Get backend for direct access
    IBackend* getBackend() { return backend_.get(); }
    
    // Get field manager for direct access
    fields::SOAFieldManager* getFieldManager() { return field_manager_.get(); }
    
private:
    // RAII wrapper for memory tracking
    class MemoryTracker {
    public:
        MemoryTracker();
        ~MemoryTracker();
        size_t getPeakUsage() const;
    private:
        size_t start_usage_;
        size_t peak_usage_;
    };
    
    std::unique_ptr&lt;IBackend&gt; backend_;
    std::unique_ptr&lt;fields::SOAFieldManager&gt; field_manager_;
    
    MemoryTracker memory_tracker_;
    Tolerance default_tolerance_;
    std::string test_name_;
    std::chrono::steady_clock::time_point test_start_time_;
    
    // Helper: load gold data from binary file
    std::vector&lt;float&gt; loadGoldData(
        const std::string&amp; path,
        size_t expected_size
    );
    
    // Helper: compute L2 and L∞ norms
    void computeErrors(
        const float* computed,
        const float* reference,
        size_t size,
        double&amp; l2_error,
        double&amp; linf_error
    );
};

} // namespace testing
} // namespace fluidloom
