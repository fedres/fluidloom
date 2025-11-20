#pragma once
// @stable - Simulation AST for DSL parser

#include "fluidloom/parsing/ast/StatementAST.h"
#include <string>
#include <vector>
#include <memory>

namespace fluidloom {
namespace parsing {
namespace ast {

/**
 * @brief Simulation definition AST
 * 
 * Represents a complete simulation with initial conditions, time loop, and output.
 */
class SimulationAST {
public:
    // Import statements
    std::vector<std::string> imports;
    
    // Code blocks
    std::vector<std::shared_ptr<Statement>> initial_condition;
    std::vector<std::shared_ptr<Statement>> time_loop;
    std::vector<std::shared_ptr<Statement>> final_output;
    
    // Source location
    Expression::SourceLoc loc;
    
    SimulationAST() = default;
    
    // Accessors
    const std::vector<std::string>& getImports() const { return imports; }
    const std::vector<std::shared_ptr<Statement>>& getInitialCondition() const { return initial_condition; }
    const std::vector<std::shared_ptr<Statement>>& getTimeLoop() const { return time_loop; }
    const std::vector<std::shared_ptr<Statement>>& getFinalOutput() const { return final_output; }
    
    // Mutators (for parser)
    void addImport(std::string import_path) { imports.push_back(std::move(import_path)); }
    void setInitialCondition(std::vector<std::shared_ptr<Statement>> stmts) { initial_condition = std::move(stmts); }
    void setTimeLoop(std::vector<std::shared_ptr<Statement>> stmts) { time_loop = std::move(stmts); }
    void setFinalOutput(std::vector<std::shared_ptr<Statement>> stmts) { final_output = std::move(stmts); }
};

} // namespace ast
} // namespace parsing
} // namespace fluidloom
