#include "GoldDataManager.h"
#include &lt;fstream&gt;
#include &lt;sstream&gt;
#include &lt;stdexcept&gt;
#include &lt;cstring&gt;
#include &lt;chrono&gt;

namespace fluidloom {
namespace testing {

void GoldDataManager::saveGoldData(
    const std::string&amp; output_dir,
    const std::string&amp; test_name,
    const std::vector&lt;std::string&gt;&amp; field_names,
    const std::vector&lt;std::vector&lt;float&gt;&gt;&amp; field_data,
    uint64_t timestep,
    double sim_time) {
    
    if (field_names.size() != field_data.size()) {
        throw std::runtime_error("Field names and data size mismatch");
    }
    
    if (field_data.empty()) {
        throw std::runtime_error("No field data to save");
    }
    
    size_t num_cells = field_data[0].size();
    
    // Create output file
    std::string output_path = output_dir + "/" + test_name + ".gold";
    std::ofstream file(output_path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to create gold data file: " + output_path);
    }
    
    // Write header
    FileHeader header;
    std::memset(&amp;header, 0, sizeof(header));
    header.magic = MAGIC_NUMBER;
    header.version = GOLD_DATA_VERSION;
    header.num_cells = num_cells;
    header.num_fields = static_cast&lt;uint32_t&gt;(field_names.size());
    header.metadata_offset = 0; // Will be updated later
    header.checksum = 0; // TODO: Implement CRC32
    
    file.write(reinterpret_cast&lt;const char*&gt;(&amp;header), sizeof(header));
    
    // Write field data
    for (const auto&amp; field : field_data) {
        if (field.size() != num_cells) {
            throw std::runtime_error("Inconsistent field sizes");
        }
        file.write(reinterpret_cast&lt;const char*&gt;(field.data()), 
                   field.size() * sizeof(float));
    }
    
    // Write metadata JSON
    size_t metadata_offset = file.tellp();
    
    std::ostringstream json;
    json &lt;&lt; "{\n";
    json &lt;&lt; "  \"version\": " &lt;&lt; GOLD_DATA_VERSION &lt;&lt; ",\n";
    json &lt;&lt; "  \"test_name\": \"" &lt;&lt; test_name &lt;&lt; "\",\n";
    json &lt;&lt; "  \"num_cells\": " &lt;&lt; num_cells &lt;&lt; ",\n";
    json &lt;&lt; "  \"timestep\": " &lt;&lt; timestep &lt;&lt; ",\n";
    json &lt;&lt; "  \"simulation_time\": " &lt;&lt; sim_time &lt;&lt; ",\n";
    json &lt;&lt; "  \"field_names\": [";
    for (size_t i = 0; i &lt; field_names.size(); ++i) {
        json &lt;&lt; "\"" &lt;&lt; field_names[i] &lt;&lt; "\"";
        if (i &lt; field_names.size() - 1) json &lt;&lt; ", ";
    }
    json &lt;&lt; "],\n";
    json &lt;&lt; "  \"timestamp\": " &lt;&lt; std::chrono::system_clock::now().time_since_epoch().count() &lt;&lt; "\n";
    json &lt;&lt; "}\n";
    
    std::string json_str = json.str();
    file.write(json_str.c_str(), json_str.size());
    
    // Update header with metadata offset
    file.seekp(0);
    header.metadata_offset = metadata_offset;
    file.write(reinterpret_cast&lt;const char*&gt;(&amp;header), sizeof(header));
    
    file.close();
}

std::vector&lt;float&gt; GoldDataManager::loadGoldField(
    const std::string&amp; gold_file_path,
    const std::string&amp; field_name,
    size_t&amp; expected_size) {
    
    std::ifstream file(gold_file_path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open gold data file: " + gold_file_path);
    }
    
    // Read header
    FileHeader header;
    file.read(reinterpret_cast&lt;char*&gt;(&amp;header), sizeof(header));
    
    if (header.magic != MAGIC_NUMBER) {
        throw std::runtime_error("Invalid gold data file: bad magic number");
    }
    
    if (header.version != GOLD_DATA_VERSION) {
        throw std::runtime_error("Gold data version mismatch");
    }
    
    expected_size = header.num_cells;
    
    // For now, just load the first field
    // TODO: Parse metadata to find specific field by name
    std::vector&lt;float&gt; data(header.num_cells);
    file.read(reinterpret_cast&lt;char*&gt;(data.data()), 
              header.num_cells * sizeof(float));
    
    return data;
}

bool GoldDataManager::verifyGoldDataIntegrity(
    const std::string&amp; gold_file_path,
    std::string&amp; error_message) {
    
    try {
        std::ifstream file(gold_file_path, std::ios::binary);
        if (!file) {
            error_message = "Failed to open file";
            return false;
        }
        
        FileHeader header;
        file.read(reinterpret_cast&lt;char*&gt;(&amp;header), sizeof(header));
        
        if (header.magic != MAGIC_NUMBER) {
            error_message = "Invalid magic number";
            return false;
        }
        
        if (header.version != GOLD_DATA_VERSION) {
            error_message = "Version mismatch";
            return false;
        }
        
        // TODO: Verify checksum
        
        return true;
        
    } catch (const std::exception&amp; e) {
        error_message = e.what();
        return false;
    }
}

void GoldDataManager::updateGoldData(
    const std::string&amp; test_output_path,
    const std::string&amp; gold_data_dir) {
    
    // TODO: Implement gold data update logic
    // This would copy test output to gold data directory
    throw std::runtime_error("updateGoldData not yet implemented");
}

void GoldDataManager::generateAnalyticalGold(
    const std::string&amp; gold_data_dir,
    const std::string&amp; test_name,
    std::function&lt;double(double,double,double,double)&gt; analytical,
    size_t nx, size_t ny, size_t nz,
    uint64_t timestep,
    double t) {
    
    // Generate grid
    size_t num_cells = nx * ny * nz;
    std::vector&lt;float&gt; field_data(num_cells);
    
    for (size_t k = 0; k &lt; nz; ++k) {
        for (size_t j = 0; j &lt; ny; ++j) {
            for (size_t i = 0; i &lt; nx; ++i) {
                double x = static_cast&lt;double&gt;(i) / nx;
                double y = static_cast&lt;double&gt;(j) / ny;
                double z = static_cast&lt;double&gt;(k) / nz;
                
                size_t idx = i + j * nx + k * nx * ny;
                field_data[idx] = static_cast&lt;float&gt;(analytical(x, y, z, t));
            }
        }
    }
    
    // Save as gold data
    std::vector&lt;std::string&gt; field_names = {"analytical_field"};
    std::vector&lt;std::vector&lt;float&gt;&gt; fields = {field_data};
    
    saveGoldData(gold_data_dir, test_name, field_names, fields, timestep, t);
}

} // namespace testing
} // namespace fluidloom
