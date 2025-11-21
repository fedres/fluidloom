#include "fluidloom/geometry/SimpleSTLVoxelizer.h"
#include "fluidloom/common/Logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <limits>
#include <cstring>

namespace fluidloom {
namespace geometry {

bool SimpleSTLVoxelizer::loadSTL(const std::string& filename) {
    m_triangles.clear();
    m_mesh_min_x = m_mesh_min_y = m_mesh_min_z = std::numeric_limits<float>::max();
    m_mesh_max_x = m_mesh_max_y = m_mesh_max_z = std::numeric_limits<float>::lowest();
    
    // Try binary first (most common)
    if (loadBinarySTL(filename)) {
        FL_LOG(INFO) << "Loaded binary STL: " << filename << " (" << m_triangles.size() << " triangles)";
        return true;
    }
    
    // Fallback to ASCII
    if (loadAsciiSTL(filename)) {
        FL_LOG(INFO) << "Loaded ASCII STL: " << filename << " (" << m_triangles.size() << " triangles)";
        return true;
    }
    
    FL_LOG(ERROR) << "Failed to load STL file: " << filename;
    return false;
}

bool SimpleSTLVoxelizer::loadBinarySTL(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) return false;
    
    // Skip header (80 bytes)
    file.seekg(80);
    
    uint32_t num_triangles = 0;
    file.read(reinterpret_cast<char*>(&num_triangles), sizeof(uint32_t));
    
    if (file.eof()) return false;
    
    m_triangles.reserve(num_triangles);
    
    for (uint32_t i = 0; i < num_triangles; ++i) {
        Triangle tri;
        float buffer[12]; // normal + 3 vertices
        uint16_t attr_byte_count;
        
        file.read(reinterpret_cast<char*>(buffer), 12 * sizeof(float));
        file.read(reinterpret_cast<char*>(&attr_byte_count), sizeof(uint16_t));
        
        // Copy data
        std::memcpy(tri.normal, &buffer[0], 3 * sizeof(float));
        std::memcpy(tri.v1, &buffer[3], 3 * sizeof(float));
        std::memcpy(tri.v2, &buffer[6], 3 * sizeof(float));
        std::memcpy(tri.v3, &buffer[9], 3 * sizeof(float));
        
        tri.computeBounds();
        m_triangles.push_back(tri);
        
        // Update mesh bounds
        m_mesh_min_x = std::min(m_mesh_min_x, tri.min_x);
        m_mesh_max_x = std::max(m_mesh_max_x, tri.max_x);
        m_mesh_min_y = std::min(m_mesh_min_y, tri.min_y);
        m_mesh_max_y = std::max(m_mesh_max_y, tri.max_y);
        m_mesh_min_z = std::min(m_mesh_min_z, tri.min_z);
        m_mesh_max_z = std::max(m_mesh_max_z, tri.max_z);
    }
    
    return true;
}

bool SimpleSTLVoxelizer::loadAsciiSTL(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) return false;
    
    std::string line;
    std::string token;
    
    // Check for "solid" keyword
    std::getline(file, line);
    if (line.find("solid") == std::string::npos) return false;
    
    Triangle current_tri;
    int vertex_idx = 0;
    
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        ss >> token;
        
        if (token == "facet") {
            ss >> token; // normal
            ss >> current_tri.normal[0] >> current_tri.normal[1] >> current_tri.normal[2];
            vertex_idx = 0;
        } else if (token == "vertex") {
            if (vertex_idx == 0) ss >> current_tri.v1[0] >> current_tri.v1[1] >> current_tri.v1[2];
            else if (vertex_idx == 1) ss >> current_tri.v2[0] >> current_tri.v2[1] >> current_tri.v2[2];
            else if (vertex_idx == 2) ss >> current_tri.v3[0] >> current_tri.v3[1] >> current_tri.v3[2];
            vertex_idx++;
        } else if (token == "endfacet") {
            current_tri.computeBounds();
            m_triangles.push_back(current_tri);
            
            // Update mesh bounds
            m_mesh_min_x = std::min(m_mesh_min_x, current_tri.min_x);
            m_mesh_max_x = std::max(m_mesh_max_x, current_tri.max_x);
            m_mesh_min_y = std::min(m_mesh_min_y, current_tri.min_y);
            m_mesh_max_y = std::max(m_mesh_max_y, current_tri.max_y);
            m_mesh_min_z = std::min(m_mesh_min_z, current_tri.min_z);
            m_mesh_max_z = std::max(m_mesh_max_z, current_tri.max_z);
        }
    }
    
    return !m_triangles.empty();
}

