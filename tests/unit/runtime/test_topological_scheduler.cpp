#include "fluidloom/runtime/dependency/DependencyGraph.h"
#include "fluidloom/runtime/scheduler/TopologicalScheduler.h"
#include "fluidloom/runtime/nodes/KernelNode.h"
#include <gtest/gtest.h>
#include <chrono>

using namespace fluidloom::runtime;

class TopologicalSchedulerTest : public ::testing::Test {
protected:
    std::vector<std::shared_ptr<nodes::ExecutionNode>> createSimpleDAG() {
        // Create 3-node DAG: A → B → C
        auto node_a = std::make_shared<nodes::KernelNode>("A");
        node_a->setId(0);
        
        auto node_b = std::make_shared<nodes::KernelNode>("B");
        node_b->setId(1);
        
        auto node_c = std::make_shared<nodes::KernelNode>("C");
        node_c->setId(2);
        
        node_a->addSuccessor(node_b);
        node_b->addPredecessor(node_a);
        
        node_b->addSuccessor(node_c);
        node_c->addPredecessor(node_b);
        
        return {node_a, node_b, node_c};
    }
    
    std::vector<std::shared_ptr<nodes::ExecutionNode>> createComplexDAG(size_t num_nodes) {
        std::vector<std::shared_ptr<nodes::ExecutionNode>> nodes;
        
        for (size_t i = 0; i < num_nodes; ++i) {
            auto node = std::make_shared<nodes::KernelNode>("Node" + std::to_string(i));
            node->setId(i);
            nodes.push_back(node);
        }
        
        // Create dependencies: each node depends on previous two
        for (size_t i = 2; i < num_nodes; ++i) {
            nodes[i-1]->addSuccessor(nodes[i]);
            nodes[i]->addPredecessor(nodes[i-1]);
            
            nodes[i-2]->addSuccessor(nodes[i]);
            nodes[i]->addPredecessor(nodes[i-2]);
        }
        
        return nodes;
    }
};

TEST_F(TopologicalSchedulerTest, SimpleDAG) {
    auto nodes = createSimpleDAG();
    auto graph = std::make_shared<dependency::DependencyGraph>(nodes);
    
    EXPECT_EQ(graph->getNodeCount(), 3);
    EXPECT_TRUE(graph->validate());
    
    const auto& order = graph->getTopologicalOrder();
    EXPECT_EQ(order.size(), 3);
    
    // A should come before B, B before C
    size_t a_pos = 0, b_pos = 0, c_pos = 0;
    for (size_t i = 0; i < order.size(); ++i) {
        if (graph->getNode(order[i])->getId() == 0) a_pos = i;
        if (graph->getNode(order[i])->getId() == 1) b_pos = i;
        if (graph->getNode(order[i])->getId() == 2) c_pos = i;
    }
    
    EXPECT_LT(a_pos, b_pos);
    EXPECT_LT(b_pos, c_pos);
}

TEST_F(TopologicalSchedulerTest, ComplexDAG) {
    const size_t num_nodes = 100;
    auto nodes = createComplexDAG(num_nodes);
    auto graph = std::make_shared<dependency::DependencyGraph>(nodes);
    
    EXPECT_EQ(graph->getNodeCount(), num_nodes);
    EXPECT_TRUE(graph->validate());
    
    const auto& order = graph->getTopologicalOrder();
    EXPECT_EQ(order.size(), num_nodes);
}

TEST_F(TopologicalSchedulerTest, Performance) {
    const size_t num_nodes = 100;
    auto nodes = createComplexDAG(num_nodes);
    
    auto start = std::chrono::high_resolution_clock::now();
    auto graph = std::make_shared<dependency::DependencyGraph>(nodes);
    auto end = std::chrono::high_resolution_clock::now();
    
    double build_time_ms = std::chrono::duration<double, std::milli>(end - start).count();
    
    // Should build and sort 100-node DAG in < 1ms
    EXPECT_LT(build_time_ms, 1.0);
}

TEST_F(TopologicalSchedulerTest, CycleDetection) {
    // Create cycle: A → B → C → A
    auto node_a = std::make_shared<nodes::KernelNode>("A");
    node_a->setId(0);
    
    auto node_b = std::make_shared<nodes::KernelNode>("B");
    node_b->setId(1);
    
    auto node_c = std::make_shared<nodes::KernelNode>("C");
    node_c->setId(2);
    
    node_a->addSuccessor(node_b);
    node_b->addPredecessor(node_a);
    
    node_b->addSuccessor(node_c);
    node_c->addPredecessor(node_b);
    
    node_c->addSuccessor(node_a);  // Creates cycle
    node_a->addPredecessor(node_c);
    
    std::vector<std::shared_ptr<nodes::ExecutionNode>> nodes = {node_a, node_b, node_c};
    
    // Should throw due to cycle
    EXPECT_THROW({dependency::DependencyGraph graph{nodes};}, std::runtime_error);
}

TEST_F(TopologicalSchedulerTest, Execute) {
    auto nodes = createSimpleDAG();
    auto graph = std::make_shared<dependency::DependencyGraph>(nodes);
    
    scheduler::TopologicalScheduler scheduler(graph);
    EXPECT_TRUE(scheduler.execute());
}
