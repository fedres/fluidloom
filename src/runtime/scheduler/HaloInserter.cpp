#include "fluidloom/runtime/nodes/HaloExchangeNode.h"
#include "fluidloom/runtime/nodes/KernelNode.h"
#include <algorithm>

namespace fluidloom {
namespace runtime {
namespace scheduler {

/**
 * @brief Automatically inserts HaloExchangeNode before kernels with halo_depth > 0
 */
class HaloInserter {
public:
    /**
     * @brief Insert halo exchange nodes where needed
     * @param nodes List of nodes (halo nodes will be inserted)
     */
    static void insertHaloExchanges(std::vector<std::shared_ptr<nodes::ExecutionNode>>& nodes) {
        std::vector<std::shared_ptr<nodes::ExecutionNode>> result;
        result.reserve(nodes.size() * 2);  // Estimate
        
        for (auto& node : nodes) {
            // Check if this node requires halo exchange
            if (node->getHaloDepth() > 0 && node->getType() == nodes::ExecutionNode::NodeType::KERNEL) {
                // Create halo exchange node
                auto halo_node = std::make_shared<nodes::HaloExchangeNode>(
                    "Halo_" + node->getName(),
                    node->getReadFields()  // Exchange fields that are read
                );
                
                // Insert halo node before kernel
                result.push_back(halo_node);
                
                // Add dependency: halo → kernel
                halo_node->addSuccessor(node);
                node->addPredecessor(halo_node);
            }
            
            result.push_back(node);
        }
        
        nodes = std::move(result);
    }
};

} // namespace scheduler
} // namespace runtime
} // namespace fluidloom
