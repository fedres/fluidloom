#pragma once

#include <cstddef>

namespace fluidloom {

/**
 * @brief Simple wrapper for a device memory buffer
 * Used for passing buffer handles to kernels
 */
struct Buffer {
    void* device_ptr{nullptr};
    size_t size_bytes{0};
    void* event{nullptr}; // Optional event handle associated with last operation
};

} // namespace fluidloom
