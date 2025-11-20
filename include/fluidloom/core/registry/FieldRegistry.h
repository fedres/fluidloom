#pragma once

#include "fluidloom/core/fields/FieldDescriptor.h"
#include <unordered_map>
#include <shared_mutex>
#include <optional>
#include <vector>

namespace fluidloom {
namespace registry {

/**
 * @brief Thread‑safe singleton registry for field descriptors.
 */
class FieldRegistry {
public:
    // Get the singleton instance
    static FieldRegistry& instance();

    // Delete copy/move
    FieldRegistry(const FieldRegistry&) = delete;
    FieldRegistry& operator=(const FieldRegistry&) = delete;

    // Register a new field descriptor. Returns false if a field with the same name already exists.
    bool registerField(const fields::FieldDescriptor& desc);

    // Lookup by name (read‑only)
    std::optional<fields::FieldDescriptor> lookupByName(const std::string& name) const;

    // Lookup by handle (fast path)
    std::optional<fields::FieldDescriptor> lookupById(fields::FieldHandle handle) const;

    // Check existence by name
    bool exists(const std::string& name) const;

    // Retrieve all registered field names (for debugging)
    std::vector<std::string> getAllNames() const;

    // Clear all registered fields (for testing)
    void clear();

private:
    FieldRegistry() = default;
    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, fields::FieldDescriptor> by_name_;
    std::unordered_map<uint64_t, fields::FieldDescriptor> by_id_;
};

} // namespace registry
} // namespace fluidloom
