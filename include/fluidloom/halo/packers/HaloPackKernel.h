#pragma once

#include "fluidloom/core/backend/IBackend.h"
#include "fluidloom/core/soa/Buffer.h"
#include "fluidloom/halo/GhostRange.h"
#include "fluidloom/halo/InterpolationParams.h"
#include "fluidloom/core/registry/FieldRegistry.h"

namespace fluidloom {
namespace halo {

/**
 * @brief Host-side wrapper for the OpenCL pack kernel
 */
class HaloPackKernel {
public:
    HaloPackKernel(IBackend* backend);
    ~HaloPackKernel() = default;
    
    // Initialize kernels (compile/load)
    void initialize();
    
    // Execute pack kernel for a specific range and field
    void execute(
        const Buffer& field_data,
        const Buffer& local_cell_indices,
        const Buffer& levels,
        const Buffer& ghost_ranges,
        Buffer& pack_buffer,
        const Buffer& interp_params,
        uint32_t range_id,
        uint32_t field_idx,
        uint32_t num_components,
        size_t num_cells
    );
    
private:
    IBackend* backend;
    // Kernel handle would be stored here in a real implementation
    // For now, we assume IBackend handles kernel caching/execution via name
};

} // namespace halo
} // namespace fluidloom
