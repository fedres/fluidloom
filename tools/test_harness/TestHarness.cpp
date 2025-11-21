#include "TestHarness.h"
#include "GoldDataManager.h"
#include "BackendAdapter.h"
#include &lt;cmath&gt;
#include &lt;algorithm&gt;
#include &lt;numeric&gt;
#include &lt;fstream&gt;
#include &lt;sstream&gt;

#ifdef __APPLE__
#include &lt;mach/mach.h&gt;
#elif __linux__
#include &lt;sys/resource.h&gt;
#endif

namespace fluidloom {
namespace testing {

// MemoryTracker implementation
TestHarness::MemoryTracker::MemoryTracker() : start_usage_(0), peak_usage_(0) {
#ifdef __APPLE__
    struct task_basic_info info;
    mach_msg_type_number_t size = sizeof(info);
    kern_return_t kerr = task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t)&amp;info, &amp;size);
    if (kerr == KERN_SUCCESS) {
        start_usage_ = info.resident_size;
    }
#elif __linux__
    struct rusage usage;
    getrusage(RUSAGE_SELF, &amp;usage);
    start_usage_ = usage.ru_maxrss * 1024; // Convert KB to bytes
#endif
    peak_usage_ = start_usage_;
}


TestHarness::MemoryTracker::~MemoryTracker() {
}

size_t TestHarness::MemoryTracker::getPeakUsage() const {
#ifdef __APPLE__
    struct task_basic_info info;
    mach_msg_type_number_t size = sizeof(info);
    kern_return_t kerr = task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t)&amp;info, &amp;size);
    if (kerr == KERN_SUCCESS) {
        return (info.resident_size - start_usage_) / (1024 * 1024); // MB
    }
#elif __linux__
    struct rusage usage;
    getrusage(RUSAGE_SELF, &amp;usage);
    return (usage.ru_maxrss * 1024 - start_usage_) / (1024 * 1024); // MB
#endif
    return 0;
}

// TestHarness implementation
TestHarness::TestHarness(BackendType backend_type, bool enable_profiling)
    : test_start_time_(std::chrono::steady_clock::now()) {
    
    // Create backend using BackendAdapter
    backend_ = BackendAdapter::create(backend_type);
    
    if (backend_) {
        // Initialize backend with default device
        backend_-&gt;initialize(0);
        
        // Create field manager
        field_manager_ = std::make_unique&lt;fields::SOAFieldManager&gt;(backend_.get());
    } else {
        throw std::runtime_error("Failed to create backend");
    }
}



TestHarness::~TestHarness() {
    // Shutdown backend before cleanup
    if (backend_) {
        backend_-&gt;shutdown();
    }
    // Cleanup happens automatically via unique_ptr
}


