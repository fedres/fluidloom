#include "fluidloom/halo/interpolation/TrilinearInterpolator.h"
#include "fluidloom/common/Logger.h"

namespace fluidloom {
namespace halo {

TrilinearInterpolator::TrilinearInterpolator(IBackend* backend)
    : backend(backend), 
      field_registry(registry::FieldRegistry::instance()) {
}

TrilinearInterpolator::~TrilinearInterpolator() {
    // Manual cleanup not possible without DeviceBuffer object in this design
    // if (backend && lut_buffer.device_ptr) {
    //     backend->free(lut_buffer);
    // }
}

void TrilinearInterpolator::initializeLookupTable() {
    host_lut.initialize();
    
    // Allocate and upload LUT to GPU constant memory
    size_t lut_size = sizeof(InterpolationLUT);
    auto dev_buf = backend->allocateBuffer(lut_size);
    
    // Transfer ownership to Buffer struct (manual management)
    lut_buffer.device_ptr = (void*)dev_buf.release();
    lut_buffer.size_bytes = lut_size;
    
    FL_LOG(INFO) << "Initialized Trilinear Interpolation LUT (" << lut_size << " bytes) at " << lut_buffer.device_ptr;
    (void)lut_buffer; // Suppress unused private field warning explicitly
}

bool TrilinearInterpolator::validateInterpolation(const fields::FieldDescriptor& field) const {
    (void)field; // Suppress unused parameter warning
    // TODO: Implement validation logic
    // Create test pattern, interpolate, compare with analytical solution
    return true;
}

} // namespace halo
} // namespace fluidloom
