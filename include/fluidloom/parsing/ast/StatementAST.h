#pragma once
// @stable - Statement AST for DSL parser

#include "fluidloom/parsing/ast/ExpressionAST.h"
#include <vector>
#include <memory>

namespace fluidloom {
namespace parsing {
namespace ast {

// Forward declaration
class StatementVisitor;

/**
 * @brief Base class for all statement AST nodes
 */
class Statement {
public:
    virtual ~Statement() = default;
    virtual void accept(StatementVisitor& visitor) const = 0;
    
    Expression::SourceLoc loc;
};

/**
 * @brief Assignment statement (x = expr, density[i] = value)
 */
class AssignmentStatement : public Statement {
public:
    std::shared_ptr<Expression> target;  // Variable or Subscript
    std::shared_ptr<Expression> value;
    
    AssignmentStatement(std::shared_ptr<Expression> tgt, std::shared_ptr<Expression> val)
        : target(std::move(tgt)), value(std::move(val)) {}
    
    void accept(StatementVisitor& visitor) const override;
};

/**
 * @brief For loop (for i in 0..19)
 */
class ForStatement : public Statement {
public:
    std::string loop_variable;
    std::shared_ptr<Expression> start;
    std::shared_ptr<Expression> end;
    std::vector<std::shared_ptr<Statement>> body;
    
    ForStatement(std::string var, std::shared_ptr<Expression> s, 
                 std::shared_ptr<Expression> e, std::vector<std::shared_ptr<Statement>> b)
        : loop_variable(std::move(var)), start(std::move(s)), end(std::move(e)), body(std::move(b)) {}
    
    void accept(StatementVisitor& visitor) const override;
};

/**
 * @brief If statement (if condition { ... })
 */
class IfStatement : public Statement {
public:
    std::shared_ptr<Expression> condition;
    std::vector<std::shared_ptr<Statement>> then_branch;
    std::vector<std::shared_ptr<Statement>> else_branch;
    
    IfStatement(std::shared_ptr<Expression> cond, 
                std::vector<std::shared_ptr<Statement>> then_stmts,
                std::vector<std::shared_ptr<Statement>> else_stmts = {})
        : condition(std::move(cond)), then_branch(std::move(then_stmts)), else_branch(std::move(else_stmts)) {}
    
    void accept(StatementVisitor& visitor) const override;
};

/**
 * @brief While loop (while condition { ... })
 */
class WhileStatement : public Statement {
public:
    std::shared_ptr<Expression> condition;
    std::vector<std::shared_ptr<Statement>> body;
    
    WhileStatement(std::shared_ptr<Expression> cond, std::vector<std::shared_ptr<Statement>> stmts)
        : condition(std::move(cond)), body(std::move(stmts)) {}
    
    void accept(StatementVisitor& visitor) const override;
};

/**
 * @brief Run kernel statement (run(kernel_name))
 */
class RunStatement : public Statement {
public:
    std::string kernel_name;
    
    explicit RunStatement(std::string name) : kernel_name(std::move(name)) {}
    
    void accept(StatementVisitor& visitor) const override;
};

/**
 * @brief Reduction operation (reduce_sum(expr), reduce_min(expr), reduce_max(expr))
 */
class ReduceStatement : public Statement {
public:
    enum class Op { SUM, MIN, MAX } op;
    std::shared_ptr<Expression> expression;
    std::string target_variable;  // Where to store result
    
    ReduceStatement(Op operation, std::shared_ptr<Expression> expr, std::string target = "")
        : op(operation), expression(std::move(expr)), target_variable(std::move(target)) {}
    
    void accept(StatementVisitor& visitor) const override;
};

/**
 * @brief Geometry placement (place_geometry(...))
 */
class PlaceGeometryStatement : public Statement {
public:
    std::string geometry_file;  // STL file path or empty for implicit
    std::shared_ptr<Expression> implicit_function;  // Lambda for implicit geometry
    std::shared_ptr<Expression> position;
    std::shared_ptr<Expression> scale;
    std::shared_ptr<Expression> rotation;
    std::string surface_material;
    
    PlaceGeometryStatement() = default;
    
    void accept(StatementVisitor& visitor) const override;
};

/**
 * @brief Visitor interface for statement traversal
 */
class StatementVisitor {
public:
    virtual void visit(const AssignmentStatement& stmt) = 0;
    virtual void visit(const ForStatement& stmt) = 0;
    virtual void visit(const IfStatement& stmt) = 0;
    virtual void visit(const WhileStatement& stmt) = 0;
    virtual void visit(const RunStatement& stmt) = 0;
    virtual void visit(const ReduceStatement& stmt) = 0;
    virtual void visit(const PlaceGeometryStatement& stmt) = 0;
    virtual ~StatementVisitor() = default;
};

} // namespace ast
} // namespace parsing
} // namespace fluidloom
