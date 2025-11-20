#include "fluidloom/parsing/visitors/FieldsVisitor.h"
#include "fluidloom/parsing/visitors/LatticesVisitor.h"
#include "fluidloom/parsing/codegen/OpenCLPreambleGenerator.h"
#include <iostream>
#include <chrono>
#include <fstream>
#include <filesystem>

using namespace std::chrono;
namespace fs = std::filesystem;

void createTestFieldsFile(const std::string& filename, int num_fields) {
    std::ofstream file(filename);
    for (int i = 0; i < num_fields; ++i) {
        file << "field test" << i << ": scalar float { halo_depth: 1 }\n";
    }
    file.close();
}

void createTestLatticesFile(const std::string& filename, int num_lattices) {
    std::ofstream file(filename);
    for (int i = 0; i < num_lattices; ++i) {
        file << "export lattice D1Q" << (i+2) << " {\n";
        file << "    stencil: [(0,0,0), (1,0,0)],\n";
        file << "    weights: [0.5, 0.5],\n";
        file << "    cs2: 1/3\n";
        file << "}\n";
    }
    file.close();
}

double measureParseFields(const std::string& filename) {
    auto start = high_resolution_clock::now();
    
    fluidloom::parsing::FieldsVisitor visitor;
    visitor.parseFile(filename);
    
    auto end = high_resolution_clock::now();
    return duration_cast<microseconds>(end - start).count() / 1000.0; // ms
}

double measureParseLattices(const std::string& filename) {
    auto start = high_resolution_clock::now();
    
    fluidloom::parsing::LatticesVisitor visitor;
    visitor.parseFile(filename);
    
    auto end = high_resolution_clock::now();
    return duration_cast<microseconds>(end - start).count() / 1000.0; // ms
}

double measureGeneratePreamble() {
    auto start = high_resolution_clock::now();
    
    fluidloom::parsing::OpenCLPreambleGenerator generator;
    std::string preamble = generator.generate();
    
    auto end = high_resolution_clock::now();
    return duration_cast<microseconds>(end - start).count() / 1000.0; // ms
}

int main() {
    fs::path temp_dir = fs::temp_directory_path() / "fluidloom_perf";
    fs::create_directories(temp_dir);
    
    std::cout << "Module 6 Performance Baseline Measurements\n";
    std::cout << "==========================================\n\n";
    
    // Test 1: Parse 10 fields
    std::string fields_file = (temp_dir / "test_fields.fl").string();
    createTestFieldsFile(fields_file, 10);
    
    double fields_time = 0;
    for (int i = 0; i < 5; ++i) {
        fluidloom::registry::FieldRegistry::instance().clear();
        fields_time += measureParseFields(fields_file);
    }
    fields_time /= 5.0;
    
    std::cout << "Parse fields.fl (10 fields): " << fields_time << " ms";
    std::cout << (fields_time < 50.0 ? " ✓ PASS" : " ✗ FAIL") << "\n";
    std::cout << "  Target: < 50ms\n\n";
    
    // Test 2: Parse 2 lattices
    std::string lattices_file = (temp_dir / "test_lattices.fl").string();
    createTestLatticesFile(lattices_file, 2);
    
    double lattices_time = 0;
    for (int i = 0; i < 5; ++i) {
        fluidloom::parsing::LatticeRegistry::getInstance().clear();
        fluidloom::parsing::ConstantRegistry::getInstance().clear();
        lattices_time += measureParseLattices(lattices_file);
    }
    lattices_time /= 5.0;
    
    std::cout << "Parse lattices.fl (2 lattices): " << lattices_time << " ms";
    std::cout << (lattices_time < 20.0 ? " ✓ PASS" : " ✗ FAIL") << "\n";
    std::cout << "  Target: < 20ms\n\n";
    
    // Test 3: Generate preamble
    double preamble_time = 0;
    for (int i = 0; i < 10; ++i) {
        preamble_time += measureGeneratePreamble();
    }
    preamble_time /= 10.0;
    
    std::cout << "Generate preamble: " << preamble_time << " ms";
    std::cout << (preamble_time < 10.0 ? " ✓ PASS" : " ✗ FAIL") << "\n";
    std::cout << "  Target: < 10ms\n\n";
    
    // Cleanup
    fs::remove_all(temp_dir);
    
    // Write results to JSON
    std::ofstream json("../perf/module6_baseline.json");
    json << "{\n";
    json << "  \"module\": \"Module 6: DSL Parser\",\n";
    json << "  \"date\": \"2025-11-19\",\n";
    json << "  \"measurements\": {\n";
    json << "    \"parse_fields_10\": {\n";
    json << "      \"description\": \"Parse fields.fl with 10 field definitions\",\n";
    json << "      \"target\": \"< 50ms\",\n";
    json << "      \"actual\": \"" << fields_time << " ms\",\n";
    json << "      \"pass\": " << (fields_time < 50.0 ? "true" : "false") << "\n";
    json << "    },\n";
    json << "    \"parse_lattices_2\": {\n";
    json << "      \"description\": \"Parse lattices.fl with 2 lattice definitions\",\n";
    json << "      \"target\": \"< 20ms\",\n";
    json << "      \"actual\": \"" << lattices_time << " ms\",\n";
    json << "      \"pass\": " << (lattices_time < 20.0 ? "true" : "false") << "\n";
    json << "    },\n";
    json << "    \"generate_preamble\": {\n";
    json << "      \"description\": \"Generate OpenCL preamble\",\n";
    json << "      \"target\": \"< 10ms\",\n";
    json << "      \"actual\": \"" << preamble_time << " ms\",\n";
    json << "      \"pass\": " << (preamble_time < 10.0 ? "true" : "false") << "\n";
    json << "    }\n";
    json << "  }\n";
    json << "}\n";
    json.close();
    
    std::cout << "Results written to perf/module6_baseline.json\n";
    
    return 0;
}
