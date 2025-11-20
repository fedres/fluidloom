#include <gtest/gtest.h>
#include "fluidloom/halo/events/EventChain.h"
#include "fluidloom/halo/communicators/HaloExchangeManager.h"
#include "fluidloom/core/backend/MockBackend.h"
#include "fluidloom/core/registry/FieldRegistry.h"
#include "fluidloom/core/fields/FieldDescriptor.h" // Assuming this is the path

using namespace fluidloom;
using namespace fluidloom::halo;
using namespace fluidloom::fields; // For FieldDescriptor

class ExchangeManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        backend = std::make_unique<MockBackend>();
        backend->initialize();
        
        // Use singleton registry
        auto& reg = registry::FieldRegistry::instance();
        
        // Register a dummy field (use unique name to avoid test pollution)
        fields::FieldDescriptor desc("density_test", fields::FieldType::FLOAT32, 1);
        if (!reg.exists("density_test")) {
            reg.registerField(desc);
        }
        
        manager = std::make_unique<HaloExchangeManager>(backend.get(), reg);
    }

    void TearDown() override {
        backend->shutdown();
    }

    std::unique_ptr<MockBackend> backend;
    // Registry is singleton, no unique_ptr
    std::unique_ptr<HaloExchangeManager> manager;
};

TEST(EventChainTest, BasicOperations) {
    EventChain chain;
    
    // Should be empty initially
    EXPECT_TRUE(chain.getUnpackWaitList().empty());
    
    // Add dummy event
    int dummy_event = 1;
    chain.addPackEvent(&dummy_event);
    
    EXPECT_FALSE(chain.getUnpackWaitList().empty());
    EXPECT_EQ(chain.getUnpackWaitList().size(), 1);
    
    // Reset
    chain.reset();
    EXPECT_TRUE(chain.getUnpackWaitList().empty());
}

TEST_F(ExchangeManagerTest, Initialization) {
    EXPECT_NO_THROW(manager->initialize(64)); // 64MB
    
    auto stats = manager->getStats();
    EXPECT_EQ(stats.num_exchanges, 0);
}

TEST_F(ExchangeManagerTest, GhostRangeManagement) {
    GhostRange range;
    range.hilbert_start = 0;
    range.hilbert_end = 100;
    range.target_gpu = 1;
    
    manager->addGhostRange(range);
    
    // No direct getter for ranges, but we can verify it doesn't crash
    // and potentially check side effects if we had access
}

TEST_F(ExchangeManagerTest, ExchangeCycle) {
    manager->initialize(10);
    
    GhostRange range;
    range.hilbert_start = 0;
    range.hilbert_end = 100;
    range.target_gpu = 1;
    range.pack_size_bytes = 1024;
    manager->addGhostRange(range);
    
    // Simulate a full cycle
    EXPECT_NO_THROW(manager->exchangeAsync());
    
    // Wait completion
    EXPECT_NO_THROW(manager->waitCompletion());
    
    auto stats = manager->getStats();
    EXPECT_EQ(stats.num_exchanges, 1);
    
    // Swap buffers
    EXPECT_NO_THROW(manager->swapBuffers());
}
