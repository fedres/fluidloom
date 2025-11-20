#include "fluidloom/parsing/LatticeDescriptor.h"
#include <sstream>
#include <cmath>
#include <algorithm>

namespace fluidloom {
namespace parsing {

bool LatticeDescriptor::validate() const {
    if (stencil_vectors.empty() || weights.empty()) {
        return false;
    }
    
    if (stencil_vectors.size() != weights.size()) {
        return false;
    }
    
    // Check weights sum to 1.0
    double sum = 0.0;
    for (double w : weights) {
        sum += w;
    }
    if (std::abs(sum - 1.0) > 1e-6) {
        return false;
    }
    
    return true;
}

void LatticeDescriptor::computeOpposites() {
    opposite.opposite.resize(stencil_vectors.size());
    opposite.has_opposites = true;
    
    for (size_t i = 0; i < stencil_vectors.size(); ++i) {
        bool found = false;
        for (size_t j = 0; j < stencil_vectors.size(); ++j) {
            if (stencil_vectors[i][0] == -stencil_vectors[j][0] &&
                stencil_vectors[i][1] == -stencil_vectors[j][1] &&
                stencil_vectors[i][2] == -stencil_vectors[j][2]) {
                opposite.opposite[i] = static_cast<uint8_t>(j);
                found = true;
                break;
            }
        }
        if (!found) {
            opposite.has_opposites = false;
        }
    }
}

void LatticeDescriptor::generateOpenCLCode() {
    std::ostringstream preamble, weight_arr, opp_arr;
    
    // Generate stencil defines
    preamble << "// Lattice: " << name << "\n";
    preamble << "#define " << name << "_Q " << stencil_vectors.size() << "\n";
    preamble << "#define " << name << "_CS2 " << cs2 << "f\n\n";
    
    // Generate weight array
    weight_arr << "__constant float " << name << "_weights[" << stencil_vectors.size() << "] = {";
    for (size_t i = 0; i < weights.size(); ++i) {
        if (i > 0) weight_arr << ", ";
        weight_arr << weights[i] << "f";
    }
    weight_arr << "};\n";
    
    // Generate opposite array if available
    if (opposite.has_opposites) {
        opp_arr << "__constant uchar " << name << "_opposite[" << stencil_vectors.size() << "] = {";
        for (size_t i = 0; i < opposite.opposite.size(); ++i) {
            if (i > 0) opp_arr << ", ";
            opp_arr << static_cast<int>(opposite.opposite[i]);
        }
        opp_arr << "};\n";
    }
    
    generated.preamble = preamble.str();
    generated.weight_array = weight_arr.str();
    generated.opposite_array = opp_arr.str();
}

std::string LatticeDescriptor::toString() const {
    std::ostringstream oss;
    oss << "Lattice: " << name << " (Q=" << stencil_vectors.size() << ", cs2=" << cs2 << ")";
    return oss.str();
}

std::string ConstantDescriptor::getOpenCLDefine() const {
    std::ostringstream oss;
    oss << "#define " << name << " ";
    if (type == Type::FLOAT) {
        oss << value.f << "f";
    } else {
        oss << value.i;
    }
    oss << "\n";
    return oss.str();
}

} // namespace parsing
} // namespace fluidloom
