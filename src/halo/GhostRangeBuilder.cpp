#include "fluidloom/halo/GhostRangeBuilder.h"
#include "fluidloom/common/mpi/MPIEnvironment.h"
#include "fluidloom/common/Logger.h"
#include <algorithm>

namespace fluidloom {
namespace halo {

GhostRangeBuilder::GhostRangeBuilder() {
    auto& env = mpi::MPIEnvironment::getInstance();
    m_topology.rank = env.getRank();
    m_topology.size = env.getSize();
    m_topology.prev_rank = (m_topology.rank > 0) ? m_topology.rank - 1 : -1;
    m_topology.next_rank = (m_topology.rank < m_topology.size - 1) ? m_topology.rank + 1 : -1;
}

void GhostRangeBuilder::buildGlobalTopology(hilbert::HilbertIndex local_min, hilbert::HilbertIndex local_max) {
    m_topology.local_min_idx = local_min;
    m_topology.local_max_idx = local_max;
    
    auto& env = mpi::MPIEnvironment::getInstance();
    int size = env.getSize();
    
    // Gather all ranges
    struct Range {
        hilbert::HilbertIndex min;
        hilbert::HilbertIndex max;
    };
    
    Range local_range = {local_min, local_max};
    std::vector<Range> global_ranges(size);
    
    MPI_Allgather(
        &local_range, sizeof(Range), MPI_BYTE,
        global_ranges.data(), sizeof(Range), MPI_BYTE,
        MPI_COMM_WORLD
    );
    
    m_global_ranges.clear();
    for (const auto& r : global_ranges) {
        m_global_ranges.push_back({r.min, r.max});
    }
    
    FL_LOG(INFO) << "Built global topology. Local range: [" << local_min << ", " << local_max << "]";
}

int GhostRangeBuilder::findOwnerRank(hilbert::HilbertIndex idx) const {
    for (int i = 0; i < m_topology.size; ++i) {
        if (idx >= m_global_ranges[i].first && idx <= m_global_ranges[i].second) {
            return i;
        }
    }
    return -1;
}

std::vector<GhostCandidate> GhostRangeBuilder::identifyGhostCandidates(
    const std::vector<hilbert::HilbertIndex>& local_cells,
    int halo_depth
) {
    (void)halo_depth; // Unused for now
    std::vector<GhostCandidate> candidates;
    if (local_cells.empty()) return candidates;
    
    // Simplified logic: Check boundaries
    // In a real implementation, we would check neighbors of boundary cells
    // For now, we'll request a range from neighbors if we are at the edge
    
    // TODO: Implement sophisticated neighbor search using Hilbert curve properties
    // This is a placeholder for the actual geometric neighbor finding
    
    return candidates;
}

} // namespace halo
} // namespace fluidloom
