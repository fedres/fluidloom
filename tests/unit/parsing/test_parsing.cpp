#include <gtest/gtest.h>
#include "fluidloom/parsing/registry/LatticeRegistry.h"
#include "fluidloom/parsing/registry/ConstantRegistry.h"
#include "fluidloom/parsing/codegen/OpenCLPreambleGenerator.h"

using namespace fluidloom::parsing;

class ParsingTest : public ::testing::Test {
protected:
    void SetUp() override {
        LatticeRegistry::getInstance().clear();
        ConstantRegistry::getInstance().clear();
    }
};

TEST_F(ParsingTest, LatticeRegistryBasic) {
    auto& registry = LatticeRegistry::getInstance();
    
    LatticeDescriptor d3q19;
    d3q19.name = "D3Q19";
    d3q19.num_populations = 19;
    d3q19.cs2 = 1.0 / 3.0;
    
    // Add some stencil vectors and weights
    d3q19.stencil_vectors = {
        {0, 0, 0},
        {1, 0, 0}, {-1, 0, 0},
        {0, 1, 0}, {0, -1, 0},
        {0, 0, 1}, {0, 0, -1}
    };
    d3q19.weights = {1.0/3.0, 1.0/18.0, 1.0/18.0, 1.0/18.0, 1.0/18.0, 1.0/18.0, 1.0/18.0};
    
    EXPECT_NO_THROW(registry.add(d3q19));
    EXPECT_TRUE(registry.exists("D3Q19"));
    
    const auto* retrieved = registry.get("D3Q19");
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->name, "D3Q19");
    EXPECT_EQ(retrieved->num_populations, 19);
}

TEST_F(ParsingTest, ConstantRegistryBasic) {
    auto& registry = ConstantRegistry::getInstance();
    
    ConstantDescriptor omega;
    omega.name = "OMEGA";
    omega.type = ConstantDescriptor::Type::FLOAT;
    omega.value.f = 1.7f;
    
    EXPECT_NO_THROW(registry.add(omega));
    EXPECT_TRUE(registry.exists("OMEGA"));
    
    const auto* retrieved = registry.get("OMEGA");
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->name, "OMEGA");
    EXPECT_FLOAT_EQ(retrieved->value.f, 1.7f);
}

TEST_F(ParsingTest, OpenCLCodeGeneration) {
    auto& const_reg = ConstantRegistry::getInstance();
    auto& lattice_reg = LatticeRegistry::getInstance();
    
    // Add a constant
    ConstantDescriptor tau;
    tau.name = "TAU";
    tau.type = ConstantDescriptor::Type::FLOAT;
    tau.value.f = 0.6f;
    const_reg.add(tau);
    
    // Add a lattice
    LatticeDescriptor d2q9;
    d2q9.name = "D2Q9";
    d2q9.cs2 = 1.0 / 3.0;
    d2q9.stencil_vectors = {{0,0,0}, {1,0,0}, {-1,0,0}};
    d2q9.weights = {4.0/9.0, 1.0/9.0, 1.0/9.0};
    d2q9.generateOpenCLCode();
    lattice_reg.add(d2q9);
    
    // Generate preamble
    OpenCLPreambleGenerator generator;
    std::string preamble = generator.generate();
    
    EXPECT_FALSE(preamble.empty());
    EXPECT_NE(preamble.find("TAU"), std::string::npos);
    EXPECT_NE(preamble.find("D2Q9"), std::string::npos);
}
