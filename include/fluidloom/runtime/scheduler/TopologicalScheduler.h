#pragma once
// @stable - Topological scheduler with Kahn's algorithm

#include "fluidloom/runtime/dependency/DependencyGraph.h"
#include <vector>
#include <memory>

namespace fluidloom {
namespace runtime {
namespace scheduler {

/**
 * @brief Topological scheduler using Kahn's algorithm
 * 
 * Schedules execution nodes in dependency order, ensuring all
 * hazards are respected and maximum parallelism is achieved.
 */
class TopologicalScheduler {
private:
    // The dependency graph to schedule
    std::shared_ptr<dependency::DependencyGraph> graph;
    
    // Performance metrics
    double last_schedule_time_ms = 0.0;
    
public:
    explicit TopologicalScheduler(std::shared_ptr<dependency::DependencyGraph> dep_graph)
        : graph(std::move(dep_graph)) {}
    
    /**
     * @brief Execute all nodes in topological order
     * @return True if execution completed successfully
     */
    bool execute();
    
    /**
     * @brief Get the execution order
     * @return Vector of node indices in execution order
     */
    const std::vector<size_t>& getExecutionOrder() const {
        return graph->getTopologicalOrder();
    }
    
    /**
     * @brief Get last schedule time
     */
    double getLastScheduleTime() const { return last_schedule_time_ms; }
    
    /**
     * @brief Stub for incremental execution (Module 12)
     */
    bool executeIncremental(size_t start_idx, size_t end_idx);
};

} // namespace scheduler
} // namespace runtime
} // namespace fluidloom
