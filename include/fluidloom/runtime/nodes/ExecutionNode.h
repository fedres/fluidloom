#pragma once
// @stable - Base class for all execution nodes in dependency graph

#include <cstdint>
#include <vector>
#include <string>
#include <memory>

// Forward declare OpenCL types to avoid header pollution
typedef struct _cl_event* cl_event;

namespace fluidloom {

// Forward declarations
class IBackend;

namespace runtime {
namespace nodes {

// Forward declarations for visitor pattern
class KernelNode;
class HaloExchangeNode;
class BarrierNode;
class AdaptMeshNode;  // Module 11
class FusedKernelNode;  // Module 12

/**
 * @brief Base class for all execution nodes in dependency graph
 * 
 * This is the **canonical abstraction** for every operation in FluidLoom.
 * All nodes must be visitable, have explicit read/write sets, and implement
 * the execute() method. The DAG is built **exclusively** from these nodes.
 * 
 * Memory management: Nodes are owned by DependencyGraph, stored in shared_ptr
 * for safe traversal from multiple threads (e.g., scheduler + profiler).
 */
class ExecutionNode : public std::enable_shared_from_this<ExecutionNode> {
public:
    // Node type for fast dispatch
    enum class NodeType {
        KERNEL,
        HALO_EXCHANGE,
        BARRIER,
        ADAPT_MESH,      // Placeholder for Module 11
        FUSED_KERNEL     // Placeholder for Module 12
    };

protected:
    // Unique ID within graph (assigned by DependencyGraphBuilder)
    int64_t node_id = -1;
    
    // Human-readable name for debugging/profiling
    std::string node_name;
    
    // Read/write field sets (used for hazard detection)
    std::vector<std::string> read_fields;
    std::vector<std::string> write_fields;
    
    // AMR level of cells this node operates on
    int8_t amr_level = -1;
    
    // Halo depth requirement (0 = local only, 1+ = requires ghost exchange)
    uint8_t halo_depth = 0;
    
    // Execution mask expression (e.g., "cell_state == 0")
    std::string execution_mask;
    
    // Node type
    NodeType node_type;
    
    // Dependency graph adjacency (populated by DependencyGraphBuilder)
    std::vector<std::weak_ptr<ExecutionNode>> predecessors;
    std::vector<std::weak_ptr<ExecutionNode>> successors;
    
    // Profiling metrics (populated by Executor)
    uint64_t execution_count = 0;
    double total_execution_time_ms = 0.0;
    bool is_profiling_enabled = false;
    
public:
    ExecutionNode(NodeType type, std::string name) 
        : node_name(std::move(name)), node_type(type) {}
    
    virtual ~ExecutionNode() = default;
    
    // Non-copyable (graphs must be rebuilt, not copied)
    ExecutionNode(const ExecutionNode&) = delete;
    ExecutionNode& operator=(const ExecutionNode&) = delete;
    
    // --- Accessors (immutable after construction) ---
    int64_t getId() const { return node_id; }
    void setId(int64_t id) { node_id = id; }
    
    const std::string& getName() const { return node_name; }
    NodeType getType() const { return node_type; }
    
    const std::vector<std::string>& getReadFields() const { return read_fields; }
    void setReadFields(std::vector<std::string> fields) { read_fields = std::move(fields); }
    
    const std::vector<std::string>& getWriteFields() const { return write_fields; }
    void setWriteFields(std::vector<std::string> fields) { write_fields = std::move(fields); }
    
    int8_t getLevel() const { return amr_level; }
    void setLevel(int8_t level) { amr_level = level; }
    
    uint8_t getHaloDepth() const { return halo_depth; }
    void setHaloDepth(uint8_t depth) { halo_depth = depth; }
    
    const std::string& getExecutionMask() const { return execution_mask; }
    void setExecutionMask(std::string mask) { execution_mask = std::move(mask); }
    
    // --- Graph navigation ---
    const auto& getPredecessors() const { return predecessors; }
    const auto& getSuccessors() const { return successors; }
    
    void addPredecessor(std::shared_ptr<ExecutionNode> pred) {
        predecessors.push_back(pred);
    }
    
    void addSuccessor(std::shared_ptr<ExecutionNode> succ) {
        successors.push_back(succ);
    }
    
    // --- Virtual execution interface ---
    /**
     * @brief Execute this node and return completion event
     * @param wait_event Single event to wait for (or nullptr for fire-and-forget)
     * @return Event signifying completion (must be waited on before dependents)
     */
    virtual cl_event execute(cl_event wait_event) = 0;
    
    /**
     * @brief Get estimated execution time (for load balancing)
     * @return Time in milliseconds from historical data
     */
    virtual double getEstimatedTime() const {
        return total_execution_time_ms / std::max(execution_count, uint64_t(1));
    }
    
    // --- Profiling ---
    void enableProfiling() { is_profiling_enabled = true; }
    void recordExecution(double time_ms) {
        if (is_profiling_enabled) {
            execution_count++;
            total_execution_time_ms += time_ms;
        }
    }
    
    // --- Visitor pattern for graph traversal ---
    class Visitor {
    public:
        virtual void visit(KernelNode& node) = 0;
        virtual void visit(HaloExchangeNode& node) = 0;
        virtual void visit(BarrierNode& node) = 0;
        virtual void visit(AdaptMeshNode& node) = 0;
        virtual void visit(FusedKernelNode& node) = 0;
        virtual ~Visitor() = default;
    };
    
    virtual void accept(Visitor& visitor) = 0;
    
    // --- Debug and visualization ---
    virtual std::string toDOT() const;
    virtual std::string toString() const;
    
    // --- Hazard checking helper ---
    bool readsField(const std::string& field) const;
    bool writesField(const std::string& field) const;
};

} // namespace nodes
} // namespace runtime
} // namespace fluidloom
