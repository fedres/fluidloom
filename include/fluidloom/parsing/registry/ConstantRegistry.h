// @stable - Module 6 API frozen
#pragma once

#include "fluidloom/parsing/LatticeDescriptor.h"
#include <unordered_map>
#include <string>
#include <mutex>

namespace fluidloom {
namespace parsing {

class ConstantRegistry {
private:
    std::unordered_map<std::string, ConstantDescriptor> constants;
    mutable std::mutex mutex_;
    
    ConstantRegistry() = default;
    
public:
    ~ConstantRegistry() = default;
    
    ConstantRegistry(const ConstantRegistry&) = delete;
    ConstantRegistry& operator=(const ConstantRegistry&) = delete;
    
    static ConstantRegistry& getInstance();
    
    void add(const ConstantDescriptor& desc);
    const ConstantDescriptor* get(const std::string& name) const;
    bool exists(const std::string& name) const;
    bool validate() const;
    void clear();
    std::string toString() const;
    
    // Generate OpenCL #defines for all constants
    std::string generateOpenCLDefines() const;
};

} // namespace parsing
} // namespace fluidloom