std::vector<CellCoord> SimpleSTLVoxelizer::voxelize(
    const GeometryDescriptor& geom,
    float cell_size
) {
    std::vector<CellCoord> cells;
    
    // Transform bounds
    float min_x = m_mesh_min_x * geom.transform.sx + geom.transform.tx;
    float max_x = m_mesh_max_x * geom.transform.sx + geom.transform.tx;
    float min_y = m_mesh_min_y * geom.transform.sy + geom.transform.ty;
    float max_y = m_mesh_max_y * geom.transform.sy + geom.transform.ty;
    float min_z = m_mesh_min_z * geom.transform.sz + geom.transform.tz;
    float max_z = m_mesh_max_z * geom.transform.sz + geom.transform.tz;
    
    // Convert to grid coordinates
    int start_x = static_cast<int>(std::floor(min_x / cell_size));
    int end_x = static_cast<int>(std::ceil(max_x / cell_size));
    int start_y = static_cast<int>(std::floor(min_y / cell_size));
    int end_y = static_cast<int>(std::ceil(max_y / cell_size));
    int start_z = static_cast<int>(std::floor(min_z / cell_size));
    int end_z = static_cast<int>(std::ceil(max_z / cell_size));
    
    FL_LOG(INFO) << "Voxelizing grid: [" << start_x << "," << end_x << "] x ["
                 << start_y << "," << end_y << "] x [" << start_z << "," << end_z << "]";
    
    // March through grid
    // TODO: Parallelize
    for (int z = start_z; z <= end_z; ++z) {
        for (int y = start_y; y <= end_y; ++y) {
            for (int x = start_x; x <= end_x; ++x) {
                // Transform point back to local mesh space for testing
                float wx = (x + 0.5f) * cell_size;
                float wy = (y + 0.5f) * cell_size;
                float wz = (z + 0.5f) * cell_size;
                
                float lx = (wx - geom.transform.tx) / geom.transform.sx;
                float ly = (wy - geom.transform.ty) / geom.transform.sy;
                float lz = (wz - geom.transform.tz) / geom.transform.sz;
                
                if (isInside(lx, ly, lz)) {
                    cells.push_back(CellCoord{x, y, z, 0});
                }
            }
        }
    }
    
    return cells;
}

bool SimpleSTLVoxelizer::isInside(float x, float y, float z) const {
    // Quick bounding box check
    if (x < m_mesh_min_x || x > m_mesh_max_x ||
        y < m_mesh_min_y || y > m_mesh_max_y ||
        z < m_mesh_min_z || z > m_mesh_max_z) {
        return false;
    }
    
    // Ray casting in +X direction
    int intersections = 0;
    
    for (const auto& tri : m_triangles) {
        // Optimization: check if ray can possibly intersect triangle bounds
        if (y < tri.min_y || y > tri.max_y || z < tri.min_z || z > tri.max_z) continue;
        if (x > tri.max_x) continue; // Ray starts after triangle
        
        if (rayIntersectsTriangle(tri, x, y, z, 1.0f, 0.0f, 0.0f)) {
            intersections++;
        }
    }
    
    // Odd number of intersections means point is inside
    return (intersections % 2) == 1;
}

bool SimpleSTLVoxelizer::rayIntersectsTriangle(
    const Triangle& tri,
    float ox, float oy, float oz,
    float dx, float dy, float dz
) const {
    // Möller–Trumbore intersection algorithm
    const float EPSILON = 1e-6f;
    
    float edge1[3], edge2[3], h[3], s[3], q[3];
    float a, f, u, v;
    
    // edge1 = v2 - v1
    edge1[0] = tri.v2[0] - tri.v1[0];
    edge1[1] = tri.v2[1] - tri.v1[1];
    edge1[2] = tri.v2[2] - tri.v1[2];
    
    // edge2 = v3 - v1
    edge2[0] = tri.v3[0] - tri.v1[0];
    edge2[1] = tri.v3[1] - tri.v1[1];
    edge2[2] = tri.v3[2] - tri.v1[2];
    
    // h = ray_dir x edge2
    h[0] = dy * edge2[2] - dz * edge2[1];
    h[1] = dz * edge2[0] - dx * edge2[2];
    h[2] = dx * edge2[1] - dy * edge2[0];
    
    a = edge1[0] * h[0] + edge1[1] * h[1] + edge1[2] * h[2];
    
    if (a > -EPSILON && a < EPSILON) return false; // Ray parallel to triangle
    
    f = 1.0f / a;
    
    // s = ray_origin - v1
    s[0] = ox - tri.v1[0];
    s[1] = oy - tri.v1[1];
    s[2] = oz - tri.v1[2];
    
    u = f * (s[0] * h[0] + s[1] * h[1] + s[2] * h[2]);
    if (u < 0.0f || u > 1.0f) return false;
    
    // q = s x edge1
    q[0] = s[1] * edge1[2] - s[2] * edge1[1];
    q[1] = s[2] * edge1[0] - s[0] * edge1[2];
    q[2] = s[0] * edge1[1] - s[1] * edge1[0];
    
    v = f * (dx * q[0] + dy * q[1] + dz * q[2]);
    if (v < 0.0f || u + v > 1.0f) return false;
    
    // At this stage we can compute t to find out where the intersection point is on the line
    float t = f * (edge2[0] * q[0] + edge2[1] * q[1] + edge2[2] * q[2]);
    
    return t > EPSILON; // Ray intersection
}

} // namespace geometry
} // namespace fluidloom
