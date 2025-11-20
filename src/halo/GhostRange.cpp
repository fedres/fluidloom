#include "fluidloom/halo/GhostRange.h"
#include <sstream>
#include <iomanip>

namespace fluidloom {
namespace halo {

std::string GhostRange::getRangeId() const {
    std::ostringstream oss;
    oss << "GR_" << target_gpu << "_" 
        << std::hex << std::setw(16) << std::setfill('0') << hilbert_start;
    return oss.str();
}

std::string GhostRange::toString() const {
    std::ostringstream oss;
    oss << "GhostRange{";
    oss << "hilbert=[" << hilbert_start << "," << hilbert_end << "), ";
    oss << "target_gpu=" << target_gpu << ", ";
    oss << "levels=[" << static_cast<int>(local_level) << "→" << static_cast<int>(remote_level) << "], ";
    oss << "num_cells=" << cached.num_cells << ", ";
    oss << "pack_offset=" << pack_offset << ", ";
    oss << "pack_size=" << pack_size_bytes << ", ";
    oss << "interp=" << (requires_interpolation ? "yes" : "no");
    oss << "}";
    return oss.str();
}

} // namespace halo
} // namespace fluidloom
