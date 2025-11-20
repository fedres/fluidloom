// @stable - Module 6 API frozen
#pragma once

#include "fluidloom/parsing/LatticeDescriptor.h"
#include <unordered_map>
#include <string>
#include <mutex>

namespace fluidloom {
namespace parsing {

class LatticeRegistry {
private:
    std::unordered_map<std::string, LatticeDescriptor> lattices;
    mutable std::mutex mutex_;
    
    LatticeRegistry() = default;
    
public:
    ~LatticeRegistry() = default;
    
    LatticeRegistry(const LatticeRegistry&) = delete;
    LatticeRegistry& operator=(const LatticeRegistry&) = delete;
    
    static LatticeRegistry& getInstance();
    
    void add(const LatticeDescriptor& desc);
    const LatticeDescriptor* get(const std::string& name) const;
    bool exists(const std::string& name) const;
    bool validate() const;
    void clear();
    std::string toString() const;
    
    // Generate OpenCL code for all lattices
    std::string generateOpenCLPreamble() const;
};

} // namespace parsing
} // namespace fluidloom
