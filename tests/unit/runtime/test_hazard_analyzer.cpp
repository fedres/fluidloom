#include "fluidloom/runtime/dependency/HazardAnalyzer.h"
#include "fluidloom/runtime/nodes/KernelNode.h"
#include <gtest/gtest.h>

using namespace fluidloom::runtime;

class HazardAnalyzerTest : public ::testing::Test {
protected:
    std::shared_ptr<dependency::FieldVersionTracker> tracker;
    std::unique_ptr<dependency::HazardAnalyzer> analyzer;
    
    void SetUp() override {
        tracker = std::make_shared<dependency::FieldVersionTracker>();
        tracker->registerField("density");
        tracker->registerField("velocity");
        tracker->registerField("pressure");
        
        analyzer = std::make_unique<dependency::HazardAnalyzer>(tracker);
    }
};

TEST_F(HazardAnalyzerTest, DetectRAW) {
    // Node A writes density, Node B reads density
    auto node_a = std::make_shared<nodes::KernelNode>("NodeA");
    node_a->setId(0);
    node_a->setWriteFields({"density"});
    
    auto node_b = std::make_shared<nodes::KernelNode>("NodeB");
    node_b->setId(1);
    node_b->setReadFields({"density"});
    
    std::vector<std::shared_ptr<nodes::ExecutionNode>> nodes = {node_a, node_b};
    auto hazards = analyzer->analyzeNodes(nodes);
    
    // Should detect RAW hazard
    bool found_raw = false;
    for (const auto& h : hazards) {
        if (h.type == dependency::HazardAnalyzer::HazardType::RAW &&
            h.from_node_idx == 0 && h.to_node_idx == 1) {
            found_raw = true;
        }
    }
    EXPECT_TRUE(found_raw);
}

TEST_F(HazardAnalyzerTest, DetectWAW) {
    // Both nodes write to density
    auto node_a = std::make_shared<nodes::KernelNode>("NodeA");
    node_a->setId(0);
    node_a->setWriteFields({"density"});
    
    auto node_b = std::make_shared<nodes::KernelNode>("NodeB");
    node_b->setId(1);
    node_b->setWriteFields({"density"});
    
    std::vector<std::shared_ptr<nodes::ExecutionNode>> nodes = {node_a, node_b};
    auto hazards = analyzer->analyzeNodes(nodes);
    
    // Should detect WAW hazard
    bool found_waw = false;
    for (const auto& h : hazards) {
        if (h.type == dependency::HazardAnalyzer::HazardType::WAW &&
            h.from_node_idx == 0 && h.to_node_idx == 1) {
            found_waw = true;
        }
    }
    EXPECT_TRUE(found_waw);
}

TEST_F(HazardAnalyzerTest, DetectWAR) {
    // Node A reads density, Node B writes density
    auto node_a = std::make_shared<nodes::KernelNode>("NodeA");
    node_a->setId(0);
    node_a->setReadFields({"density"});
    
    auto node_b = std::make_shared<nodes::KernelNode>("NodeB");
    node_b->setId(1);
    node_b->setWriteFields({"density"});
    
    std::vector<std::shared_ptr<nodes::ExecutionNode>> nodes = {node_a, node_b};
    auto hazards = analyzer->analyzeNodes(nodes);
    
    // Should detect WAR hazard
    bool found_war = false;
    for (const auto& h : hazards) {
        if (h.type == dependency::HazardAnalyzer::HazardType::WAR &&
            h.from_node_idx == 0 && h.to_node_idx == 1) {
            found_war = true;
        }
    }
    EXPECT_TRUE(found_war);
}

TEST_F(HazardAnalyzerTest, DetectLevelBarrier) {
    // Nodes at different levels
    auto node_a = std::make_shared<nodes::KernelNode>("NodeA");
    node_a->setId(0);
    node_a->setLevel(0);
    
    auto node_b = std::make_shared<nodes::KernelNode>("NodeB");
    node_b->setId(1);
    node_b->setLevel(1);
    
    std::vector<std::shared_ptr<nodes::ExecutionNode>> nodes = {node_a, node_b};
    auto hazards = analyzer->analyzeNodes(nodes);
    
    // Should detect level barrier
    bool found_barrier = false;
    for (const auto& h : hazards) {
        if (h.type == dependency::HazardAnalyzer::HazardType::LEVEL_BARRIER) {
            found_barrier = true;
        }
    }
    EXPECT_TRUE(found_barrier);
}

TEST_F(HazardAnalyzerTest, NoFalsePositives) {
    // Nodes with no dependencies
    auto node_a = std::make_shared<nodes::KernelNode>("NodeA");
    node_a->setId(0);
    node_a->setReadFields({"density"});
    node_a->setWriteFields({"velocity"});
    
    auto node_b = std::make_shared<nodes::KernelNode>("NodeB");
    node_b->setId(1);
    node_b->setReadFields({"pressure"});
    node_b->setWriteFields({"density"});
    
    std::vector<std::shared_ptr<nodes::ExecutionNode>> nodes = {node_a, node_b};
    auto hazards = analyzer->analyzeNodes(nodes);
    
    // Should only detect WAR (A reads density, B writes density)
    EXPECT_EQ(hazards.size(), 1);
    EXPECT_EQ(hazards[0].type, dependency::HazardAnalyzer::HazardType::WAR);
}
