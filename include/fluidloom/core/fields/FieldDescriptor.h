#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace fluidloom {
namespace fields {

// Field component data types (must match OpenCL types exactly)
enum class FieldType : uint8_t {
    FLOAT32 = 0,   // cl_float
    FLOAT64 = 1,   // cl_double
    INT32   = 2,   // cl_int
    INT64   = 3,   // cl_long
    UINT8   = 4    // cl_uchar (for masks, flags)
};

// Averaging rules for level mismatches in halo exchange
enum class AveragingRule : uint8_t {
    ARITHMETIC      = 0,
    VOLUME_WEIGHTED = 1,
    MINIMUM         = 2,
    MAXIMUM         = 3,
    SKIP            = 4
};

// Solid cell treatment during kernel execution
enum class SolidScheme : uint8_t {
    ZERO        = 0,
    BOUNCE_BACK = 1,
    NEUMANN     = 2,
    SKIP        = 3,
    CUSTOM      = 4
};

/**
 * @brief Immutable description of a field used throughout the engine.
 */
struct alignas(64) FieldDescriptor {
    // Unique identifier (hash of name)
    uint64_t id{};
    // Human‑readable name
    std::string name;
    // Core characteristics
    FieldType type{FieldType::FLOAT32};
    uint16_t num_components{1};
    // Halo exchange metadata
    uint8_t halo_depth{0};
    AveragingRule avg_rule{AveragingRule::ARITHMETIC};
    // Solid cell handling
    SolidScheme solid_scheme{SolidScheme::ZERO};
    // Default value per component
    std::vector<double> default_value;
    // Runtime flags
    bool is_visualization_field{false};
    bool is_mask{false};
    bool is_material{false};
    // GPU state (populated at allocation time)
    struct {
        uint64_t version{0};
        size_t bytes_allocated{0};
        void* device_ptr{nullptr};
        bool is_dirty{false};
    } gpu_state;

    // Default constructor (needed for unordered_map)
    FieldDescriptor() = default;

    // Constructor validates parameters
    FieldDescriptor(const std::string& field_name,
                    FieldType field_type,
                    uint16_t components,
                    uint8_t halo = 0,
                    AveragingRule avg = AveragingRule::ARITHMETIC,
                    SolidScheme solid = SolidScheme::ZERO);

    // Compute byte size per cell for this field
    size_t bytesPerCell() const;

    // Simple validation used by registries
    bool isValid() const;
};

// Lightweight handle used for map keys
struct FieldHandle {
    uint64_t id;
    explicit FieldHandle(uint64_t field_id) : id(field_id) {}
    bool operator==(const FieldHandle& other) const { return id == other.id; }
    bool operator<(const FieldHandle& other) const { return id < other.id; }
};

} // namespace fields
} // namespace fluidloom

// Hash support for unordered containers
namespace std {
    template<>
    struct hash<fluidloom::fields::FieldHandle> {
        size_t operator()(const fluidloom::fields::FieldHandle& h) const noexcept {
            return std::hash<uint64_t>{}(h.id);
        }
    };
}
