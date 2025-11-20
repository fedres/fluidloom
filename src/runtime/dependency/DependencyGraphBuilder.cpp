#include "fluidloom/runtime/dependency/DependencyGraph.h"
#include <queue>
#include <stack>
#include <sstream>
#include <stdexcept>
#include <algorithm>

namespace fluidloom {
namespace runtime {
namespace dependency {

DependencyGraph::DependencyGraph(std::vector<std::shared_ptr<nodes::ExecutionNode>> node_list)
    : nodes(std::move(node_list)) {
    
    // Build index map
    for (size_t i = 0; i < nodes.size(); ++i) {
        node_id_to_index[nodes[i]->getId()] = i;
    }
    
    // Build adjacency lists
    buildAdjacencyLists();
    
    // Compute topological order
    computeTopologicalOrder();
    
    // Validate
    is_valid = validate();
}

std::shared_ptr<nodes::ExecutionNode> DependencyGraph::getNodeById(int64_t id) const {
    auto it = node_id_to_index.find(id);
    if (it != node_id_to_index.end()) {
        return nodes[it->second];
    }
    return nullptr;
}

void DependencyGraph::buildAdjacencyLists() {
    predecessors.resize(nodes.size());
    successors.resize(nodes.size());
    in_degree.resize(nodes.size(), 0);
    
    num_edges = 0;
    
    for (size_t i = 0; i < nodes.size(); ++i) {
        const auto& node = nodes[i];
        
        // Build successor list from node's successors
        for (const auto& weak_succ : node->getSuccessors()) {
            if (auto succ = weak_succ.lock()) {
                auto it = node_id_to_index.find(succ->getId());
                if (it != node_id_to_index.end()) {
                    size_t succ_idx = it->second;
                    successors[i].push_back(succ_idx);
                    predecessors[succ_idx].push_back(i);
                    in_degree[succ_idx]++;
                    num_edges++;
                }
            }
        }
    }
}

void DependencyGraph::computeTopologicalOrder() {
    // Kahn's algorithm
    topological_order.clear();
    topological_order.reserve(nodes.size());
    
    // Copy in-degree (will be modified)
    std::vector<size_t> temp_in_degree = in_degree;
    
    // Queue of nodes with in-degree 0
    std::queue<size_t> ready_queue;
    for (size_t i = 0; i < nodes.size(); ++i) {
        if (temp_in_degree[i] == 0) {
            ready_queue.push(i);
        }
    }
    
    // Process nodes
    while (!ready_queue.empty()) {
        size_t node_idx = ready_queue.front();
        ready_queue.pop();
        
        topological_order.push_back(node_idx);
        
        // Decrement in-degree of successors
        for (size_t succ_idx : successors[node_idx]) {
            temp_in_degree[succ_idx]--;
            if (temp_in_degree[succ_idx] == 0) {
                ready_queue.push(succ_idx);
            }
        }
    }
    
    // Check for cycles
    if (topological_order.size() != nodes.size()) {
        throw std::runtime_error("Dependency graph contains cycles");
    }
}

bool DependencyGraph::validate() const {
    // Check that topological order is valid
    if (topological_order.size() != nodes.size()) {
        return false;
    }
    
    // Check for cycles
    return !hasCycles();
}

bool DependencyGraph::hasCycles() const {
    std::vector<bool> visited(nodes.size(), false);
    std::vector<bool> rec_stack(nodes.size(), false);
    
    std::function<bool(size_t)> dfs_cycle = [&](size_t idx) -> bool {
        visited[idx] = true;
        rec_stack[idx] = true;
        
        for (size_t succ_idx : successors[idx]) {
            if (!visited[succ_idx]) {
                if (dfs_cycle(succ_idx)) {
                    return true;
                }
            } else if (rec_stack[succ_idx]) {
                return true;  // Back edge found
            }
        }
        
        rec_stack[idx] = false;
        return false;
    };
    
    for (size_t i = 0; i < nodes.size(); ++i) {
        if (!visited[i]) {
            if (dfs_cycle(i)) {
                return true;
            }
        }
    }
    
    return false;
}

std::vector<size_t> DependencyGraph::findCycles() const {
    // Simplified - returns empty for now
    // Full implementation would trace back through rec_stack
    return {};
}

void DependencyGraph::depthFirstSearch(size_t start_idx, std::function<void(size_t)> visitor) const {
    std::vector<bool> visited(nodes.size(), false);
    std::stack<size_t> stack;
    
    stack.push(start_idx);
    
    while (!stack.empty()) {
        size_t idx = stack.top();
        stack.pop();
        
        if (!visited[idx]) {
            visited[idx] = true;
            visitor(idx);
            
            for (size_t succ_idx : successors[idx]) {
                if (!visited[succ_idx]) {
                    stack.push(succ_idx);
                }
            }
        }
    }
}

void DependencyGraph::breadthFirstSearch(size_t start_idx, std::function<void(size_t)> visitor) const {
    std::vector<bool> visited(nodes.size(), false);
    std::queue<size_t> queue;
    
    queue.push(start_idx);
    visited[start_idx] = true;
    
    while (!queue.empty()) {
        size_t idx = queue.front();
        queue.pop();
        
        visitor(idx);
        
        for (size_t succ_idx : successors[idx]) {
            if (!visited[succ_idx]) {
                visited[succ_idx] = true;
                queue.push(succ_idx);
            }
        }
    }
}

std::string DependencyGraph::toDOT() const {
    std::ostringstream oss;
    oss << "digraph DependencyGraph {\n";
    oss << "  rankdir=TB;\n";
    
    for (const auto& node : nodes) {
        oss << node->toDOT();
    }
    
    oss << "}\n";
    return oss.str();
}

std::string DependencyGraph::toString() const {
    std::ostringstream oss;
    oss << "DependencyGraph: " << nodes.size() << " nodes, " << num_edges << " edges\n";
    oss << "Topological order: [";
    for (size_t i = 0; i < topological_order.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << topological_order[i];
    }
    oss << "]\n";
    return oss.str();
}

} // namespace dependency
} // namespace runtime
} // namespace fluidloom
