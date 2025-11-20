#pragma once
// @stable - Immutable dependency graph representation

#include "fluidloom/runtime/nodes/ExecutionNode.h"
#include <vector>
#include <unordered_map>
#include <memory>

namespace fluidloom {
namespace runtime {
namespace dependency {

/**
 * @brief Immutable directed acyclic graph of execution nodes
 * 
 * Once built, the graph structure cannot be modified. This prevents
 * race conditions during scheduling. To modify, rebuild a new graph.
 * 
 * The graph is **topologically sorted** on construction and stores
 * both forward and reverse adjacency for efficient traversal.
 */
class DependencyGraph {
private:
    // All nodes (owns memory)
    std::vector<std::shared_ptr<nodes::ExecutionNode>> nodes;
    
    // Node ID → index in nodes vector (for O(1) lookup)
    std::unordered_map<int64_t, size_t> node_id_to_index;
    
    // Predecessor lists (dense representation for cache efficiency)
    std::vector<std::vector<size_t>> predecessors;
    
    // Successor lists
    std::vector<std::vector<size_t>> successors;
    
    // Topological order (cached)
    std::vector<size_t> topological_order;
    
    // In-degree for each node (for Kahn's algorithm)
    std::vector<size_t> in_degree;
    
    // Metadata
    size_t num_edges = 0;
    bool is_valid = false;
    
public:
    DependencyGraph() = default;
    
    // Construct graph from unsorted nodes (runs builder internally)
    explicit DependencyGraph(std::vector<std::shared_ptr<nodes::ExecutionNode>> node_list);
    
    ~DependencyGraph() = default;
    
    // Non-copyable (graphs are large; move-only)
    DependencyGraph(const DependencyGraph&) = delete;
    DependencyGraph& operator=(const DependencyGraph&) = delete;
    DependencyGraph(DependencyGraph&&) = default;
    DependencyGraph& operator=(DependencyGraph&&) = default;
    
    // --- Node access ---
    size_t getNodeCount() const { return nodes.size(); }
    
    const auto& getNode(size_t index) const { 
        if (index >= nodes.size()) {
            throw std::out_of_range("Node index out of range");
        }
        return nodes[index]; 
    }
    
    // Find node by ID
    std::shared_ptr<nodes::ExecutionNode> getNodeById(int64_t id) const;
    
    // --- Graph topology ---
    const auto& getPredecessors(size_t node_idx) const { 
        return predecessors[node_idx]; 
    }
    
    const auto& getSuccessors(size_t node_idx) const { 
        return successors[node_idx]; 
    }
    
    const auto& getTopologicalOrder() const { 
        if (!is_valid) {
            throw std::logic_error("Graph not yet validated");
        }
        return topological_order; 
    }
    
    size_t getNumEdges() const { return num_edges; }
    
    // --- Validation ---
    bool validate() const;
    bool hasCycles() const;
    std::vector<size_t> findCycles() const;
    
    // --- Traversal utilities ---
    void depthFirstSearch(size_t start_idx, std::function<void(size_t)> visitor) const;
    void breadthFirstSearch(size_t start_idx, std::function<void(size_t)> visitor) const;
    
    // --- Debug ---
    std::string toDOT() const;
    std::string toString() const;
    
private:
    void buildAdjacencyLists();
    void computeTopologicalOrder();
};

} // namespace dependency
} // namespace runtime
} // namespace fluidloom
