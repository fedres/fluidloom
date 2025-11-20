#pragma once

#include <cstdint>
#include <string>

namespace fluidloom {
namespace hash {

/**
 * @brief FNV-1a 64-bit hash function
 * Fast, simple hash for strings used in field IDs
 */
inline uint64_t fnv1a_64(const std::string& str) {
    constexpr uint64_t FNV_OFFSET_BASIS = 14695981039346656037ULL;
    constexpr uint64_t FNV_PRIME = 1099511628211ULL;
    
    uint64_t hash = FNV_OFFSET_BASIS;
    for (char c : str) {
        hash ^= static_cast<uint64_t>(c);
        hash *= FNV_PRIME;
    }
    return hash;
}

} // namespace hash
} // namespace fluidloom
