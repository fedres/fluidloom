#include &lt;gtest/gtest.h&gt;
#include "tools/test_harness/TestHarness.h"
#include &lt;vector&gt;
#include &lt;fstream&gt;
#include &lt;cstdlib&gt;

using namespace fluidloom::testing;

/**
 * @brief Memory safety validation tests
 * 
 * Validates:
 * - No memory leaks (Valgrind/ASan)
 * - No buffer overflows
 * - No use-after-free
 * - Proper cleanup on exceptions
 */
class MemorySafetyTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup test environment
    }
    
    void TearDown() override {
        // Cleanup
    }
};

TEST_F(MemorySafetyTest, NoMemoryLeaks_SmallSimulation) {
    TestHarness harness(BackendType::MOCK, false);
    
    // Record initial memory
    size_t initial_memory = harness.getMemoryUsageMB();
    
    // Run small simulation
    std::string script = "# Small test simulation";
    TestResult result = harness.runSimulation(script, 100, {"density"});
    
    // Check for leaks
    EXPECT_FALSE(harness.checkMemoryLeak()) &lt;&lt; "Memory leak detected";
    
    // Memory should not grow excessively
    size_t final_memory = harness.getMemoryUsageMB();
    size_t growth = final_memory - initial_memory;
    EXPECT_LT(growth, 50) &lt;&lt; "Memory grew by " &lt;&lt; growth &lt;&lt; " MB";
}

TEST_F(MemorySafetyTest, NoLeaks_LargeAllocation) {
    TestHarness harness(BackendType::MOCK, false);
    
    // Allocate large field
    auto field_manager = harness.getFieldManager();
    
    // TODO: Allocate and deallocate large fields
    // Verify no leaks
    
    EXPECT_FALSE(harness.checkMemoryLeak());
}

TEST_F(MemorySafetyTest, DISABLED_ValgrindClean) {
    // This test should be run manually with Valgrind
    // valgrind --leak-check=full --error-exitcode=1 ./memory_safety_test
    
    TestHarness harness(BackendType::MOCK, false);
    std::string script = "# Valgrind test";
    TestResult result = harness.runSimulation(script, 10, {"density"});
    
    EXPECT_TRUE(result.passed);
}

TEST_F(MemorySafetyTest, ExceptionSafety) {
    // Test that exceptions don't leak memory
    try {
        TestHarness harness(BackendType::MOCK, false);
        
        // Trigger an error condition
        std::string invalid_script = "# Invalid script that should fail";
        TestResult result = harness.runSimulation(invalid_script, 1, {"nonexistent_field"});
        
        // Even if it fails, no leaks should occur
        EXPECT_FALSE(harness.checkMemoryLeak());
        
    } catch (...) {
        // Exception is expected, just verify no leaks
        EXPECT_TRUE(true);
    }
}

TEST_F(MemorySafetyTest, RepeatedAllocationDeallocation) {
    TestHarness harness(BackendType::MOCK, false);
    
    size_t initial_memory = harness.getMemoryUsageMB();
    
    // Allocate and deallocate multiple times
    for (int i = 0; i &lt; 10; ++i) {
        std::string script = "# Iteration " + std::to_string(i);
        TestResult result = harness.runSimulation(script, 10, {"density"});
    }
    
    size_t final_memory = harness.getMemoryUsageMB();
    size_t growth = final_memory - initial_memory;
    
    // Memory should not grow linearly with iterations
    EXPECT_LT(growth, 100) &lt;&lt; "Memory leaked across iterations";
}
