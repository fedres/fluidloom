#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <stdexcept>

namespace fluidloom {
namespace adaptation {

/**
 * @brief Registry for field-specific averaging rules during merge operations
 * 
 * Different fields require different averaging strategies:
 * - Density, mass: volume-weighted sum
 * - Velocity, momentum: arithmetic mean
 * - LBM populations: custom (equilibrium-preserving)
 */
class FieldAveragingRuleRegistry {
public:
    // Averaging function signature: (parent_index, child_indices[8], field_data) → parent_value
    using AveragingFunc = std::function<void(size_t, const size_t*, const float*, float&)>;
    
    static FieldAveragingRuleRegistry& getInstance() {
        static FieldAveragingRuleRegistry instance;
        return instance;
    }
    
    // Register a field's averaging rule by name
    void registerRule(const std::string& field_name, const std::string& rule_type) {
        if (rule_type == "arithmetic") {
            rules_[field_name] = [](size_t parent_idx, const size_t* child_indices, 
                                   const float* field_data, float& result) {
                (void)parent_idx;
                // Simple arithmetic mean of 8 children
                float sum = 0.0f;
                for (int i = 0; i < 8; ++i) {
                    sum += field_data[child_indices[i]];
                }
                result = sum * 0.125f;
            };
        } else if (rule_type == "volume_weighted") {
            rules_[field_name] = [](size_t parent_idx, const size_t* child_indices,
                                   const float* field_data, float& result) {
                (void)parent_idx;
                // Volume-weighted sum (child volume = parent volume / 8)
                float sum = 0.0f;
                for (int i = 0; i < 8; ++i) {
                    sum += field_data[child_indices[i]];
                }
                result = sum;  // Sum of children = parent (conservation)
            };
        } else {
            throw std::invalid_argument("Unknown averaging rule: " + rule_type);
        }
        
        rule_types_[field_name] = rule_type;
    }
    
    // Get averaging function for a field
    AveragingFunc getRule(const std::string& field_name) const {
        auto it = rules_.find(field_name);
        if (it == rules_.end()) {
            // Default to arithmetic if not specified
            return [](size_t parent_idx, const size_t* child_indices,
                     const float* field_data, float& result) {
                (void)parent_idx;
                float sum = 0.0f;
                for (int i = 0; i < 8; ++i) {
                    sum += field_data[child_indices[i]];
                }
                result = sum * 0.125f;
            };
        }
        return it->second;
    }
    
    std::string getRuleType(const std::string& field_name) const {
        auto it = rule_types_.find(field_name);
        return (it != rule_types_.end()) ? it->second : "arithmetic";
    }
    
private:
    FieldAveragingRuleRegistry() = default;
    std::unordered_map<std::string, AveragingFunc> rules_;
    std::unordered_map<std::string, std::string> rule_types_;
};

} // namespace adaptation
} // namespace fluidloom
