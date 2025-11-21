#pragma once

#include <string>
#include <vector>
#include <memory>
#include "fluidloom/core/fields/SOAFieldManager.h"
#include "fluidloom/core/hilbert/CellCoord.h"

namespace fluidloom {
namespace io {

class VTKWriter {
public:
    VTKWriter(const std::string& output_dir);
    
    void writeSnapshot(
        int step,
        double time,
        const std::vector<CellCoord>& cells,
        const fields::SOAFieldManager& field_manager
    );

private:
    std::string m_output_dir;
    
    void writeVTP(
        const std::string& filename,
        double time,
        const std::vector<CellCoord>& cells,
        const fields::SOAFieldManager& field_manager
    );
    
    void writePVTP(
        const std::string& filename,
        double time,
        int num_pieces
    );
    
    // Helper to encode data in Base64 or raw binary (for now we'll use ASCII/Appended for simplicity or just ASCII for V1)
    // Actually, VTP supports ASCII, which is easiest for debugging.
};

} // namespace io
} // namespace fluidloom
