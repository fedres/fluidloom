#pragma once

#include <cstdint>

namespace fluidloom {
namespace hilbert {
namespace internal {

// Extract N bits from position P in value V
static inline uint32_t extractBits(uint32_t V, uint8_t P, uint8_t N) {
    return (V >> P) & ((1u << N) - 1);
}

// Insert N bits from value V into position P in word W
static inline uint32_t insertBits(uint32_t W, uint32_t V, uint8_t P, uint8_t N) {
    uint32_t mask = ((1u << N) - 1) << P;
    return (W & ~mask) | ((V << P) & mask);
}

// Branchless conditional: returns a if cond is true, else b
static inline uint32_t select(uint32_t cond, uint32_t a, uint32_t b) {
    return (a & -cond) | (b & ~(-cond));
}

// Rotate 3-bit state by 1 bit left (branchless)
static inline uint8_t rotateLeft(uint8_t state) {
    return ((state << 1) | (state >> 2)) & 0b111;
}

// Rotate 3-bit state by 1 bit right (branchless)
static inline uint8_t rotateRight(uint8_t state) {
    return ((state >> 1) | (state << 2)) & 0b111;
}

} // namespace internal
} // namespace hilbert
} // namespace fluidloom
