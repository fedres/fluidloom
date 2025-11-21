#pragma once

#include &lt;string&gt;
#include &lt;vector&gt;
#include &lt;unordered_map&gt;
#include &lt;cstdint&gt;
#include &lt;functional&gt;

namespace fluidloom {
namespace testing {

/**
 * @brief Manages gold (reference) data for regression testing
 * 
 * Gold data is stored as binary files with metadata JSON:
 * - Format: .gold (binary) + .gold.json (metadata)
 * - Contains: Field arrays, cell coordinates, domain metadata
 * - Versioned: Each gold file has a schema version for backward compatibility
 */
class GoldDataManager {
public:
    static constexpr uint32_t GOLD_DATA_VERSION = 3;
    
    struct GoldMetadata {
        uint32_t version;
        uint64_t num_cells;
        uint64_t timestep;
        double simulation_time;
        std::string test_name;
        std::string backend_used;
        std::vector&lt;std::string&gt; field_names;
        std::unordered_map&lt;std::string, double&gt; field_norms; // L2 norms for tolerance scaling
        std::string git_commit_hash;
        std::string hostname;
        uint64_t timestamp_unix;
    };
    
    // Save current simulation state as gold data
    static void saveGoldData(
        const std::string&amp; output_dir,
        const std::string&amp; test_name,
        const std::vector&lt;std::string&gt;&amp; field_names,
        const std::vector&lt;std::vector&lt;float&gt;&gt;&amp; field_data,
        uint64_t timestep,
        double sim_time
    );
    
    // Load gold data for comparison
    static std::vector&lt;float&gt; loadGoldField(
        const std::string&amp; gold_file_path,
        const std::string&amp; field_name,
        size_t&amp; expected_size
    );
    
    // Validate gold data integrity
    static bool verifyGoldDataIntegrity(
        const std::string&amp; gold_file_path,
        std::string&amp; error_message
    );
    
    // Update gold data (when intentional changes are made)
    static void updateGoldData(
        const std::string&amp; test_output_path,
        const std::string&amp; gold_data_dir
    );
    
    // Generate gold data from analytical solution
    static void generateAnalyticalGold(
        const std::string&amp; gold_data_dir,
        const std::string&amp; test_name,
        std::function&lt;double(double,double,double,double)&gt; analytical,
        size_t nx, size_t ny, size_t nz,
        uint64_t timestep,
        double t
    );
    
private:
    // Binary file format:
    // [Header: 32 bytes]
    //   - Magic number: 0x474F4C44 ("GOLD")
    //   - Version: uint32_t
    //   - Num cells: uint64_t
    //   - Num fields: uint32_t
    // [Field data: N * (num_cells * bytes_per_component) ]
    //   - Each field stored contiguously, SOA format
    // [Metadata JSON: variable length, appended at end with offset in header]
    
    static constexpr uint32_t MAGIC_NUMBER = 0x474F4C44;
    static constexpr size_t HEADER_SIZE = 64;
    
    struct FileHeader {
        uint32_t magic;
        uint32_t version;
        uint64_t num_cells;
        uint32_t num_fields;
        uint64_t metadata_offset; // From start of file
        uint64_t checksum; // CRC32 of field data
        uint8_t reserved[32]; // For future use
    };
};

} // namespace testing
} // namespace fluidloom
