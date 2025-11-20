#pragma once

#include "fluidloom/runtime/nodes/ExecutionNode.h"
#include <vector>
#include <memory>

namespace fluidloom {
namespace runtime {

// Use nodes namespace for ExecutionNode
using nodes::ExecutionNode;

/**
 * @brief Execution graph for simulation pipeline
 * 
 * Minimal implementation for AMR demo
 */
class ExecutionGraph {
public:
    ExecutionGraph() = default;
    
    void addNode(std::shared_ptr<ExecutionNode> node) {
        m_nodes.push_back(node);
    }
    
    void execute() {
        for (auto& node : m_nodes) {
            node->execute(nullptr);
        }
    }
    
    size_t getNodeCount() const {
        return m_nodes.size();
    }
    
private:
    std::vector<std::shared_ptr<ExecutionNode>> m_nodes;
};

} // namespace runtime
} // namespace fluidloom
