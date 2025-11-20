#include "fluidloom/runtime/nodes/ExecutionNode.h"
#include <algorithm>
#include <sstream>

namespace fluidloom {
namespace runtime {
namespace nodes {

bool ExecutionNode::readsField(const std::string& field) const {
    return std::find(read_fields.begin(), read_fields.end(), field) != read_fields.end();
}

bool ExecutionNode::writesField(const std::string& field) const {
    return std::find(write_fields.begin(), write_fields.end(), field) != write_fields.end();
}

std::string ExecutionNode::toString() const {
    std::ostringstream oss;
    oss << "Node[" << node_id << "]: " << node_name 
        << " (type=" << static_cast<int>(node_type) << ")\n";
    oss << "  Level: " << static_cast<int>(amr_level) << ", Halo: " << static_cast<int>(halo_depth) << "\n";
    oss << "  Reads: [";
    for (size_t i = 0; i < read_fields.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << read_fields[i];
    }
    oss << "]\n  Writes: [";
    for (size_t i = 0; i < write_fields.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << write_fields[i];
    }
    oss << "]\n";
    return oss.str();
}

std::string ExecutionNode::toDOT() const {
    std::ostringstream oss;
    oss << "  node" << node_id << " [label=\"" << node_name << "\\n";
    oss << "L" << static_cast<int>(amr_level) << " H" << static_cast<int>(halo_depth) << "\"];\n";
    
    // Add edges to successors
    for (const auto& weak_succ : successors) {
        if (auto succ = weak_succ.lock()) {
            oss << "  node" << node_id << " -> node" << succ->getId() << ";\n";
        }
    }
    
    return oss.str();
}

} // namespace nodes
} // namespace runtime
} // namespace fluidloom
