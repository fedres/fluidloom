// @stable - Module 7 API frozen
#pragma once

#include <array>
#include <cmath>

namespace fluidloom {
namespace halo {

/**
 * @brief Parameters for trilinear interpolation on device
 * 
 * Stored in __constant memory for fast access by interpolation kernels.
 * Weights are pre-computed based on relative position between coarse and fine cells.
 */
struct TrilinearParams {
    // For coarse → fine interpolation
    std::array<float, 8> weights;    // Weights for 8 coarse corner cells
    std::array<int8_t, 8> offset_x;  // Coordinate offsets
    std::array<int8_t, 8> offset_y;
    std::array<int8_t, 8> offset_z;
    
    // For fine → coarse averaging
    float fine_to_coarse_factor{0.125f};  // Typically 1/8.0f for volume-weighted
    
    // Validation
    bool validate() const {
        float sum = 0.0f;
        for (float w : weights) sum += w;
        return std::fabs(sum - 1.0f) < 1e-6f;
    }
    
    // Initialize with default trilinear weights
    void initializeDefault() {
        // Standard trilinear interpolation weights
        // For a cell at the center of a coarse voxel
        for (int i = 0; i < 8; ++i) {
            weights[i] = 0.125f; // Equal weights for center
            offset_x[i] = (i & 1) ? 1 : 0;
            offset_y[i] = (i & 2) ? 1 : 0;
            offset_z[i] = (i & 4) ? 1 : 0;
        }
    }
};

/**
 * @brief Lookup table for all possible level differences
 */
struct InterpolationLUT {
    static constexpr size_t MAX_LEVEL = 8;
    TrilinearParams params[MAX_LEVEL][MAX_LEVEL];  // [local][remote]
    
    // Initialize on host during setup
    void initialize() {
        for (size_t local = 0; local < MAX_LEVEL; ++local) {
            for (size_t remote = 0; remote < MAX_LEVEL; ++remote) {
                params[local][remote].initializeDefault();
            }
        }
    }
    
    // Get parameters for a specific level pair
    const TrilinearParams& get(uint8_t local_level, uint8_t remote_level) const {
        return params[local_level][remote_level];
    }
};

} // namespace halo
} // namespace fluidloom
