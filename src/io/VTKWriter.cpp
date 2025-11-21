#include "fluidloom/io/VTKWriter.h"
#include "fluidloom/common/Logger.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>

namespace fluidloom {
namespace io {

VTKWriter::VTKWriter(const std::string& output_dir)
    : m_output_dir(output_dir) {
    // Ensure output directory exists
    if (!std::filesystem::exists(m_output_dir)) {
        std::filesystem::create_directories(m_output_dir);
    }
}

void VTKWriter::writeSnapshot(
    int step,
    double time,
    const std::vector<CellCoord>& cells,
    const fields::SOAFieldManager& field_manager
) {
    std::stringstream ss;
    ss << m_output_dir << "/snapshot_" << std::setfill('0') << std::setw(6) << step << ".vtp";
    std::string filename = ss.str();
    
    FL_LOG(INFO) << "Writing VTK snapshot to " << filename;
    
    writeVTP(filename, time, cells, field_manager);
}

void VTKWriter::writeVTP(
    const std::string& filename,
    double time,
    const std::vector<CellCoord>& cells,
    const fields::SOAFieldManager& field_manager
) {
    (void)time; // Unused for now
    (void)field_manager; // Unused for now

    std::ofstream file(filename);
    if (!file.is_open()) {
        FL_LOG(ERROR) << "Failed to open VTK file for writing: " << filename;
        return;
    }
    
    // Write header
    file << "<?xml version=\"1.0\"?>\n";
    file << "<VTKFile type=\"PolyData\" version=\"0.1\" byte_order=\"LittleEndian\">\n";
    file << "  <PolyData>\n";
    file << "    <Piece NumberOfPoints=\"" << cells.size() << "\" NumberOfVerts=\"" << cells.size() << "\">\n";
    
    // Points (Cell centers)
    file << "      <Points>\n";
    file << "        <DataArray type=\"Float32\" Name=\"Points\" NumberOfComponents=\"3\" format=\"ascii\">\n";
    for (const auto& cell : cells) {
        // Assuming unit spacing for now, or we need to pass spacing/origin
        // Using cell coordinates directly
        file << cell.x << " " << cell.y << " " << cell.z << " ";
    }
    file << "\n        </DataArray>\n";
    file << "      </Points>\n";
    
    // Verts (Topology - each point is a vertex)
    file << "      <Verts>\n";
    file << "        <DataArray type=\"Int64\" Name=\"connectivity\" format=\"ascii\">\n";
    for (size_t i = 0; i < cells.size(); ++i) {
        file << i << " ";
    }
    file << "\n        </DataArray>\n";
    file << "        <DataArray type=\"Int64\" Name=\"offsets\" format=\"ascii\">\n";
    for (size_t i = 0; i < cells.size(); ++i) {
        file << (i + 1) << " ";
    }
    file << "\n        </DataArray>\n";
    file << "      </Verts>\n";
    
    // Point Data (Fields)
    file << "      <PointData>\n";
    
    // Write Level
    file << "        <DataArray type=\"Int32\" Name=\"Level\" format=\"ascii\">\n";
    for (const auto& cell : cells) {
        file << (int)cell.level << " ";
    }
    file << "\n        </DataArray>\n";
    
    // TODO: Iterate over registered fields in FieldManager and write them
    // For now, we need a way to get field data from FieldManager on host
    // This requires FieldManager to have a "copyToHost" or similar, or we assume data is already on host.
    // Since FieldManager manages device memory, we likely need to pull data back.
    
    file << "      </PointData>\n";
    
    file << "    </Piece>\n";
    file << "  </PolyData>\n";
    file << "</VTKFile>\n";
    
    file.close();
}

void VTKWriter::writePVTP(
    const std::string& filename,
    double time,
    int num_pieces
) {
    (void)filename;
    (void)time;
    (void)num_pieces;
    // Implementation for parallel VTP (later)
}

} // namespace io
} // namespace fluidloom
