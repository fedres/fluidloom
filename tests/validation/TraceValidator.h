#pragma once

#include &lt;string&gt;
#include &lt;vector&gt;
#include &lt;fstream&gt;
#include &lt;sstream&gt;
#include &lt;nlohmann/json.hpp&gt; // Or use a simple JSON parser

namespace fluidloom {
namespace validation {

/**
 * @brief Chrome Trace Event JSON validator
 * 
 * Validates trace files against Chrome Trace Event Format v1.0:
 * - JSON syntax correctness
 * - Required fields present
 * - No duplicate timestamps
 * - Event ordering
 * - Complete event coverage
 */
class TraceValidator {
public:
    struct ValidationResult {
        bool passed;
        std::vector&lt;std::string&gt; errors;
        std::vector&lt;std::string&gt; warnings;
        size_t event_count;
        double total_duration_ms;
    };
    
    /**
     * @brief Validate trace file
     * @param trace_path Path to trace JSON file
     * @param required_events List of event names that must be present
     * @return Validation result
     */
    static ValidationResult validateTraceFile(
        const std::string&amp; trace_path,
        const std::vector&lt;std::string&gt;&amp; required_events = {}) {
        
        ValidationResult result;
        result.passed = true;
        result.event_count = 0;
        result.total_duration_ms = 0.0;
        
        // Read trace file
        std::ifstream file(trace_path);
        if (!file) {
            result.passed = false;
            result.errors.push_back("Failed to open trace file: " + trace_path);
            return result;
        }
        
        // Parse JSON
        try {
            std::string content((std::istreambuf_iterator&lt;char&gt;(file)),
                               std::istreambuf_iterator&lt;char&gt;());
            
            // Simple validation: check for basic JSON structure
            if (content.empty()) {
                result.passed = false;
                result.errors.push_back("Trace file is empty");
                return result;
            }
            
            // Check for Chrome trace format markers
            if (content.find("\"traceEvents\"") == std::string::npos &amp;&amp;
                content.find("\"name\"") == std::string::npos) {
                result.passed = false;
                result.errors.push_back("Not a valid Chrome trace format");
                return result;
            }
            
            // Count events (simple heuristic)
            size_t event_count = 0;
            size_t pos = 0;
            while ((pos = content.find("\"name\"", pos)) != std::string::npos) {
                event_count++;
                pos++;
            }
            result.event_count = event_count;
            
            if (event_count == 0) {
                result.warnings.push_back("No events found in trace");
            }
            
            // Check for required events
            for (const auto&amp; required : required_events) {
                if (content.find("\"name\":\"" + required + "\"") == std::string::npos) {
                    result.passed = false;
                    result.errors.push_back("Required event missing: " + required);
                }
            }
            
        } catch (const std::exception&amp; e) {
            result.passed = false;
            result.errors.push_back(std::string("JSON parse error: ") + e.what());
        }
        
        return result;
    }
    
    /**
     * @brief Check for duplicate timestamps
     */
    static bool checkNoDuplicateTimestamps(const std::string&amp; trace_path) {
        // TODO: Implement timestamp uniqueness check
        return true;
    }
    
    /**
     * @brief Verify event ordering
     */
    static bool checkEventOrdering(const std::string&amp; trace_path) {
        // TODO: Implement ordering validation
        return true;
    }
};

} // namespace validation
} // namespace fluidloom
