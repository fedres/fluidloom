#pragma once
// @stable - Explicit synchronization barrier node

#include "fluidloom/runtime/nodes/ExecutionNode.h"

namespace fluidloom {
namespace runtime {
namespace nodes {

/**
 * @brief Explicit synchronization barrier
 * 
 * Used to enforce ordering at critical points (before/after adaptation).
 * Translates to clEnqueueBarrierWithWaitList in OpenCL.
 */
class BarrierNode : public ExecutionNode {
public:
    // Barrier kind for profiling
    enum class BarrierKind {
        ADAPTATION_PRE,
        ADAPTATION_POST,
        LOAD_BALANCE,
        USER_REQUESTED,
        LEVEL_TRANSITION  // Between AMR levels
    };

private:
    // True if this is a global barrier across all GPUs
    bool is_global_barrier;
    
    // Barrier kind
    BarrierKind kind;
    
public:
    BarrierNode(std::string name, BarrierKind barrier_kind, bool global = false)
        : ExecutionNode(NodeType::BARRIER, std::move(name)),
          is_global_barrier(global), kind(barrier_kind) {
        setHaloDepth(0);
        // Barriers have no field dependencies (explicit synchronization)
    }
    
    // Override execution: enqueue barrier
    cl_event execute(cl_event wait_event) override;
    
    // Visitor pattern
    void accept(Visitor& visitor) override { visitor.visit(*this); }
    
    bool isGlobal() const { return is_global_barrier; }
    BarrierKind getKind() const { return kind; }
};

} // namespace nodes
} // namespace runtime
} // namespace fluidloom
