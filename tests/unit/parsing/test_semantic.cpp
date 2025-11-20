#include "fluidloom/parsing/symbol_table/SymbolTable.h"
#include "fluidloom/parsing/semantic/SemanticAnalyzer.h"
#include "fluidloom/parsing/ast/KernelAST.h"
#include <gtest/gtest.h>

using namespace fluidloom::parsing;

class SymbolTableTest : public ::testing::Test {
protected:
    symbol_table::SymbolTable table;
};

TEST_F(SymbolTableTest, AddAndLookupSymbol) {
    symbol_table::Symbol var("x", symbol_table::SymbolType::FLOAT);
    ASSERT_TRUE(table.addSymbol(var));
    
    auto result = table.lookup("x");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->name, "x");
    EXPECT_EQ(result->type, symbol_table::SymbolType::FLOAT);
}

TEST_F(SymbolTableTest, DuplicateSymbolFails) {
    symbol_table::Symbol var1("x", symbol_table::SymbolType::FLOAT);
    symbol_table::Symbol var2("x", symbol_table::SymbolType::INT);
    
    ASSERT_TRUE(table.addSymbol(var1));
    ASSERT_FALSE(table.addSymbol(var2));  // Should fail - duplicate
}

TEST_F(SymbolTableTest, ScopeManagement) {
    table.addVariable("global", symbol_table::SymbolType::FLOAT);
    
    table.enterScope();
    table.addVariable("local", symbol_table::SymbolType::INT);
    
    // Both should be visible in inner scope
    EXPECT_TRUE(table.exists("global"));
    EXPECT_TRUE(table.exists("local"));
    
    table.exitScope();
    
    // Only global should be visible
    EXPECT_TRUE(table.exists("global"));
    EXPECT_FALSE(table.exists("local"));
}

TEST_F(SymbolTableTest, BuiltInFunctions) {
    // Built-in functions should be available
    EXPECT_TRUE(table.exists("sqrt"));
    EXPECT_TRUE(table.exists("pow"));
    EXPECT_TRUE(table.exists("abs"));
}

TEST_F(SymbolTableTest, AddField) {
    ASSERT_TRUE(table.addField("density", 1));
    
    auto result = table.lookup("density");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->type, symbol_table::SymbolType::FIELD);
    EXPECT_TRUE(result->is_field);
    EXPECT_EQ(result->num_components, 1);
}

class SemanticAnalyzerTest : public ::testing::Test {
protected:
    semantic::SemanticAnalyzer analyzer;
};

TEST_F(SemanticAnalyzerTest, AnalyzeSimpleKernel) {
    auto kernel = std::make_shared<ast::KernelAST>("TestKernel");
    kernel->setReadFields({"input"});
    kernel->setWriteFields({"output"});
    kernel->setHaloDepth(1);
    
    // Should succeed without field registry
    EXPECT_TRUE(analyzer.analyzeKernel(*kernel));
    EXPECT_TRUE(analyzer.getErrors().empty());
}

TEST_F(SemanticAnalyzerTest, ComputeHaloDepth) {
    auto kernel = std::make_shared<ast::KernelAST>("TestKernel");
    kernel->setHaloDepth(2);
    
    EXPECT_EQ(analyzer.computeHaloDepth(*kernel), 2);
}

TEST_F(SemanticAnalyzerTest, SymbolTablePopulation) {
    auto kernel = std::make_shared<ast::KernelAST>("TestKernel");
    kernel->setReadFields({"density", "velocity"});
    kernel->setWriteFields({"populations"});
    
    analyzer.analyzeKernel(*kernel);
    
    // Check that fields were added to symbol table
    auto& sym_table = analyzer.getSymbolTable();
    EXPECT_TRUE(sym_table.exists("density"));
    EXPECT_TRUE(sym_table.exists("velocity"));
    EXPECT_TRUE(sym_table.exists("populations"));
}
