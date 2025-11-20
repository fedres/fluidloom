#include "fluidloom/parsing/Parser.h"
#include <iostream>

int main() {
    std::string source = R"(
kernel TestKernel: {
    reads: density, velocity,
    writes: populations,
    halo: 1,
    script: |
        populations[0] = density[0] + 1
    |
}
)";
    
    fluidloom::parsing::Parser parser;
    try {
        auto kernel = parser.parseKernel(source);
        std::cout << "Success! Kernel name: " << kernel->getName() << std::endl;
        std::cout << "Read fields: " << kernel->getReadFields().size() << std::endl;
        std::cout << "Write fields: " << kernel->getWriteFields().size() << std::endl;
        std::cout << "Halo depth: " << (int)kernel->getHaloDepth() << std::endl;
        std::cout << "Statements: " << kernel->getStatements().size() << std::endl;
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
