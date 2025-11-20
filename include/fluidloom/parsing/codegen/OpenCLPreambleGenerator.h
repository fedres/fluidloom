// @stable - Module 6 API frozen
#pragma once

#include <string>

namespace fluidloom {
namespace parsing {

class OpenCLPreambleGenerator {
public:
    OpenCLPreambleGenerator() = default;
    ~OpenCLPreambleGenerator() = default;
    
    // Generate complete preamble from all registries
    std::string generate() const;
    
    // Generate and write to file
    bool generateToFile(const std::string& output_path) const;
    
private:
    std::string generateFieldAccessMacros() const;
    std::string generateLatticeDefinitions() const;
    std::string generateConstants() const;
};

} // namespace parsing
} // namespace fluidloom
