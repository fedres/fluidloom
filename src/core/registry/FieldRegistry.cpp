#include "fluidloom/core/registry/FieldRegistry.h"
#include "fluidloom/common/Logger.h"

namespace fluidloom {
namespace registry {

FieldRegistry& FieldRegistry::instance() {
    static FieldRegistry instance;
    return instance;
}

bool FieldRegistry::registerField(const fields::FieldDescriptor& desc) {
    if (!desc.isValid()) {
        FL_LOG(ERROR) << "Attempt to register invalid field descriptor: " << desc.name;
        return false;
    }
    std::unique_lock<std::shared_mutex> lock(mutex_);
    if (by_name_.find(desc.name) != by_name_.end()) {
        FL_LOG(WARN) << "Field '" << desc.name << "' already registered, skipping";
        return false;
    }
    by_name_[desc.name] = desc;
    by_id_[desc.id] = desc;
    FL_LOG(INFO) << "Registered field: " << desc.name << " (id=" << desc.id << ", components=" << desc.num_components << ")";
    return true;
}

std::optional<fields::FieldDescriptor> FieldRegistry::lookupByName(const std::string& name) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    auto it = by_name_.find(name);
    if (it != by_name_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<fields::FieldDescriptor> FieldRegistry::lookupById(fields::FieldHandle handle) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    auto it = by_id_.find(handle.id);
    if (it != by_id_.end()) {
        return it->second;
    }
    return std::nullopt;
}

bool FieldRegistry::exists(const std::string& name) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return by_name_.find(name) != by_name_.end();
}

std::vector<std::string> FieldRegistry::getAllNames() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    std::vector<std::string> names;
    names.reserve(by_name_.size());
    for (const auto& pair : by_name_) {
        names.push_back(pair.first);
    }
    return names;
}

void FieldRegistry::clear() {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    by_name_.clear();
    by_id_.clear();
    FL_LOG(INFO) << "Cleared FieldRegistry";
}

} // namespace registry
} // namespace fluidloom
