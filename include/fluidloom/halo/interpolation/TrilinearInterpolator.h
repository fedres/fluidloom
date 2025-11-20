#pragma once

#include "fluidloom/halo/GhostRange.h"
#include "fluidloom/halo/InterpolationParams.h"
#include "fluidloom/core/registry/FieldRegistry.h"
#include "fluidloom/core/backend/IBackend.h"
#include "fluidloom/core/soa/Buffer.h"

namespace fluidloom {
namespace halo {

/**
 * @brief Pre-computes interpolation weights for coarse→fine ghost exchanges
 * 
 * For each fine ghost cell, finds its 8 coarse corner cells and computes
 * trilinear weights based on relative position within coarse voxel.
 */
class TrilinearInterpolator {
private:
    IBackend* backend;
    const registry::FieldRegistry& field_registry;
    
    // OpenCL buffers for interpolation parameters
    Buffer lut_buffer;  // TrilinearParams LUT
    InterpolationLUT host_lut;
    
public:
    explicit TrilinearInterpolator(IBackend* backend);
    ~TrilinearInterpolator();
    
    // Non-copyable
    TrilinearInterpolator(const TrilinearInterpolator&) = delete;
    TrilinearInterpolator& operator=(const TrilinearInterpolator&) = delete;
    
    // Initialize LUT for all possible level combinations
    void initializeLookupTable();
    
    // Get buffer handle for kernels
    const Buffer& getLUTBuffer() const { return lut_buffer; }
    
    // Validate interpolation accuracy
    bool validateInterpolation(const fields::FieldDescriptor& field) const;
};

} // namespace halo
} // namespace fluidloom
