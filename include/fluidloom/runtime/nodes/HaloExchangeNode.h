#pragma once
// @stable - Halo exchange execution node

#include "fluidloom/runtime/nodes/ExecutionNode.h"
#include <vector>

namespace fluidloom {

namespace halo {
    // Forward declarations from Module 7
    struct GhostRange;
}

namespace transport {
    // Forward declaration from Module 8
    class MPITransport;
}

namespace runtime {
namespace nodes {

/**
 * @brief Represents halo exchange operation between GPUs
 * 
 * This node is **automatically inserted** by the scheduler based on halo_depth.
 * It orchestrates pack → MPI → unpack sequence from Modules 7 & 8.
 */
class HaloExchangeNode : public ExecutionNode {
private:
    // Fields that require halo exchange
    std::vector<std::string> halo_fields;
    
    // Which buffer to use (double buffering)
    bool use_buffer_a = true;
    
public:
    HaloExchangeNode(std::string name, std::vector<std::string> fields)
        : ExecutionNode(NodeType::HALO_EXCHANGE, std::move(name)),
          halo_fields(std::move(fields)) {
        
        // Configure node metadata
        setHaloDepth(0);  // Halo nodes don't require halos themselves
        setReadFields(halo_fields);  // Reads from field buffers
        setWriteFields(halo_fields); // Writes to ghost cells
    }
    
    // Override execution: orchestrates pack → MPI → unpack
    cl_event execute(cl_event wait_event) override;
    
    // Visitor pattern
    void accept(Visitor& visitor) override { visitor.visit(*this); }
    
    // Double buffer management
    void swapBuffers() { use_buffer_a = !use_buffer_a; }
    
    const std::vector<std::string>& getHaloFields() const { return halo_fields; }
};

} // namespace nodes
} // namespace runtime
} // namespace fluidloom
