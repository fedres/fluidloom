#include "fluidloom/parsing/registry/LatticeRegistry.h"
#include <sstream>
#include <stdexcept>

namespace fluidloom {
namespace parsing {

LatticeRegistry& LatticeRegistry::getInstance() {
    static LatticeRegistry instance;
    return instance;
}

void LatticeRegistry::add(const LatticeDescriptor& desc) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (lattices.find(desc.name) != lattices.end()) {
        throw std::runtime_error("Lattice already registered: " + desc.name);
    }
    lattices[desc.name] = desc;
}

const LatticeDescriptor* LatticeRegistry::get(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = lattices.find(name);
    return (it != lattices.end()) ? &it->second : nullptr;
}

bool LatticeRegistry::exists(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return lattices.find(name) != lattices.end();
}

bool LatticeRegistry::validate() const {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& [name, desc] : lattices) {
        if (!desc.validate()) {
            return false;
        }
    }
    return true;
}

void LatticeRegistry::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    lattices.clear();
}

std::string LatticeRegistry::toString() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream oss;
    oss << "LatticeRegistry: " << lattices.size() << " lattices\n";
    for (const auto& [name, desc] : lattices) {
        oss << "  " << desc.toString() << "\n";
    }
    return oss.str();
}

std::string LatticeRegistry::generateOpenCLPreamble() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream oss;
    oss << "// Auto-generated lattice definitions\n\n";
    for (const auto& [name, desc] : lattices) {
        oss << desc.generated.preamble;
        oss << desc.generated.weight_array;
        oss << desc.generated.opposite_array;
        oss << "\n";
    }
    return oss.str();
}

} // namespace parsing
} // namespace fluidloom
