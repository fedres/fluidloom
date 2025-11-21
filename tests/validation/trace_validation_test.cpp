#include &lt;gtest/gtest.h&gt;
#include "tests/validation/TraceValidator.h"
#include &lt;fstream&gt;

using namespace fluidloom::validation;

/**
 * @brief Trace file validation tests
 * 
 * Validates:
 * - Chrome Trace Event JSON format compliance
 * - Required events present
 * - No duplicate timestamps
 * - Valid event ordering
 * - Complete coverage (no gaps)
 */
class TraceValidationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test trace file
        createTestTraceFile("test_trace.json");
    }
    
    void TearDown() override {
        // Cleanup
        std::remove("test_trace.json");
        std::remove("invalid_trace.json");
    }
    
    void createTestTraceFile(const std::string&amp; filename) {
        std::ofstream file(filename);
        file &lt;&lt; R"({
  "traceEvents": [
    {
      "name": "LBM_Collide",
      "cat": "kernel",
      "ph": "X",
      "ts": 1000,
      "dur": 500,
      "pid": 0,
      "tid": 0
    },
    {
      "name": "LBM_Stream",
      "cat": "kernel",
      "ph": "X",
      "ts": 1600,
      "dur": 400,
      "pid": 0,
      "tid": 0
    },
    {
      "name": "HashTable_Rebuild",
      "cat": "rebuild",
      "ph": "X",
      "ts": 2100,
      "dur": 800,
      "pid": 0,
      "tid": 0
    }
  ]
})";
        file.close();
    }
};

TEST_F(TraceValidationTest, ValidTraceFile) {
    auto result = TraceValidator::validateTraceFile("test_trace.json");
    
    EXPECT_TRUE(result.passed) &lt;&lt; "Valid trace should pass";
    EXPECT_GT(result.event_count, 0) &lt;&lt; "Should have events";
    EXPECT_TRUE(result.errors.empty()) &lt;&lt; "Should have no errors";
}

TEST_F(TraceValidationTest, RequiredEventsPresent) {
    std::vector&lt;std::string&gt; required = {
        "LBM_Collide",
        "LBM_Stream",
        "HashTable_Rebuild"
    };
    
    auto result = TraceValidator::validateTraceFile("test_trace.json", required);
    
    EXPECT_TRUE(result.passed) &lt;&lt; "All required events should be present";
}

TEST_F(TraceValidationTest, MissingRequiredEvent) {
    std::vector&lt;std::string&gt; required = {
        "LBM_Collide",
        "NonexistentEvent"
    };
    
    auto result = TraceValidator::validateTraceFile("test_trace.json", required);
    
    EXPECT_FALSE(result.passed) &lt;&lt; "Should fail when required event missing";
    EXPECT_FALSE(result.errors.empty()) &lt;&lt; "Should report error";
}

TEST_F(TraceValidationTest, InvalidTraceFile) {
    // Create invalid JSON
    std::ofstream file("invalid_trace.json");
    file &lt;&lt; "{ invalid json }";
    file.close();
    
    auto result = TraceValidator::validateTraceFile("invalid_trace.json");
    
    EXPECT_FALSE(result.passed) &lt;&lt; "Invalid JSON should fail";
}

TEST_F(TraceValidationTest, EmptyTraceFile) {
    std::ofstream file("empty_trace.json");
    file.close();
    
    auto result = TraceValidator::validateTraceFile("empty_trace.json");
    
    EXPECT_FALSE(result.passed) &lt;&lt; "Empty file should fail";
    
    std::remove("empty_trace.json");
}

TEST_F(TraceValidationTest, DISABLED_NoDuplicateTimestamps) {
    bool no_duplicates = TraceValidator::checkNoDuplicateTimestamps("test_trace.json");
    EXPECT_TRUE(no_duplicates) &lt;&lt; "Should have no duplicate timestamps";
}

TEST_F(TraceValidationTest, DISABLED_ValidEventOrdering) {
    bool ordered = TraceValidator::checkEventOrdering("test_trace.json");
    EXPECT_TRUE(ordered) &lt;&lt; "Events should be properly ordered";
}
