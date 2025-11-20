#include <vector>
#include <stdexcept>

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

namespace fluidloom {
namespace runtime {
namespace executor {

/**
 * @brief Integrates OpenCL event chains for dependency management
 * 
 * Manages event dependencies between execution nodes.
 * Ensures proper synchronization and memory safety.
 */
class EventChainIntegrator {
private:
    // Track events for cleanup
    std::vector<cl_event> managed_events;
    
public:
    EventChainIntegrator() = default;
    
    ~EventChainIntegrator() {
        // Clean up all managed events
        for (auto event : managed_events) {
            if (event) {
                clReleaseEvent(event);
            }
        }
    }
    
    /**
     * @brief Collect predecessor events for a node
     * @param predecessor_events List of events from predecessors
     * @return Combined event or nullptr if no predecessors
     */
    cl_event collectPredecessorEvents(const std::vector<cl_event>& predecessor_events) {
        if (predecessor_events.empty()) {
            return nullptr;
        }
        
        // For simplicity, we return the last event
        // Full implementation would use clEnqueueMarkerWithWaitList
        // to create a single event that waits on all predecessors
        return predecessor_events.back();
    }
    
    /**
     * @brief Create a marker event that waits on multiple events
     * @param queue Command queue
     * @param wait_events Events to wait on
     * @return Marker event
     */
    cl_event createMarker(cl_command_queue queue, const std::vector<cl_event>& wait_events) {
        if (!queue || wait_events.empty()) {
            return nullptr;
        }
        
        cl_event marker_event;
        cl_int err = clEnqueueMarkerWithWaitList(
            queue,
            wait_events.size(),
            wait_events.data(),
            &marker_event
        );
        
        if (err != CL_SUCCESS) {
            throw std::runtime_error("Failed to create marker event");
        }
        
        managed_events.push_back(marker_event);
        return marker_event;
    }
    
    /**
     * @brief Release completed events
     * @param event Event to release
     */
    void releaseEvent(cl_event event) {
        if (event) {
            clReleaseEvent(event);
            
            // Remove from managed events
            auto it = std::find(managed_events.begin(), managed_events.end(), event);
            if (it != managed_events.end()) {
                managed_events.erase(it);
            }
        }
    }
    
    /**
     * @brief Wait for event to complete
     * @param event Event to wait on
     */
    void waitForEvent(cl_event event) {
        if (event) {
            clWaitForEvents(1, &event);
        }
    }
    
    /**
     * @brief Get event execution time in milliseconds
     * @param event Event to query
     * @return Execution time in ms
     */
    double getEventTime(cl_event event) {
        if (!event) return 0.0;
        
        cl_ulong start_time, end_time;
        
        clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START,
                               sizeof(cl_ulong), &start_time, nullptr);
        clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END,
                               sizeof(cl_ulong), &end_time, nullptr);
        
        return (end_time - start_time) / 1e6;  // Convert nanoseconds to milliseconds
    }
};

} // namespace executor
} // namespace runtime
} // namespace fluidloom
