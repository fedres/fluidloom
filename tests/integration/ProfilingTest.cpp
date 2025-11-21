#include <gtest/gtest.h>
#include "fluidloom/profiling/Profiler.h"
#include "fluidloom/core/backend/OpenCLBackend.h"
#include <fstream>

using namespace fluidloom;
using namespace fluidloom::profiling;

TEST(ProfilingTest, GeneratesTraceFile) {
    // Set output path
    std::string trace_path = "test_trace.json";
    Profiler::getInstance().setOutputPath(trace_path);
    
    {
        ScopedEvent event("TestEvent", "TestCategory");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Flush events
    Profiler::getInstance().flush();
    
    // Verify file exists and has content
    std::ifstream file(trace_path);
    ASSERT_TRUE(file.is_open());
    
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    
    EXPECT_TRUE(content.find("TestEvent") != std::string::npos);
    EXPECT_TRUE(content.find("TestCategory") != std::string::npos);
    EXPECT_TRUE(content.find("\"ph\": \"B\"") != std::string::npos);
    EXPECT_TRUE(content.find("\"ph\": \"E\"") != std::string::npos);
}
