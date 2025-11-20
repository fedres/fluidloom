#include "fluidloom/runtime/dependency/FieldVersionTracker.h"
#include <sstream>
#include <stdexcept>

namespace fluidloom {
namespace runtime {
namespace dependency {

void FieldVersionTracker::registerField(const std::string& field_name) {
    std::unique_lock lock(writer_mutex);
    if (versions.find(field_name) == versions.end()) {
        versions[field_name].store(0, std::memory_order_release);
        last_writer[field_name] = -1;
    }
}

uint64_t FieldVersionTracker::getVersion(const std::string& field_name) const {
    auto it = versions.find(field_name);
    if (it != versions.end()) {
        return it->second.load(std::memory_order_acquire);
    }
    throw std::invalid_argument("Field not registered: " + field_name);
}

uint64_t FieldVersionTracker::incrementVersion(const std::string& field_name, int64_t node_id) {
    std::unique_lock lock(writer_mutex);
    auto it = versions.find(field_name);
    if (it != versions.end()) {
        uint64_t new_version = it->second.fetch_add(1, std::memory_order_release) + 1;
        last_writer[field_name] = node_id;
        return new_version;
    }
    throw std::invalid_argument("Field not registered: " + field_name);
}

int64_t FieldVersionTracker::getLastWriter(const std::string& field_name) const {
    std::shared_lock lock(writer_mutex);
    auto it = last_writer.find(field_name);
    if (it != last_writer.end()) {
        return it->second;
    }
    return -1;
}

uint64_t FieldVersionTracker::incrementEpoch() {
    return global_epoch.fetch_add(1, std::memory_order_seq_cst) + 1;
}

uint64_t FieldVersionTracker::getCurrentEpoch() const {
    return global_epoch.load(std::memory_order_seq_cst);
}

void FieldVersionTracker::resetAll() {
    std::unique_lock lock(writer_mutex);
    for (auto& [name, version] : versions) {
        version.store(0, std::memory_order_release);
    }
    for (auto& [name, writer] : last_writer) {
        writer = -1;
    }
    global_epoch.store(0, std::memory_order_release);
}

std::string FieldVersionTracker::toString() const {
    std::shared_lock lock(writer_mutex);
    std::ostringstream oss;
    oss << "FieldVersionTracker(epoch=" << global_epoch.load() << "):\n";
    for (const auto& [name, version] : versions) {
        oss << "  " << name << ": v" << version.load() 
            << " (last_writer=" << last_writer.at(name) << ")\n";
    }
    return oss.str();
}

} // namespace dependency
} // namespace runtime
} // namespace fluidloom
