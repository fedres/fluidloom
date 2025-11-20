#include "fluidloom/runtime/scheduler/TopologicalScheduler.h"
#include "fluidloom/common/Logger.h"
#include <chrono>
#include <unordered_map>

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

namespace fluidloom {
namespace runtime {
namespace scheduler {

bool TopologicalScheduler::execute() {
    auto start = std::chrono::high_resolution_clock::now();
    
    const auto& order = graph->getTopologicalOrder();
    
    // Track completion events for each node
    std::unordered_map<int64_t, cl_event> node_events;
    
    FL_LOG(INFO) << "TopologicalScheduler executing " << order.size() << " nodes";
    
    // Execute nodes in topological order
    for (size_t node_idx : order) {
        auto node = graph->getNode(node_idx);
        
        // Collect predecessor events
        std::vector<cl_event> wait_events;
        for (const auto& weak_pred : node->getPredecessors()) {
            if (auto pred = weak_pred.lock()) {
                auto it = node_events.find(pred->getId());
                if (it != node_events.end() && it->second != nullptr) {
                    wait_events.push_back(it->second);
                }
            }
        }
        
        // Execute node with dependencies
        cl_event completion_event = nullptr;
        if (wait_events.empty()) {
            completion_event = node->execute(nullptr);
        } else {
            // For simplicity, use last event as dependency
            // Full implementation would create marker event
            completion_event = node->execute(wait_events.back());
        }
        
        // Store completion event
        node_events[node->getId()] = completion_event;
        
        FL_LOG(DEBUG) << "Executed node " << node->getId() << ": " << node->getName();
    }
    
    // Wait for all nodes to complete
    for (const auto& [node_id, event] : node_events) {
        if (event) {
            clWaitForEvents(1, &event);
            clReleaseEvent(event);
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    last_schedule_time_ms = std::chrono::duration<double, std::milli>(end - start).count();
    
    FL_LOG(INFO) << "TopologicalScheduler completed in " << last_schedule_time_ms << " ms";
    
    return true;
}

bool TopologicalScheduler::executeIncremental(size_t start_idx, size_t end_idx) {
    (void)start_idx;
    (void)end_idx;
    // Stub for Module 12 (kernel fusion)
    FL_LOG(WARN) << "Incremental execution not yet implemented (Module 12)";
    return false;
}

} // namespace scheduler
} // namespace runtime
} // namespace fluidloom
