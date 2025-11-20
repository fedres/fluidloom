#include "fluidloom/halo/packers/HaloUnpackKernel.h"
#include "fluidloom/common/Logger.h"

namespace fluidloom {
namespace halo {

HaloUnpackKernel::HaloUnpackKernel(IBackend* backend)
    : backend(backend) {
}

void HaloUnpackKernel::initialize() {
    // backend->compileKernel("kernels/halo/unpack.cl", "unpack_halo");
    FL_LOG(INFO) << "Initialized HaloUnpackKernel";
}

void HaloUnpackKernel::execute(
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
) {
    std::vector<IBackend::KernelArg> args;
    args.reserve(9);
    
    args.push_back(IBackend::KernelArg::fromBuffer(field_data.device_ptr));
    args.push_back(IBackend::KernelArg::fromBuffer(ghost_cell_indices.device_ptr));
    args.push_back(IBackend::KernelArg::fromBuffer(levels.device_ptr));
    args.push_back(IBackend::KernelArg::fromBuffer(ghost_ranges.device_ptr));
    args.push_back(IBackend::KernelArg::fromBuffer(pack_buffer.device_ptr));
    args.push_back(IBackend::KernelArg::fromBuffer(interp_params.device_ptr));
    args.push_back(IBackend::KernelArg::fromScalar(range_id));
    args.push_back(IBackend::KernelArg::fromScalar(field_idx));
    args.push_back(IBackend::KernelArg::fromScalar(num_components));
    
    backend->launchKernel(
        IBackend::KernelHandle(nullptr),
        num_cells,
        64,
        args
    );
}

} // namespace halo
} // namespace fluidloom
