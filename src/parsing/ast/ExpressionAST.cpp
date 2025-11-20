#include "fluidloom/parsing/ast/ExpressionAST.h"

namespace fluidloom {
namespace parsing {
namespace ast {

void LiteralExpression::accept(ExpressionVisitor& visitor) const {
    visitor.visit(*this);
}

void VariableExpression::accept(ExpressionVisitor& visitor) const {
    visitor.visit(*this);
}

void BinaryExpression::accept(ExpressionVisitor& visitor) const {
    visitor.visit(*this);
}

void UnaryExpression::accept(ExpressionVisitor& visitor) const {
    visitor.visit(*this);
}

void CallExpression::accept(ExpressionVisitor& visitor) const {
    visitor.visit(*this);
}

void SubscriptExpression::accept(ExpressionVisitor& visitor) const {
    visitor.visit(*this);
}

void MemberExpression::accept(ExpressionVisitor& visitor) const {
    visitor.visit(*this);
}

void VectorLiteralExpression::accept(ExpressionVisitor& visitor) const {
    visitor.visit(*this);
}

void LambdaExpression::accept(ExpressionVisitor& visitor) const {
    visitor.visit(*this);
}

} // namespace ast
} // namespace parsing
} // namespace fluidloom
