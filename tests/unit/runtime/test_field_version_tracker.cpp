#include "fluidloom/runtime/dependency/FieldVersionTracker.h"
#include <gtest/gtest.h>
#include <thread>
#include <vector>

using namespace fluidloom::runtime::dependency;

class FieldVersionTrackerTest : public ::testing::Test {
protected:
    FieldVersionTracker tracker;
    
    void SetUp() override {
        tracker.registerField("density");
        tracker.registerField("velocity");
        tracker.registerField("pressure");
    }
};

TEST_F(FieldVersionTrackerTest, RegisterField) {
    // Initial version should be 0
    EXPECT_EQ(tracker.getVersion("density"), 0);
    EXPECT_EQ(tracker.getVersion("velocity"), 0);
    EXPECT_EQ(tracker.getVersion("pressure"), 0);
}

TEST_F(FieldVersionTrackerTest, IncrementVersion) {
    uint64_t v1 = tracker.incrementVersion("density", 1);
    EXPECT_EQ(v1, 1);
    EXPECT_EQ(tracker.getVersion("density"), 1);
    
    uint64_t v2 = tracker.incrementVersion("density", 2);
    EXPECT_EQ(v2, 2);
    EXPECT_EQ(tracker.getVersion("density"), 2);
}

TEST_F(FieldVersionTrackerTest, LastWriter) {
    tracker.incrementVersion("density", 42);
    EXPECT_EQ(tracker.getLastWriter("density"), 42);
    
    tracker.incrementVersion("density", 99);
    EXPECT_EQ(tracker.getLastWriter("density"), 99);
}

TEST_F(FieldVersionTrackerTest, Epoch) {
    EXPECT_EQ(tracker.getCurrentEpoch(), 0);
    
    uint64_t e1 = tracker.incrementEpoch();
    EXPECT_EQ(e1, 1);
    EXPECT_EQ(tracker.getCurrentEpoch(), 1);
    
    uint64_t e2 = tracker.incrementEpoch();
    EXPECT_EQ(e2, 2);
}

TEST_F(FieldVersionTrackerTest, ResetAll) {
    tracker.incrementVersion("density", 1);
    tracker.incrementVersion("velocity", 2);
    tracker.incrementEpoch();
    
    tracker.resetAll();
    
    EXPECT_EQ(tracker.getVersion("density"), 0);
    EXPECT_EQ(tracker.getVersion("velocity"), 0);
    EXPECT_EQ(tracker.getCurrentEpoch(), 0);
    EXPECT_EQ(tracker.getLastWriter("density"), -1);
}

TEST_F(FieldVersionTrackerTest, ThreadSafety) {
    const int num_threads = 10;
    const int increments_per_thread = 100;
    
    std::vector<std::thread> threads;
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([this, i]() {
            for (int j = 0; j < increments_per_thread; ++j) {
                tracker.incrementVersion("density", i);
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // Should have incremented exactly num_threads * increments_per_thread times
    EXPECT_EQ(tracker.getVersion("density"), num_threads * increments_per_thread);
}

TEST_F(FieldVersionTrackerTest, UnregisteredField) {
    EXPECT_THROW(tracker.getVersion("unknown"), std::invalid_argument);
    EXPECT_THROW(tracker.incrementVersion("unknown", 1), std::invalid_argument);
}