TestHarness::TestResult TestHarness::runSimulation(
    const std::string&amp; simulation_script,
    uint64_t max_steps,
    const std::vector&lt;std::string&gt;&amp; fields_to_validate,
    const std::string&amp; gold_data_dir) {
    
    TestResult result;
    result.passed = false;
    result.l2_error = 0.0;
    result.linf_error = 0.0;
    result.duration_ms = 0.0;
    result.memory_peak_mb = 0;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    try {
        // TODO: Parse and execute simulation script
        // This would use SimulationBuilder to create execution graph
        // and run for max_steps
        
        // For now, return placeholder
        result.message = "Simulation execution not yet implemented";
        
    } catch (const std::exception&amp; e) {
        result.passed = false;
        result.message = std::string("Exception: ") + e.what();
        return result;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    result.duration_ms = std::chrono::duration&lt;double, std::milli&gt;(end - start).count();
    result.memory_peak_mb = memory_tracker_.getPeakUsage();
    
    // Compare with gold data if provided
    if (!gold_data_dir.empty()) {
        for (const auto&amp; field_name : fields_to_validate) {
            std::string gold_path = gold_data_dir + "/" + field_name + ".gold";
            if (!compareWithGold(field_name, gold_path, default_tolerance_, result)) {
                result.passed = false;
                result.message += "\nGold data comparison failed for field: " + field_name;
                return result;
            }
        }
    }
    
    result.passed = true;
    return result;
}

TestHarness::TestResult TestHarness::validateAnalytical(
    std::function&lt;double(double, double, double, double)&gt; analytical_func,
    const std::string&amp; field_name,
    double t,
    const Tolerance&amp; tol) {
    
    TestResult result;
    result.passed = false;
    
    // TODO: Implement analytical validation
    // Would iterate over mesh cells and compare field values
    // with analytical_func(x, y, z, t)
    
    result.message = "Analytical validation not yet implemented";
    return result;
}

TestHarness::BenchmarkResult TestHarness::benchmark(
    const std::string&amp; kernel_name,
    size_t num_warmup,
    size_t num_runs) {
    
    BenchmarkResult result;
    std::vector&lt;double&gt; timings;
    
    // Warm-up runs
    for (size_t i = 0; i &lt; num_warmup; ++i) {
        // TODO: Execute kernel
    }
    
    // Timed runs
    for (size_t i = 0; i &lt; num_runs; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        // TODO: Execute kernel
        auto end = std::chrono::high_resolution_clock::now();
        
        double time_ms = std::chrono::duration&lt;double, std::milli&gt;(end - start).count();
        timings.push_back(time_ms);
    }
    
    // Compute statistics
    result.mean_ms = std::accumulate(timings.begin(), timings.end(), 0.0) / timings.size();
    
    double variance = 0.0;
    for (double t : timings) {
        variance += (t - result.mean_ms) * (t - result.mean_ms);
    }
    result.stddev_ms = std::sqrt(variance / timings.size());
    
    result.min_ms = *std::min_element(timings.begin(), timings.end());
    result.max_ms = *std::max_element(timings.begin(), timings.end());
    result.throughput_mcells_sec = 0.0; // TODO: Calculate based on problem size
    
    return result;
}

size_t TestHarness::getMemoryUsageMB() const {
    return memory_tracker_.getPeakUsage();
}

bool TestHarness::checkMemoryLeak() const {
    // Simple heuristic: if peak usage is significantly higher than start,
    // there might be a leak
    size_t peak = memory_tracker_.getPeakUsage();
    return peak &gt; 100; // More than 100MB growth might indicate leak
}

bool TestHarness::compareWithGold(
    const std::string&amp; field_name,
    const std::string&amp; gold_file_path,
    const Tolerance&amp; tol,
    TestResult&amp; result) {
    
    try {
        size_t expected_size = 0;
        std::vector&lt;float&gt; gold_data = GoldDataManager::loadGoldField(
            gold_file_path, field_name, expected_size);
        
        // TODO: Get computed field data from field manager
        // For now, return false as placeholder
        result.message += "\nGold data comparison not fully implemented";
        return false;
        
    } catch (const std::exception&amp; e) {
        result.message += "\nFailed to load gold data: " + std::string(e.what());
        return false;
    }
}

bool TestHarness::compareResultsCrossBackend(
    const TestResult&amp; result_mock,
    const TestResult&amp; result_gpu,
    const Tolerance&amp; tol) {
    
    // Compare L2 errors
    double error_diff = std::abs(result_mock.l2_error - result_gpu.l2_error);
    if (error_diff &gt; tol.bitwise) {
        return false;
    }
    
    return true;
}

std::vector&lt;float&gt; TestHarness::loadGoldData(
    const std::string&amp; path,
    size_t expected_size) {
    
    return GoldDataManager::loadGoldField(path, "", expected_size);
}

void TestHarness::computeErrors(
    const float* computed,
    const float* reference,
    size_t size,
    double&amp; l2_error,
    double&amp; linf_error) {
    
    double sum_sq_error = 0.0;
    double sum_sq_ref = 0.0;
    double max_error = 0.0;
    
    for (size_t i = 0; i &lt; size; ++i) {
        double error = std::abs(computed[i] - reference[i]);
        sum_sq_error += error * error;
        sum_sq_ref += reference[i] * reference[i];
        max_error = std::max(max_error, error);
    }
    
    l2_error = std::sqrt(sum_sq_error / sum_sq_ref);
    linf_error = max_error;
}

} // namespace testing
} // namespace fluidloom
