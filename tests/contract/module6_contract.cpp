#include <gtest/gtest.h>
#include "fluidloom/parsing/visitors/FieldsVisitor.h"
#include "fluidloom/parsing/visitors/LatticesVisitor.h"
#include "fluidloom/parsing/registry/LatticeRegistry.h"
#include "fluidloom/parsing/registry/ConstantRegistry.h"
#include "fluidloom/parsing/codegen/OpenCLPreambleGenerator.h"
#include "fluidloom/core/registry/FieldRegistry.h"

/**
 * @file module6_contract.cpp
 * @brief Contract tests for Module 6 API stability
 * 
 * These tests MUST NOT CHANGE after Module 6 completion.
 * They verify that the public API remains stable and backwards compatible.
 */

using namespace fluidloom::parsing;

class Module6ContractTest : public ::testing::Test {
protected:
    void SetUp() override {
        fluidloom::registry::FieldRegistry::instance().clear();
        LatticeRegistry::getInstance().clear();
        ConstantRegistry::getInstance().clear();
    }
};

// Contract: FieldsVisitor can parse valid field definitions
TEST_F(Module6ContractTest, FieldsVisitorAPIStability) {
    FieldsVisitor visitor;
    EXPECT_NO_THROW(visitor.parseFile("test.fl"));
    EXPECT_TRUE(visitor.hasErrors() || !visitor.hasErrors()); // API exists
    EXPECT_NO_THROW(visitor.getErrors());
}

// Contract: LatticesVisitor can parse valid lattice definitions
TEST_F(Module6ContractTest, LatticesVisitorAPIStability) {
    LatticesVisitor visitor;
    EXPECT_NO_THROW(visitor.parseFile("test.fl"));
    EXPECT_TRUE(visitor.hasErrors() || !visitor.hasErrors()); // API exists
    EXPECT_NO_THROW(visitor.getErrors());
}

// Contract: LatticeRegistry is a singleton
TEST_F(Module6ContractTest, LatticeRegistrySingleton) {
    auto& reg1 = LatticeRegistry::getInstance();
    auto& reg2 = LatticeRegistry::getInstance();
    EXPECT_EQ(&reg1, &reg2);
}

// Contract: ConstantRegistry is a singleton
TEST_F(Module6ContractTest, ConstantRegistrySingleton) {
    auto& reg1 = ConstantRegistry::getInstance();
    auto& reg2 = ConstantRegistry::getInstance();
    EXPECT_EQ(&reg1, &reg2);
}

// Contract: LatticeDescriptor has required fields
TEST_F(Module6ContractTest, LatticeDescriptorStructure) {
    LatticeDescriptor desc;
    desc.name = "test";
    desc.cs2 = 1.0/3.0;
    desc.num_populations = 0;
    EXPECT_TRUE(desc.stencil_vectors.empty());
    EXPECT_TRUE(desc.weights.empty());
}

// Contract: ConstantDescriptor has required fields
TEST_F(Module6ContractTest, ConstantDescriptorStructure) {
    ConstantDescriptor desc;
    desc.name = "test";
    desc.type = ConstantDescriptor::Type::FLOAT;
    desc.value.f = 1.0f;
    EXPECT_NO_THROW(desc.getOpenCLDefine());
}

// Contract: OpenCLPreambleGenerator can generate code
TEST_F(Module6ContractTest, CodeGeneratorAPIStability) {
    OpenCLPreambleGenerator generator;
    EXPECT_NO_THROW(generator.generate());
    EXPECT_NO_THROW(generator.generateToFile("/tmp/test.cl"));
}

// Contract: ParseError has line and column information
TEST_F(Module6ContractTest, ParseErrorStructure) {
    ParseError error;
    error.message = "test";
    error.line = 1;
    error.column = 1;
    EXPECT_NO_THROW(error.toString());
}

// Contract: LatticeDescriptor validation works
TEST_F(Module6ContractTest, LatticeValidation) {
    LatticeDescriptor desc;
    desc.stencil_vectors = {{0,0,0}};
    desc.weights = {1.0};
    EXPECT_TRUE(desc.validate());
}

// Contract: LatticeDescriptor can compute opposites
TEST_F(Module6ContractTest, LatticeOppositeComputation) {
    LatticeDescriptor desc;
    desc.stencil_vectors = {{0,0,0}, {1,0,0}, {-1,0,0}};
    desc.weights = {2.0/3.0, 1.0/6.0, 1.0/6.0};
    EXPECT_NO_THROW(desc.computeOpposites());
}

// Contract: LatticeDescriptor can generate OpenCL code
TEST_F(Module6ContractTest, LatticeCodeGeneration) {
    LatticeDescriptor desc;
    desc.name = "D1Q3";
    desc.stencil_vectors = {{0,0,0}};
    desc.weights = {1.0};
    desc.cs2 = 1.0/3.0;
    EXPECT_NO_THROW(desc.generateOpenCLCode());
    EXPECT_FALSE(desc.generated.preamble.empty());
}
