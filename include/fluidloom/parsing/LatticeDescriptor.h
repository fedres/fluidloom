// @stable - Module 6 API frozen
#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <array>

namespace fluidloom {
namespace parsing {

/**
 * @brief Descriptor for a lattice declared in lattices.fl
 */
struct LatticeDescriptor {
    std::string name;
    bool is_valid{false};
    
    std::vector<std::array<int8_t, 3>> stencil_vectors;
    std::vector<double> weights;
    double cs2{0.0};
    
    size_t num_populations{0};
    int8_t stencil_dimensions{3};
    
    struct OppositeIndices {
        std::vector<uint8_t> opposite;
        bool has_opposites{false};
    } opposite;
    
    struct GeneratedCode {
        std::string preamble;
        std::string weight_array;
        std::string opposite_array;
    } generated;
    
    LatticeDescriptor() = default;
    ~LatticeDescriptor() = default;
    
    bool validate() const;
    void computeOpposites();
    void generateOpenCLCode();
    std::string toString() const;
};

/**
 * @brief Descriptor for exported constants
 */
struct ConstantDescriptor {
    std::string name;
    enum class Type { FLOAT, INT } type;
    union Value {
        float f;
        int32_t i;
    } value;
    
    std::string getOpenCLDefine() const;
};

} // namespace parsing
} // namespace fluidloom
