#pragma once
// @stable - Hazard detection for dependency graph construction

#include "fluidloom/runtime/nodes/ExecutionNode.h"
#include "fluidloom/runtime/dependency/FieldVersionTracker.h"
#include <vector>
#include <memory>
#include <unordered_set>

namespace fluidloom {
namespace runtime {
namespace dependency {

/**
 * @brief Analyzes field access patterns to detect hazards
 * 
 * Detects RAW (Read-After-Write), WAW (Write-After-Write), and
 * WAR (Write-After-Read) hazards between execution nodes.
 * 
 * Also handles level-aware constraints for AMR.
 */
class HazardAnalyzer {
public:
    enum class HazardType {
        RAW,  // Read-After-Write
        WAW,  // Write-After-Write
        WAR,  // Write-After-Read
        LEVEL_BARRIER  // Different AMR levels
    };
    
    struct Hazard {
        size_t from_node_idx;
        size_t to_node_idx;
        std::string field_name;
        HazardType type;
    };

private:
    // Field version tracker for hazard detection
    std::shared_ptr<FieldVersionTracker> version_tracker;
    
    // Detected hazards
    std::vector<Hazard> detected_hazards;
    
public:
    explicit HazardAnalyzer(std::shared_ptr<FieldVersionTracker> tracker)
        : version_tracker(std::move(tracker)) {}
    
    /**
     * @brief Analyze nodes and detect all hazards
     * @param nodes List of execution nodes
     * @return List of detected hazards
     */
    std::vector<Hazard> analyzeNodes(
        const std::vector<std::shared_ptr<nodes::ExecutionNode>>& nodes);
    
    /**
     * @brief Add edges to nodes based on detected hazards
     * @param nodes List of execution nodes (will be modified)
     * @param hazards List of hazards to enforce
     */
    void enforceHazards(
        std::vector<std::shared_ptr<nodes::ExecutionNode>>& nodes,
        const std::vector<Hazard>& hazards);
    
    const std::vector<Hazard>& getDetectedHazards() const {
        return detected_hazards;
    }
    
private:
    void detectRAW(
        const std::vector<std::shared_ptr<nodes::ExecutionNode>>& nodes,
        std::vector<Hazard>& hazards);
    
    void detectWAW(
        const std::vector<std::shared_ptr<nodes::ExecutionNode>>& nodes,
        std::vector<Hazard>& hazards);
    
    void detectWAR(
        const std::vector<std::shared_ptr<nodes::ExecutionNode>>& nodes,
        std::vector<Hazard>& hazards);
    
    void detectLevelBarriers(
        const std::vector<std::shared_ptr<nodes::ExecutionNode>>& nodes,
        std::vector<Hazard>& hazards);
};

} // namespace dependency
} // namespace runtime
} // namespace fluidloom
