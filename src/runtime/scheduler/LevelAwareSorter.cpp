#include "fluidloom/runtime/dependency/DependencyGraph.h"
#include "fluidloom/runtime/nodes/BarrierNode.h"
#include <algorithm>

namespace fluidloom {
namespace runtime {
namespace scheduler {

/**
 * @brief Level-aware sorter that groups nodes by AMR level
 * 
 * Ensures nodes at level 0 execute before level 1, etc.
 * Inserts barriers between level transitions.
 */
class LevelAwareSorter {
public:
    /**
     * @brief Sort nodes by level and insert barriers
     * @param nodes List of nodes (will be reordered)
     */
    static void sortByLevel(std::vector<std::shared_ptr<nodes::ExecutionNode>>& nodes) {
        // Sort nodes by level (ascending)
        std::sort(nodes.begin(), nodes.end(),
            [](const auto& a, const auto& b) {
                return a->getLevel() < b->getLevel();
            });
    }
    
    /**
     * @brief Insert barrier nodes between level transitions
     * @param nodes List of nodes (barriers will be inserted)
     */
    static void insertLevelBarriers(std::vector<std::shared_ptr<nodes::ExecutionNode>>& nodes) {
        if (nodes.empty()) return;
        
        std::vector<std::shared_ptr<nodes::ExecutionNode>> result;
        result.reserve(nodes.size() * 2);  // Estimate
        
        int8_t current_level = nodes[0]->getLevel();
        
        for (auto& node : nodes) {
            int8_t node_level = node->getLevel();
            
            // If level changed, insert barrier
            if (node_level != current_level && current_level >= 0 && node_level >= 0) {
                auto barrier = std::make_shared<nodes::BarrierNode>(
                    "LevelBarrier_" + std::to_string(current_level) + "_to_" + std::to_string(node_level),
                    nodes::BarrierNode::BarrierKind::LEVEL_TRANSITION
                );
                result.push_back(barrier);
                current_level = node_level;
            }
            
            result.push_back(node);
        }
        
        nodes = std::move(result);
    }
};

} // namespace scheduler
} // namespace runtime
} // namespace fluidloom
