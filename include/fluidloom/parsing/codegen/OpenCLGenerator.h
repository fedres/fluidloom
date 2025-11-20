#pragma once
// @stable - OpenCL code generator from AST

#include "fluidloom/parsing/ast/ExpressionAST.h"
#include "fluidloom/parsing/ast/StatementAST.h"
#include "fluidloom/parsing/ast/KernelAST.h"
#include <sstream>
#include <string>

namespace fluidloom {
namespace parsing {
namespace codegen {

/**
 * @brief Generates OpenCL C code from AST
 */
class OpenCLGenerator : public ast::ExpressionVisitor, public ast::StatementVisitor {
private:
    std::ostringstream code;
    int indent_level = 0;
    
    void writeIndent() {
        for (int i = 0; i < indent_level; ++i) {
            code << "    ";
        }
    }
    
public:
    OpenCLGenerator() = default;
    
    /**
     * @brief Generate complete OpenCL kernel from KernelAST
     */
    std::string generateKernel(const ast::KernelAST& kernel);
    
    /**
     * @brief Generate expression code
     */
    std::string generateExpression(const ast::Expression& expr);
    
    /**
     * @brief Generate statement code
     */
    std::string generateStatement(const ast::Statement& stmt);
    
    // Expression visitor methods
    void visit(const ast::LiteralExpression& expr) override;
    void visit(const ast::VariableExpression& expr) override;
    void visit(const ast::BinaryExpression& expr) override;
    void visit(const ast::UnaryExpression& expr) override;
    void visit(const ast::CallExpression& expr) override;
    void visit(const ast::SubscriptExpression& expr) override;
    void visit(const ast::MemberExpression& expr) override;
    void visit(const ast::VectorLiteralExpression& expr) override;
    void visit(const ast::LambdaExpression& expr) override;
    
    // Statement visitor methods
    void visit(const ast::AssignmentStatement& stmt) override;
    void visit(const ast::ForStatement& stmt) override;
    void visit(const ast::IfStatement& stmt) override;
    void visit(const ast::WhileStatement& stmt) override;
    void visit(const ast::RunStatement& stmt) override;
    void visit(const ast::ReduceStatement& stmt) override;
    void visit(const ast::PlaceGeometryStatement& stmt) override;
    
private:
    std::string getOperatorString(ast::BinaryExpression::Op op);
    std::string getUnaryOperatorString(ast::UnaryExpression::Op op);
};

} // namespace codegen
} // namespace parsing
} // namespace fluidloom
