# Parsing library with ANTLR4

# Add cmake module path
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

# Find ANTLR4
find_package(ANTLR4 REQUIRED)

message(STATUS "ANTLR4 include: ${ANTLR4_INCLUDE_DIRS}")
message(STATUS "ANTLR4 library: ${ANTLR4_LIBRARIES}")
message(STATUS "ANTLR4 jar: ${ANTLR4_JAR}")

# Generated parser directory
set(GENERATED_PARSER_DIR ${CMAKE_SOURCE_DIR}/generated/parsers)

# Compile ANTLR4 grammars (Fields and Lattices - already working)
set(GRAMMAR_DIR ${CMAKE_SOURCE_DIR}/src/parsing/grammars)

antlr4_generate(
    GRAMMARS 
        ${GRAMMAR_DIR}/FluidLoomFieldsLexer.g4
        ${GRAMMAR_DIR}/FluidLoomFieldsParser.g4
    OUTPUT_DIR ${GENERATED_PARSER_DIR}
    VISITOR
)

antlr4_generate(
    GRAMMARS
        ${GRAMMAR_DIR}/FluidLoomLatticesLexer.g4
        ${GRAMMAR_DIR}/FluidLoomLatticesParser.g4
    OUTPUT_DIR ${GENERATED_PARSER_DIR}
    VISITOR
)

# Simulation grammar (for .fl files)
antlr4_generate(
    GRAMMARS
        ${GRAMMAR_DIR}/FluidLoomKernelLexer.g4
        ${GRAMMAR_DIR}/FluidLoomKernelParser.g4
        ${GRAMMAR_DIR}/FluidLoomSimulationLexer.g4
        ${GRAMMAR_DIR}/FluidLoomSimulationParser.g4
    OUTPUT_DIR ${GENERATED_PARSER_DIR}
    VISITOR
)

# Collect all generated sources
file(GLOB GENERATED_SOURCES
    ${GENERATED_PARSER_DIR}/*.cpp
    ${GENERATED_PARSER_DIR}/src/parsing/grammars/*.cpp
)

# Parsing library sources
set(PARSING_SOURCES
    registry/LatticeDescriptor.cpp
    registry/LatticeRegistry.cpp
    registry/ConstantRegistry.cpp
    codegen/OpenCLPreambleGenerator.cpp
    visitors/FieldsVisitor.cpp
    visitors/LatticesVisitor.cpp
    # Module 10 additions
    ast/ExpressionAST.cpp
    ast/StatementAST.cpp
    codegen/OpenCLGenerator.cpp
    Parser.cpp
    ParseError.cpp
    # Phase 4: Symbol table and semantic analysis
    symbol_table/SymbolTable.cpp
    semantic/SemanticAnalyzer.cpp
    # Phase 6: Advanced code generation
    codegen/TypeResolver.cpp
    # Module 10: Simulation builder
    SimulationBuilder.cpp
)

# Create parsing library
add_library(fluidloom_parsing STATIC
    ${PARSING_SOURCES}
    ${GENERATED_SOURCES}
)

target_include_directories(fluidloom_parsing PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${ANTLR4_INCLUDE_DIRS}
    ${GENERATED_PARSER_DIR}
    ${GENERATED_PARSER_DIR}/src/parsing/grammars
    ${CMAKE_SOURCE_DIR}/src/parsing/grammars
)

target_link_libraries(fluidloom_parsing PUBLIC
    ${ANTLR4_LIBRARIES}
    fluidloom_core_objects
)

# CLI parse tool
add_executable(fluidloom-parse
    cli/ParseTool.cpp
)

target_link_libraries(fluidloom-parse
    fluidloom_parsing
    fluidloom_core_objects
)

target_include_directories(fluidloom-parse PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# Simulation runner
add_executable(fluidloom-run
    cli/SimulationRunner.cpp
)

target_link_libraries(fluidloom-run
    fluidloom_parsing
    fluidloom_core_objects
    fluidloom_adaptation
    fluidloom_runtime_objects
    ${ANTLR4_LIBRARIES}
    ${OpenCL_LIBRARIES}
)

target_include_directories(fluidloom-run PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${GENERATED_PARSER_DIR}
)
