#include "fluidloom/parsing/codegen/TypeResolver.h"
#include "fluidloom/parsing/ast/ExpressionAST.h"

namespace fluidloom {
namespace parsing {
namespace codegen {

// Helper visitor class for type resolution
class TypeResolverVisitor : public ast::ExpressionVisitor {
private:
    symbol_table::SymbolTable* symbol_table;
    symbol_table::SymbolType result_type;
    
public:
    TypeResolverVisitor(symbol_table::SymbolTable* table) 
        : symbol_table(table), result_type(symbol_table::SymbolType::UNKNOWN) {}
    
    symbol_table::SymbolType getResultType() const { return result_type; }
    
    void visit(const ast::LiteralExpression& expr) override {
        (void)expr;
        // For now, assume all literals are float
        // In a full implementation, check expr value type
        result_type = symbol_table::SymbolType::FLOAT;
    }
    
    void visit(const ast::VariableExpression& expr) override {
        if (symbol_table) {
            auto sym = symbol_table->lookup(expr.name);
            if (sym.has_value()) {
                result_type = sym->type;
                return;
            }
        }
        result_type = symbol_table::SymbolType::FLOAT;  // Default
    }
    
    void visit(const ast::BinaryExpression& expr) override {
        // Comparison operators return bool
        switch (expr.op) {
            case ast::BinaryExpression::Op::EQ:
            case ast::BinaryExpression::Op::NE:
            case ast::BinaryExpression::Op::LT:
            case ast::BinaryExpression::Op::LE:
            case ast::BinaryExpression::Op::GT:
            case ast::BinaryExpression::Op::GE:
                result_type = symbol_table::SymbolType::BOOL;
                return;
            default:
                break;
        }
        
        // For arithmetic, assume float (proper implementation would check operands)
        result_type = symbol_table::SymbolType::FLOAT;
    }
    
    void visit(const ast::UnaryExpression& expr) override {
        (void)expr;
        result_type = symbol_table::SymbolType::FLOAT;
    }
    
    void visit(const ast::CallExpression& expr) override {
        (void)expr;
        result_type = symbol_table::SymbolType::FLOAT;
    }
    
    void visit(const ast::SubscriptExpression& expr) override {
        (void)expr;
        result_type = symbol_table::SymbolType::FLOAT;
    }
    
    void visit(const ast::VectorLiteralExpression& expr) override {
        (void)expr;
        result_type = symbol_table::SymbolType::VECTOR_19;
    }
    
    void visit(const ast::LambdaExpression& expr) override {
        (void)expr;
        result_type = symbol_table::SymbolType::LAMBDA;
    }
    
    void visit(const ast::MemberExpression& expr) override {
        (void)expr;
        result_type = symbol_table::SymbolType::FLOAT;
    }
};

symbol_table::SymbolType TypeResolver::resolveType(const ast::Expression& expr) {
    TypeResolverVisitor visitor(symbol_table);
    expr.accept(visitor);
    return visitor.getResultType();
}

std::string TypeResolver::getOpenCLType(symbol_table::SymbolType type) {
    switch (type) {
        case symbol_table::SymbolType::FLOAT:
            return "float";
        case symbol_table::SymbolType::DOUBLE:
            return "double";
        case symbol_table::SymbolType::INT:
            return "int";
        case symbol_table::SymbolType::BOOL:
            return "bool";
        case symbol_table::SymbolType::VECTOR_3:
            return "float3";
        case symbol_table::SymbolType::VECTOR_19:
            return "float";  // Individual component
        default:
            return "float";
    }
}

} // namespace codegen
} // namespace parsing
} // namespace fluidloom
