#include "fluidloom/parsing/ast/StatementAST.h"

namespace fluidloom {
namespace parsing {
namespace ast {

void AssignmentStatement::accept(StatementVisitor& visitor) const {
    visitor.visit(*this);
}

void ForStatement::accept(StatementVisitor& visitor) const {
    visitor.visit(*this);
}

void IfStatement::accept(StatementVisitor& visitor) const {
    visitor.visit(*this);
}

void WhileStatement::accept(StatementVisitor& visitor) const {
    visitor.visit(*this);
}

void RunStatement::accept(StatementVisitor& visitor) const {
    visitor.visit(*this);
}

void ReduceStatement::accept(StatementVisitor& visitor) const {
    visitor.visit(*this);
}

void PlaceGeometryStatement::accept(StatementVisitor& visitor) const {
    visitor.visit(*this);
}

} // namespace ast
} // namespace parsing
} // namespace fluidloom
