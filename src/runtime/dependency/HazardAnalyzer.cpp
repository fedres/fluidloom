#include "fluidloom/runtime/dependency/HazardAnalyzer.h"
#include <algorithm>

namespace fluidloom {
namespace runtime {
namespace dependency {

std::vector<HazardAnalyzer::Hazard> HazardAnalyzer::analyzeNodes(
    const std::vector<std::shared_ptr<nodes::ExecutionNode>>& nodes) {
    
    detected_hazards.clear();
    
    // Detect all hazard types
    detectRAW(nodes, detected_hazards);
    detectWAW(nodes, detected_hazards);
    detectWAR(nodes, detected_hazards);
    detectLevelBarriers(nodes, detected_hazards);
    
    return detected_hazards;
}

void HazardAnalyzer::enforceHazards(
    std::vector<std::shared_ptr<nodes::ExecutionNode>>& nodes,
    const std::vector<Hazard>& hazards) {
    
    for (const auto& hazard : hazards) {
        auto& from_node = nodes[hazard.from_node_idx];
        auto& to_node = nodes[hazard.to_node_idx];
        
        // Add edge: from_node → to_node
        from_node->addSuccessor(to_node);
        to_node->addPredecessor(from_node);
    }
}

void HazardAnalyzer::detectRAW(
    const std::vector<std::shared_ptr<nodes::ExecutionNode>>& nodes,
    std::vector<Hazard>& hazards) {
    
    // RAW: Node B reads field F that Node A writes
    // Requires A → B ordering
    
    for (size_t i = 0; i < nodes.size(); ++i) {
        const auto& node_a = nodes[i];
        const auto& writes_a = node_a->getWriteFields();
        
        for (size_t j = i + 1; j < nodes.size(); ++j) {
            const auto& node_b = nodes[j];
            const auto& reads_b = node_b->getReadFields();
            
            // Check if any field written by A is read by B
            for (const auto& field : writes_a) {
                if (std::find(reads_b.begin(), reads_b.end(), field) != reads_b.end()) {
                    hazards.push_back({i, j, field, HazardType::RAW});
                }
            }
        }
    }
}

void HazardAnalyzer::detectWAW(
    const std::vector<std::shared_ptr<nodes::ExecutionNode>>& nodes,
    std::vector<Hazard>& hazards) {
    
    // WAW: Both Node A and Node B write to field F
    // Requires strict ordering A → B
    
    for (size_t i = 0; i < nodes.size(); ++i) {
        const auto& node_a = nodes[i];
        const auto& writes_a = node_a->getWriteFields();
        
        for (size_t j = i + 1; j < nodes.size(); ++j) {
            const auto& node_b = nodes[j];
            const auto& writes_b = node_b->getWriteFields();
            
            // Check if any field is written by both
            for (const auto& field : writes_a) {
                if (std::find(writes_b.begin(), writes_b.end(), field) != writes_b.end()) {
                    hazards.push_back({i, j, field, HazardType::WAW});
                }
            }
        }
    }
}

void HazardAnalyzer::detectWAR(
    const std::vector<std::shared_ptr<nodes::ExecutionNode>>& nodes,
    std::vector<Hazard>& hazards) {
    
    // WAR: Node A reads field F, Node B writes field F
    // Requires A → B ordering
    
    for (size_t i = 0; i < nodes.size(); ++i) {
        const auto& node_a = nodes[i];
        const auto& reads_a = node_a->getReadFields();
        
        for (size_t j = i + 1; j < nodes.size(); ++j) {
            const auto& node_b = nodes[j];
            const auto& writes_b = node_b->getWriteFields();
            
            // Check if any field read by A is written by B
            for (const auto& field : reads_a) {
                if (std::find(writes_b.begin(), writes_b.end(), field) != writes_b.end()) {
                    hazards.push_back({i, j, field, HazardType::WAR});
                }
            }
        }
    }
}

void HazardAnalyzer::detectLevelBarriers(
    const std::vector<std::shared_ptr<nodes::ExecutionNode>>& nodes,
    std::vector<Hazard>& hazards) {
    
    // Level barriers: Nodes at different AMR levels cannot execute concurrently
    // Insert barrier between level transitions
    
    for (size_t i = 0; i < nodes.size(); ++i) {
        const auto& node_a = nodes[i];
        int8_t level_a = node_a->getLevel();
        
        if (level_a < 0) continue;  // Skip nodes without level
        
        for (size_t j = i + 1; j < nodes.size(); ++j) {
            const auto& node_b = nodes[j];
            int8_t level_b = node_b->getLevel();
            
            if (level_b < 0) continue;
            
            // If levels differ, add barrier
            if (level_a != level_b) {
                hazards.push_back({i, j, "", HazardType::LEVEL_BARRIER});
            }
        }
    }
}

} // namespace dependency
} // namespace runtime
} // namespace fluidloom
