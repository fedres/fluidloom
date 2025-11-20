#include "fluidloom/parsing/visitors/FieldsVisitor.h"
#include "fluidloom/parsing/visitors/LatticesVisitor.h"
#include "fluidloom/parsing/codegen/OpenCLPreambleGenerator.h"
#include "fluidloom/common/Logger.h"
#include <iostream>
#include <filesystem>
#include <cstring>

namespace fs = std::filesystem;

void printUsage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [options] <fields.fl> <lattices.fl>\n";
    std::cout << "\nOptions:\n";
    std::cout << "  -o <dir>          Output directory (default: ./generated)\n";
    std::cout << "  --validate-only   Only validate, don't generate code\n";
    std::cout << "  -h, --help        Show this help message\n";
    std::cout << "\nExample:\n";
    std::cout << "  " << program_name << " -o build/generated fields.fl lattices.fl\n";
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }
    
    std::string output_dir = "./generated";
    bool validate_only = false;
    std::string fields_file;
    std::string lattices_file;
    
    // Parse arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "-o") {
            if (i + 1 < argc) {
                output_dir = argv[++i];
            } else {
                std::cerr << "Error: -o requires an argument\n";
                return 1;
            }
        } else if (arg == "--validate-only") {
            validate_only = true;
        } else {
            if (fields_file.empty()) {
                fields_file = arg;
            } else if (lattices_file.empty()) {
                lattices_file = arg;
            } else {
                std::cerr << "Error: Too many input files\n";
                return 1;
            }
        }
    }
    
    if (fields_file.empty() || lattices_file.empty()) {
        std::cerr << "Error: Both fields.fl and lattices.fl are required\n";
        printUsage(argv[0]);
        return 1;
    }
    
    // Validate input files exist
    if (!fs::exists(fields_file)) {
        std::cerr << "Error: File not found: " << fields_file << "\n";
        return 1;
    }
    if (!fs::exists(lattices_file)) {
        std::cerr << "Error: File not found: " << lattices_file << "\n";
        return 1;
    }
    
    try {
        // Parse fields
        std::cout << "Parsing " << fields_file << "...\n";
        fluidloom::parsing::FieldsVisitor fields_visitor;
        fields_visitor.parseFile(fields_file);
        
        if (fields_visitor.hasErrors()) {
            std::cerr << "Errors in " << fields_file << ":\n";
            for (const auto& error : fields_visitor.getErrors()) {
                std::cerr << "  " << error.toString() << "\n";
            }
            return 1;
        }
        
        // Parse lattices
        std::cout << "Parsing " << lattices_file << "...\n";
        fluidloom::parsing::LatticesVisitor lattices_visitor;
        lattices_visitor.parseFile(lattices_file);
        
        if (lattices_visitor.hasErrors()) {
            std::cerr << "Errors in " << lattices_file << ":\n";
            for (const auto& error : lattices_visitor.getErrors()) {
                std::cerr << "  " << error.toString() << "\n";
            }
            return 1;
        }
        
        std::cout << "Validation successful!\n";
        
        if (!validate_only) {
            // Create output directory
            fs::create_directories(output_dir);
            
            // Generate OpenCL preamble
            std::cout << "Generating OpenCL preamble...\n";
            fluidloom::parsing::OpenCLPreambleGenerator generator;
            
            std::string preamble_path = output_dir + "/fluidloom_preamble.cl";
            if (generator.generateToFile(preamble_path)) {
                std::cout << "Generated: " << preamble_path << "\n";
            } else {
                std::cerr << "Error: Failed to write preamble file\n";
                return 1;
            }
        }
        
        std::cout << "Done!\n";
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
