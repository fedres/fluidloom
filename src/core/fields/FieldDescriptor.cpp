#include "fluidloom/core/fields/FieldDescriptor.h"
#include "fluidloom/common/Logger.h"
#include "fluidloom/common/Hash.h" // Assuming a hash utility exists

namespace fluidloom {
namespace fields {

FieldDescriptor::FieldDescriptor(const std::string& field_name,
                                 FieldType field_type,
                                 uint16_t components,
                                 uint8_t halo,
                                 AveragingRule avg,
                                 SolidScheme solid)
    : name(field_name),
      type(field_type),
      num_components(components),
      halo_depth(halo),
      avg_rule(avg),
      solid_scheme(solid) {
    // Validate name length
    if (field_name.empty() || field_name.size() > 64) {
        throw std::invalid_argument("Field name must be 1-64 characters");
    }
    // Validate components
    if (components == 0 || components > 32) {
        throw std::invalid_argument("num_components must be in [1,32]");
    }
    // Validate halo depth (max 2 as per design)
    if (halo > 2) {
        throw std::invalid_argument("halo_depth must be 0, 1, or 2");
    }
    // Compute unique ID (FNV-1a hash of name)
    id = hash::fnv1a_64(field_name);
    // Initialise default values vector
    default_value.resize(components, 0.0);
    // Basic sanity checks for mask and material flags (they can be set later by user)
    if (is_mask && type != FieldType::UINT8) {
        FL_LOG(WARN) << "Mask field '" << name << "' should be UINT8 for efficiency";
    }
    if (is_material && components != 1) {
        throw std::invalid_argument("Material ID fields must be scalar");
    }
}

size_t FieldDescriptor::bytesPerCell() const {
    size_t type_size = 0;
    switch (type) {
        case FieldType::FLOAT32: type_size = 4; break;
        case FieldType::FLOAT64: type_size = 8; break;
        case FieldType::INT32:   type_size = 4; break;
        case FieldType::INT64:   type_size = 8; break;
        case FieldType::UINT8:   type_size = 1; break;
        default: type_size = 0; break;
    }
    return type_size * static_cast<size_t>(num_components);
}

bool FieldDescriptor::isValid() const {
    if (id == 0) {
        FL_LOG(ERROR) << "Field '" << name << "' validation failed: id == 0";
        return false;
    }
    if (name.empty() || name.size() > 64) {
        FL_LOG(ERROR) << "Field '" << name << "' validation failed: name empty or > 64 chars";
        return false;
    }
    if (num_components == 0 || num_components > 32) {
        FL_LOG(ERROR) << "Field '" << name << "' validation failed: num_components=" << num_components;
        return false;
    }
    if (halo_depth > 2) {
        FL_LOG(ERROR) << "Field '" << name << "' validation failed: halo_depth=" << static_cast<int>(halo_depth);
        return false;
    }
    if (default_value.size() != static_cast<size_t>(num_components)) {
        FL_LOG(ERROR) << "Field '" << name << "' validation failed: default_value.size()=" 
                     << default_value.size() << " != num_components=" << num_components;
        return false;
    }
    // GPU state validation – only meaningful after allocation
    if (gpu_state.device_ptr != nullptr) {
        if (gpu_state.bytes_allocated == 0) return false;
        if (gpu_state.version == 0) return false; // version should start at 1
    }
    return true;
}

} // namespace fields
} // namespace fluidloom
