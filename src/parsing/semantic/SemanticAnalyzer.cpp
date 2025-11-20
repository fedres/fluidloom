#include "fluidloom/parsing/semantic/SemanticAnalyzer.h"
#include "fluidloom/parsing/ast/ExpressionAST.h"
#include "fluidloom/parsing/ast/StatementAST.h"
#include <sstream>

namespace fluidloom {
namespace parsing {
namespace semantic {

std::string SemanticError::toString() const {
    std::ostringstream oss;
    if (line > 0) {
        oss << "Semantic error at line " << line << ", column " << column << ": ";
    } else {
        oss << "Semantic error: ";
    }
    oss << message;
    return oss.str();
}

bool SemanticAnalyzer::analyzeKernel(const ast::KernelAST& kernel) {
    clearErrors();
    
    // Add kernel to symbol table
    symbol_table.addKernel(kernel.getName());
    
    // Validate field references
    if (!validateFieldReferences(kernel)) {
        return false;
    }
    
    // Add read fields to symbol table
    for (const auto& field_name : kernel.getReadFields()) {
        if (field_registry) {
            auto field_opt = field_registry->lookupByName(field_name);
            if (field_opt.has_value()) {
                symbol_table.addField(field_name, field_opt->num_components);
            } else {
                addError("Field '" + field_name + "' not found in registry");
            }
        } else {
            // No registry, assume 1 component
            symbol_table.addField(field_name, 1);
        }
    }
    
    // Add write fields to symbol table
    for (const auto& field_name : kernel.getWriteFields()) {
        if (!symbol_table.exists(field_name)) {
            if (field_registry) {
                auto field_opt = field_registry->lookupByName(field_name);
                if (field_opt.has_value()) {
                    symbol_table.addField(field_name, field_opt->num_components);
                } else {
                    addError("Field '" + field_name + "' not found in registry");
                }
            } else {
                symbol_table.addField(field_name, 1);
            }
        }
    }
    
    // Analyze statements
    symbol_table.enterScope();
    for (const auto& stmt : kernel.getStatements()) {
        analyzeStatement(*stmt);
    }
    symbol_table.exitScope();
    
    return errors.empty();
}

bool SemanticAnalyzer::validateFieldReferences(const ast::KernelAST& kernel) {
    if (!field_registry) {
        // No registry to validate against
        return true;
    }
    
    // Check read fields
    for (const auto& field_name : kernel.getReadFields()) {
        auto field_opt = field_registry->lookupByName(field_name);
        if (!field_opt.has_value()) {
            addError("Read field '" + field_name + "' not found in FieldRegistry");
        }
    }
    
    // Check write fields
    for (const auto& field_name : kernel.getWriteFields()) {
        auto field_opt = field_registry->lookupByName(field_name);
        if (!field_opt.has_value()) {
            addError("Write field '" + field_name + "' not found in FieldRegistry");
        }
    }
    
    return errors.empty();
}

uint8_t SemanticAnalyzer::computeHaloDepth(const ast::KernelAST& kernel) {
    // For now, just return the declared halo depth
    // In a full implementation, this would analyze neighbor accesses
    return kernel.getHaloDepth();
}

void SemanticAnalyzer::analyzeExpression(const ast::Expression& expr) {
    (void)expr;  // TODO: Implement full expression analysis
    // Basic expression analysis
    // In a full implementation, this would:
    // - Type check expressions
    // - Validate variable references
    // - Check function calls
    
    // For now, just a placeholder
}

void SemanticAnalyzer::analyzeStatement(const ast::Statement& stmt) {
    (void)stmt;  // TODO: Implement full statement analysis
    // Basic statement analysis
    // In a full implementation, this would:
    // - Validate assignments
    // - Check loop bounds
    // - Validate control flow
    
    // For now, just a placeholder
}

void SemanticAnalyzer::addError(const std::string& message, size_t line, size_t column) {
    errors.emplace_back(message, line, column);
}

} // namespace semantic
} // namespace parsing
} // namespace fluidloom
