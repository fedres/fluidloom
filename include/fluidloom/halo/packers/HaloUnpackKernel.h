#pragma once

#include "fluidloom/core/backend/IBackend.h"
#include "fluidloom/core/soa/Buffer.h"
#include "fluidloom/halo/GhostRange.h"
#include "fluidloom/halo/InterpolationParams.h"

namespace fluidloom {
namespace halo {

/**
 * @brief Host-side wrapper for the OpenCL unpack kernel
 */
class HaloUnpackKernel {
public:
    HaloUnpackKernel(IBackend* backend);
    ~HaloUnpackKernel() = default;
    
    // Initialize kernels
    void initialize();
    
    // Execute unpack kernel
    void execute(
        Buffer& field_data,
        const Buffer& ghost_cell_indices,
        const Buffer& levels,
        const Buffer& ghost_ranges,
        const Buffer& pack_buffer,
        const Buffer& interp_params,
        uint32_t range_id,
        uint32_t field_idx,
        uint32_t num_components,
        size_t num_cells
    );
    
private:
    IBackend* backend;
};

} // namespace halo
} // namespace fluidloom
