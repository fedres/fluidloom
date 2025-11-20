#pragma once

#include "fluidloom/halo/GhostRange.h"
#include "fluidloom/core/fields/FieldDescriptor.h"
#include "fluidloom/core/soa/Buffer.h"
#include "fluidloom/core/backend/IBackend.h"
#include <array>

namespace fluidloom {
namespace halo {

/**
 * @brief Handles fine→coarse averaging for halo exchange
 * 
 * When a coarse GPU receives fine ghost cells, it must average them.
 * This class pre-computes averaging masks and validates conservation.
 */
class VolumeWeightedAverager {
private:
    // For each coarse cell, track which 8 fine cells contribute
    struct AveragingMask {
        uint32_t coarse_cell_idx;
        std::array<uint32_t, 8> fine_cell_indices;  // May be UNUSED (0xFFFFFFFF)
        uint8_t num_valid_fine_cells;
    };
    
    std::vector<AveragingMask> masks;
    
public:
    VolumeWeightedAverager() = default;
    ~VolumeWeightedAverager() = default;
    
    // Build averaging masks for a set of ghost ranges
    // void buildMasks(const std::vector<GhostRange>& ranges, const HashTable& hash_table);
    
    // Validate mass conservation: sum(fine) == coarse * 8 (within tolerance)
    bool validateConservation(const fields::FieldDescriptor& field,
                             const Buffer& coarse_data,
                             const Buffer& fine_data,
                             float tolerance = 1e-6f) const;
    
    // Apply averaging (host-side for validation, device-side in kernel)
    void applyAveraging(const fields::FieldDescriptor& field,
                       const Buffer& source_data,
                       Buffer& dest_data) const;
};

} // namespace halo
} // namespace fluidloom
