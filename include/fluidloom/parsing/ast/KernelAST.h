#pragma once
// @stable - Kernel AST for DSL parser

#include "fluidloom/parsing/ast/StatementAST.h"
#include <string>
#include <vector>
#include <memory>

namespace fluidloom {
namespace parsing {
namespace ast {

/**
 * @brief Kernel definition AST
 * 
 * Represents a complete kernel with metadata and script body.
 */
class KernelAST {
public:
    // Kernel metadata
    std::string name;
    std::vector<std::string> read_fields;
    std::vector<std::string> write_fields;
    uint8_t halo_depth = 0;
    std::string execution_mask;
    
    // Script body
    std::vector<std::shared_ptr<Statement>> statements;
    
    // Source location
    Expression::SourceLoc loc;
    
    KernelAST(std::string kernel_name) : name(std::move(kernel_name)) {}
    
    // Accessors
    const std::string& getName() const { return name; }
    const std::vector<std::string>& getReadFields() const { return read_fields; }
    const std::vector<std::string>& getWriteFields() const { return write_fields; }
    uint8_t getHaloDepth() const { return halo_depth; }
    const std::string& getExecutionMask() const { return execution_mask; }
    const std::vector<std::shared_ptr<Statement>>& getStatements() const { return statements; }
    
    // Mutators (for parser)
    void setReadFields(std::vector<std::string> fields) { read_fields = std::move(fields); }
    void setWriteFields(std::vector<std::string> fields) { write_fields = std::move(fields); }
    void setHaloDepth(uint8_t depth) { halo_depth = depth; }
    void setExecutionMask(std::string mask) { execution_mask = std::move(mask); }
    void setStatements(std::vector<std::shared_ptr<Statement>> stmts) { statements = std::move(stmts); }
};

} // namespace ast
} // namespace parsing
} // namespace fluidloom
