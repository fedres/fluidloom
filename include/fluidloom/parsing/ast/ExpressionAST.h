#pragma once
// @stable - Expression AST for DSL parser

#include <memory>
#include <vector>
#include <string>
#include <variant>

namespace fluidloom {
namespace parsing {
namespace ast {

// Forward declaration
class ExpressionVisitor;

/**
 * @brief Base class for all expression AST nodes
 */
class Expression {
public:
    virtual ~Expression() = default;
    virtual void accept(ExpressionVisitor& visitor) const = 0;
    
    // Source location for error reporting
    struct SourceLoc {
        size_t line = 0;
        size_t column = 0;
        size_t length = 0;
    } loc;
    
    // Type information (filled during semantic analysis)
    enum class Type {
        UNKNOWN,
        FLOAT,
        DOUBLE,
        INT,
        BOOL,
        VECTOR_3,
        VECTOR_19,  // For population arrays
        FIELD_REF,
        LAMBDA
    } type = Type::UNKNOWN;
};

/**
 * @brief Literal expression (42, 3.14, true)
 */
class LiteralExpression : public Expression {
public:
    std::variant<double, int64_t, bool> value;
    
    explicit LiteralExpression(double val) : value(val) { type = Type::FLOAT; }
    explicit LiteralExpression(int64_t val) : value(val) { type = Type::INT; }
    explicit LiteralExpression(bool val) : value(val) { type = Type::BOOL; }
    
    void accept(ExpressionVisitor& visitor) const override;
};

/**
 * @brief Variable/field reference (density, velocity.x)
 */
class VariableExpression : public Expression {
public:
    std::string name;
    std::string component;  // For velocity.x: component="x"
    
    explicit VariableExpression(std::string n) : name(std::move(n)) {}
    VariableExpression(std::string n, std::string comp) 
        : name(std::move(n)), component(std::move(comp)) {}
    
    void accept(ExpressionVisitor& visitor) const override;
};

/**
 * @brief Binary operation (a + b, x * y, etc.)
 */
class BinaryExpression : public Expression {
public:
    enum class Op {
        ADD, SUB, MUL, DIV, MOD,
        POW,  // Power operator **
        EQ, NE, LT, LE, GT, GE,
        AND, OR
    } op;
    
    std::shared_ptr<Expression> left;
    std::shared_ptr<Expression> right;
    
    BinaryExpression(Op operation, std::shared_ptr<Expression> l, std::shared_ptr<Expression> r)
        : op(operation), left(std::move(l)), right(std::move(r)) {}
    
    void accept(ExpressionVisitor& visitor) const override;
};

/**
 * @brief Unary operation (-x, !flag)
 */
class UnaryExpression : public Expression {
public:
    enum class Op { NEG, NOT } op;
    
    std::shared_ptr<Expression> operand;
    
    UnaryExpression(Op operation, std::shared_ptr<Expression> expr)
        : op(operation), operand(std::move(expr)) {}
    
    void accept(ExpressionVisitor& visitor) const override;
};

/**
 * @brief Function call (sqrt(x), equilibrium(i, rho, u))
 */
class CallExpression : public Expression {
public:
    std::string function_name;
    std::vector<std::shared_ptr<Expression>> arguments;
    
    CallExpression(std::string name, std::vector<std::shared_ptr<Expression>> args)
        : function_name(std::move(name)), arguments(std::move(args)) {}
    
    void accept(ExpressionVisitor& visitor) const override;
};

/**
 * @brief Array subscript (populations[i])
 */
class SubscriptExpression : public Expression {
public:
    std::shared_ptr<Expression> array;
    std::shared_ptr<Expression> index;
    
    SubscriptExpression(std::shared_ptr<Expression> arr, std::shared_ptr<Expression> idx)
        : array(std::move(arr)), index(std::move(idx)) {}
    
    void accept(ExpressionVisitor& visitor) const override;
};

/**
 * @brief Member access (velocity.x)
 */
class MemberExpression : public Expression {
public:
    std::shared_ptr<Expression> object;
    std::string member;
    
    MemberExpression(std::shared_ptr<Expression> obj, std::string mem)
        : object(std::move(obj)), member(std::move(mem)) {}
    
    void accept(ExpressionVisitor& visitor) const override;
};

/**
 * @brief Vector literal ((1.0, 2.0, 3.0))
 */
class VectorLiteralExpression : public Expression {
public:
    std::vector<std::shared_ptr<Expression>> elements;
    
    explicit VectorLiteralExpression(std::vector<std::shared_ptr<Expression>> elems)
        : elements(std::move(elems)) {
        type = elements.size() == 3 ? Type::VECTOR_3 : Type::VECTOR_19;
    }
    
    void accept(ExpressionVisitor& visitor) const override;
};

/**
 * @brief Lambda expression (lambda x, y: x**2 + y**2)
 */
class LambdaExpression : public Expression {
public:
    std::vector<std::string> parameters;
    std::shared_ptr<Expression> body;
    
    LambdaExpression(std::vector<std::string> params, std::shared_ptr<Expression> expr)
        : parameters(std::move(params)), body(std::move(expr)) { type = Type::LAMBDA; }
    
    void accept(ExpressionVisitor& visitor) const override;
};

/**
 * @brief Visitor interface for expression traversal
 */
class ExpressionVisitor {
public:
    virtual void visit(const LiteralExpression& expr) = 0;
    virtual void visit(const VariableExpression& expr) = 0;
    virtual void visit(const BinaryExpression& expr) = 0;
    virtual void visit(const UnaryExpression& expr) = 0;
    virtual void visit(const CallExpression& expr) = 0;
    virtual void visit(const SubscriptExpression& expr) = 0;
    virtual void visit(const MemberExpression& expr) = 0;
    virtual void visit(const VectorLiteralExpression& expr) = 0;
    virtual void visit(const LambdaExpression& expr) = 0;
    virtual ~ExpressionVisitor() = default;
};

} // namespace ast
} // namespace parsing
} // namespace fluidloom
