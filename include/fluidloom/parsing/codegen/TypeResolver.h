#pragma once
// Type resolver for expression type inference

#include "fluidloom/parsing/ast/ExpressionAST.h"
#include "fluidloom/parsing/symbol_table/SymbolTable.h"
#include <memory>

namespace fluidloom {
namespace parsing {
namespace codegen {

/**
 * @brief Resolves types of expressions using visitor pattern
 */
class TypeResolver {
private:
    symbol_table::SymbolTable* symbol_table = nullptr;
    
public:
    TypeResolver() = default;
    
    void setSymbolTable(symbol_table::SymbolTable* table) {
        symbol_table = table;
    }
    
    /**
     * @brief Resolve the type of an expression
     */
    symbol_table::SymbolType resolveType(const ast::Expression& expr);
    
    /**
     * @brief Get OpenCL type string
     */
    std::string getOpenCLType(symbol_table::SymbolType type);
};

} // namespace codegen
} // namespace parsing
} // namespace fluidloom
