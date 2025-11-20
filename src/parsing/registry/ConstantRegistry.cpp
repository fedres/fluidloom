#include "fluidloom/parsing/registry/ConstantRegistry.h"
#include <sstream>
#include <stdexcept>

namespace fluidloom {
namespace parsing {

ConstantRegistry& ConstantRegistry::getInstance() {
    static ConstantRegistry instance;
    return instance;
}

void ConstantRegistry::add(const ConstantDescriptor& desc) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (constants.find(desc.name) != constants.end()) {
        throw std::runtime_error("Constant already registered: " + desc.name);
    }
    constants[desc.name] = desc;
}

const ConstantDescriptor* ConstantRegistry::get(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = constants.find(name);
    return (it != constants.end()) ? &it->second : nullptr;
}

bool ConstantRegistry::exists(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return constants.find(name) != constants.end();
}

bool ConstantRegistry::validate() const {
    // Check for reserved OpenCL names
    const std::vector<std::string> reserved = {
        "kernel", "global", "local", "constant", "private",
        "float", "int", "char", "void", "bool"
    };
    
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& [name, desc] : constants) {
        for (const auto& res : reserved) {
            if (name == res) {
                return false;
            }
        }
    }
    return true;
}

void ConstantRegistry::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    constants.clear();
}

std::string ConstantRegistry::toString() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream oss;
    oss << "ConstantRegistry: " << constants.size() << " constants\n";
    for (const auto& [name, desc] : constants) {
        oss << "  " << name << " = ";
        if (desc.type == ConstantDescriptor::Type::FLOAT) {
            oss << desc.value.f << "f";
        } else {
            oss << desc.value.i;
        }
        oss << "\n";
    }
    return oss.str();
}

std::string ConstantRegistry::generateOpenCLDefines() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream oss;
    oss << "// Auto-generated constant definitions\n\n";
    for (const auto& [name, desc] : constants) {
        oss << desc.getOpenCLDefine();
    }
    return oss.str();
}

} // namespace parsing
} // namespace fluidloom
