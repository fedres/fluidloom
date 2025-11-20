// @stable - Module 7 API frozen
#pragma once

#include <cstdint>
#include <vector>
#include <string>

namespace fluidloom {
namespace halo {

// MPI Tags for communication
enum class MPITag : int {
    GHOST_EXCHANGE = 100,
    GLOBAL_REDUCE = 101,
    SYNC_MARKER = 102
};

// Direction of halo exchange relative to current rank
enum class HaloDirection {
    PREV_RANK, // Send to rank - 1
    NEXT_RANK  // Send to rank + 1
};

// Candidate range identified during local topology analysis
struct GhostCandidate {
    uint64_t start_idx;
    uint64_t end_idx;
    int target_rank;
};

// Global topology information
struct GlobalTopology {
    int rank;
    int size;
    uint64_t local_min_idx; // Min Hilbert index owned by this rank
    uint64_t local_max_idx; // Max Hilbert index owned by this rank
    
    // Neighbors
    int prev_rank; // -1 if none
    int next_rank; // -1 if none
};

/**
 * @brief Immutable descriptor for a contiguous range of halo cells to exchange
 * 
 * This struct is the **canonical source of truth** for inter-GPU boundaries.
 * Once computed by GhostRangeBuilder (Module 5), it is read-only for entire simulation.
 * The pack_offset and interpolation metadata are consumed directly by OpenCL kernels.
 */
struct alignas(64) GhostRange {
    // Global Hilbert space boundaries (inclusive start, exclusive end)
    uint64_t hilbert_start{0};
    uint64_t hilbert_end{0};
    
    // Remote GPU identification
    int32_t target_gpu{-1};          // MPI rank or device ID
    int32_t target_device_id{-1};    // OpenCL device ID on remote rank
    
    // Local buffer metadata
    size_t pack_offset{0};           // Byte offset in pack buffer (populated by PackBufferManager)
    size_t pack_size_bytes{0};       // Total bytes for this range (num_cells * field_size)
    
    // Topology information
    uint8_t local_level{0};          // Level of cells on this GPU
    uint8_t remote_level{0};         // Level of cells on remote GPU
    bool requires_interpolation{false}; // True if local_level != remote_level
    
    // Interpolation parameters
    enum class InterpolationType : uint8_t {
        NONE = 0,
        TRILINEAR,               // Coarse → Fine (gather 2x2x2 coarse cells)
        VOLUME_WEIGHTED_AVERAGE  // Fine → Coarse (average 8 fine children)
    };
    InterpolationType interpolation_type{InterpolationType::NONE};
    
    // Cached cell indices (populated during pack setup)
    struct CachedIndices {
        std::vector<uint32_t> local_cell_indices;    // Indices into SOA arrays
        std::vector<uint32_t> neighbor_cell_indices; // For interpolation source cells
        size_t num_cells{0};
    };
    CachedIndices cached;
    
    // Methods
    GhostRange() = default;
    ~GhostRange() = default;
    
    // Calculate number of cells in this range
    size_t numCells() const {
        return cached.num_cells;
    }
    
    // Validate range integrity
    bool validate() const {
        return hilbert_start < hilbert_end && 
               target_gpu >= 0 && 
               pack_offset % 64 == 0; // Must be cache-line aligned
    }
    
    // Generate unique range ID for debugging
    std::string getRangeId() const;
    
    // For debugging
    std::string toString() const;
};

// Hash for unordered_map (used by HaloExchangeManager)
struct GhostRangeHash {
    size_t operator()(const GhostRange& gr) const noexcept {
        return std::hash<uint64_t>{}(gr.hilbert_start ^ gr.hilbert_end);
    }
};

// Equality for unordered_map
struct GhostRangeEqual {
    bool operator()(const GhostRange& a, const GhostRange& b) const noexcept {
        return a.hilbert_start == b.hilbert_start && 
               a.hilbert_end == b.hilbert_end &&
               a.target_gpu == b.target_gpu;
    }
};

} // namespace halo
} // namespace fluidloom
