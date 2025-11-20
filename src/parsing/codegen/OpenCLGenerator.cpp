#include "fluidloom/parsing/codegen/OpenCLGenerator.h"
#include <stdexcept>

namespace fluidloom {
namespace parsing {
namespace codegen {

std::string OpenCLGenerator::generateKernel(const ast::KernelAST& kernel) {
    code.str("");  // Clear
    code.clear();
    
    // Kernel signature
    code << "__kernel void " << kernel.getName() << "(\n";
    
    // Parameters (field buffers)
    bool first = true;
    for (const auto& field : kernel.getReadFields()) {
        if (!first) code << ",\n";
        code << "    __global float* " << field;
        first = false;
    }
    for (const auto& field : kernel.getWriteFields()) {
        // Skip if already in read list
        if (std::find(kernel.getReadFields().begin(), kernel.getReadFields().end(), field) 
            != kernel.getReadFields().end()) {
            continue;
        }
        if (!first) code << ",\n";
        code << "    __global float* " << field;
        first = false;
    }
    code << "\n) {\n";
    
    indent_level = 1;
    
    // Global ID
    writeIndent();
    code << "size_t gid = get_global_id(0);\n";
    
    // Execution mask
    if (!kernel.getExecutionMask().empty()) {
        writeIndent();
        code << "if (!(" << kernel.getExecutionMask() << ")) return;\n";
    }
    
    // Generate body
    for (const auto& stmt : kernel.getStatements()) {
        stmt->accept(*this);
    }
    
    indent_level = 0;
    code << "}\n";
    
    return code.str();
}

std::string OpenCLGenerator::generateExpression(const ast::Expression& expr) {
    code.str("");
    code.clear();
    expr.accept(*this);
    return code.str();
}

std::string OpenCLGenerator::generateStatement(const ast::Statement& stmt) {
    code.str("");
    code.clear();
    stmt.accept(*this);
    return code.str();
}

void OpenCLGenerator::visit(const ast::LiteralExpression& expr) {
    if (std::holds_alternative<double>(expr.value)) {
        code << std::get<double>(expr.value) << "f";
    } else if (std::holds_alternative<int64_t>(expr.value)) {
        code << std::get<int64_t>(expr.value);
    } else if (std::holds_alternative<bool>(expr.value)) {
        code << (std::get<bool>(expr.value) ? "true" : "false");
    }
}

void OpenCLGenerator::visit(const ast::VariableExpression& expr) {
    code << expr.name;
    if (!expr.component.empty()) {
        code << "." << expr.component;
    }
}

void OpenCLGenerator::visit(const ast::BinaryExpression& expr) {
    code << "(";
    expr.left->accept(*this);
    code << " " << getOperatorString(expr.op) << " ";
    expr.right->accept(*this);
    code << ")";
}

void OpenCLGenerator::visit(const ast::UnaryExpression& expr) {
    code << getUnaryOperatorString(expr.op);
    expr.operand->accept(*this);
}

void OpenCLGenerator::visit(const ast::CallExpression& expr) {
    code << expr.function_name << "(";
    for (size_t i = 0; i < expr.arguments.size(); ++i) {
        if (i > 0) code << ", ";
        expr.arguments[i]->accept(*this);
    }
    code << ")";
}

void OpenCLGenerator::visit(const ast::SubscriptExpression& expr) {
    expr.array->accept(*this);
    code << "[";
    expr.index->accept(*this);
    code << "]";
}

void OpenCLGenerator::visit(const ast::MemberExpression& expr) {
    expr.object->accept(*this);
    code << "." << expr.member;
}

void OpenCLGenerator::visit(const ast::VectorLiteralExpression& expr) {
    code << "(float" << expr.elements.size() << ")(";
    for (size_t i = 0; i < expr.elements.size(); ++i) {
        if (i > 0) code << ", ";
        expr.elements[i]->accept(*this);
    }
    code << ")";
}

void OpenCLGenerator::visit(const ast::LambdaExpression& expr) {
    // Lambda expressions need to be converted to inline functions
    // For now, just generate the body
    code << "/* lambda: */ ";
    expr.body->accept(*this);
}

void OpenCLGenerator::visit(const ast::AssignmentStatement& stmt) {
    writeIndent();
    stmt.target->accept(*this);
    code << " = ";
    stmt.value->accept(*this);
    code << ";\\n";
}

void OpenCLGenerator::visit(const ast::ForStatement& stmt) {
    writeIndent();
    code << "for (int " << stmt.loop_variable << " = ";
    stmt.start->accept(*this);
    code << "; " << stmt.loop_variable << " < ";
    stmt.end->accept(*this);
    code << "; " << stmt.loop_variable << "++) {\\n";
    
    indent_level++;
    for (const auto& s : stmt.body) {
        s->accept(*this);
    }
    indent_level--;
    
    writeIndent();
    code << "}\\n";
}

void OpenCLGenerator::visit(const ast::IfStatement& stmt) {
    writeIndent();
    code << "if (";
    stmt.condition->accept(*this);
    code << ") {\\n";
    
    indent_level++;
    for (const auto& s : stmt.then_branch) {
        s->accept(*this);
    }
    indent_level--;
    
    if (!stmt.else_branch.empty()) {
        writeIndent();
        code << "} else {\\n";
        indent_level++;
        for (const auto& s : stmt.else_branch) {
            s->accept(*this);
        }
        indent_level--;
    }
    
    writeIndent();
    code << "}\\n";
}

void OpenCLGenerator::visit(const ast::WhileStatement& stmt) {
    writeIndent();
    code << "while (";
    stmt.condition->accept(*this);
    code << ") {\\n";
    
    indent_level++;
    for (const auto& s : stmt.body) {
        s->accept(*this);
    }
    indent_level--;
    
    writeIndent();
    code << "}\\n";
}

void OpenCLGenerator::visit(const ast::RunStatement& stmt) {
    writeIndent();
    code << "// run(" << stmt.kernel_name << ") - requires runtime integration\\n";
}

void OpenCLGenerator::visit(const ast::ReduceStatement& stmt) {
    writeIndent();
    code << "// reduce_";
    switch (stmt.op) {
        case ast::ReduceStatement::Op::SUM: code << "sum"; break;
        case ast::ReduceStatement::Op::MIN: code << "min"; break;
        case ast::ReduceStatement::Op::MAX: code << "max"; break;
    }
    code << "(";
    stmt.expression->accept(*this);
    code << ") - requires tree reduction\\n";
}

void OpenCLGenerator::visit(const ast::PlaceGeometryStatement& stmt) {
    (void)stmt;  // Unused for now
    writeIndent();
    code << "// place_geometry - requires geometry integration\\n";
}

std::string OpenCLGenerator::getOperatorString(ast::BinaryExpression::Op op) {
    switch (op) {
        case ast::BinaryExpression::Op::ADD: return "+";
        case ast::BinaryExpression::Op::SUB: return "-";
        case ast::BinaryExpression::Op::MUL: return "*";
        case ast::BinaryExpression::Op::DIV: return "/";
        case ast::BinaryExpression::Op::MOD: return "%";
        case ast::BinaryExpression::Op::POW: return "pow";  // Will need special handling
        case ast::BinaryExpression::Op::EQ: return "==";
        case ast::BinaryExpression::Op::NE: return "!=";
        case ast::BinaryExpression::Op::LT: return "<";
        case ast::BinaryExpression::Op::LE: return "<=";
        case ast::BinaryExpression::Op::GT: return ">";
        case ast::BinaryExpression::Op::GE: return ">=";
        case ast::BinaryExpression::Op::AND: return "&&";
        case ast::BinaryExpression::Op::OR: return "||";
        default: return "?";
    }
}

std::string OpenCLGenerator::getUnaryOperatorString(ast::UnaryExpression::Op op) {
    switch (op) {
        case ast::UnaryExpression::Op::NEG: return "-";
        case ast::UnaryExpression::Op::NOT: return "!";
        default: return "?";
    }
}

} // namespace codegen
} // namespace parsing
} // namespace fluidloom
