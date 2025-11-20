#pragma once

#include "fluidloom/halo/GhostRange.h"
#include "fluidloom/core/hilbert/HilbertCodec.h"
#include <vector>
#include <memory>

namespace fluidloom {
namespace halo {

class GhostRangeBuilder {
public:
    GhostRangeBuilder();
    ~GhostRangeBuilder() = default;
    
    // Build global topology by exchanging local ranges with all ranks
    void buildGlobalTopology(hilbert::HilbertIndex local_min, hilbert::HilbertIndex local_max);
    
    // Identify ghost ranges needed from neighbors based on halo depth
    // Returns a list of ranges to request from neighbors
    std::vector<GhostCandidate> identifyGhostCandidates(
        const std::vector<hilbert::HilbertIndex>& local_cells,
        int halo_depth
    );
    
    const GlobalTopology& getTopology() const { return m_topology; }
    
private:
    GlobalTopology m_topology;
    std::vector<std::pair<hilbert::HilbertIndex, hilbert::HilbertIndex>> m_global_ranges;
    
    // Helper to find which rank owns a given Hilbert index
    int findOwnerRank(hilbert::HilbertIndex idx) const;
};

} // namespace halo
} // namespace fluidloom
