#pragma once
// Semantic analyzer for validating AST

#include "fluidloom/parsing/ast/KernelAST.h"
#include "fluidloom/parsing/symbol_table/SymbolTable.h"
#include "fluidloom/core/registry/FieldRegistry.h"
#include <string>
#include <vector>

namespace fluidloom {
namespace parsing {
namespace semantic {

/**
 * @brief Semantic error information
 */
struct SemanticError {
    std::string message;
    size_t line = 0;
    size_t column = 0;
    
    SemanticError(std::string msg, size_t l = 0, size_t c = 0)
        : message(std::move(msg)), line(l), column(c) {}
    
    std::string toString() const;
};

/**
 * @brief Semantic analyzer for AST validation
 */
class SemanticAnalyzer {
private:
    symbol_table::SymbolTable symbol_table;
    registry::FieldRegistry* field_registry = nullptr;
    std::vector<SemanticError> errors;
    
public:
    SemanticAnalyzer() = default;
    
    /**
     * @brief Set field registry for validation
     */
    void setFieldRegistry(registry::FieldRegistry* registry) {
        field_registry = registry;
    }
    
    /**
     * @brief Analyze a kernel AST
     * @return true if no errors, false otherwise
     */
    bool analyzeKernel(const ast::KernelAST& kernel);
    
    /**
     * @brief Validate field references in kernel
     */
    bool validateFieldReferences(const ast::KernelAST& kernel);
    
    /**
     * @brief Compute transitive halo depth
     */
    uint8_t computeHaloDepth(const ast::KernelAST& kernel);
    
    /**
     * @brief Get errors
     */
    const std::vector<SemanticError>& getErrors() const { return errors; }
    
    /**
     * @brief Clear errors
     */
    void clearErrors() { errors.clear(); }
    
    /**
     * @brief Get symbol table
     */
    symbol_table::SymbolTable& getSymbolTable() { return symbol_table; }
    
private:
    void analyzeExpression(const ast::Expression& expr);
    void analyzeStatement(const ast::Statement& stmt);
    void addError(const std::string& message, size_t line = 0, size_t column = 0);
};

} // namespace semantic
} // namespace parsing
} // namespace fluidloom
