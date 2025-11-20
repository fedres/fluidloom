#include <gtest/gtest.h>
#include "fluidloom/core/backend/BackendFactory.h"
#include "fluidloom/core/backend/MockBackend.h"
#include "fluidloom/common/Logger.h"
#include <cstdlib>

using namespace fluidloom;

class BackendFactoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        Logger::instance().setLevel(LogLevel::ERROR);
        // Clear any existing backend
        BackendFactory::setBackend(nullptr);
    }
    
    void TearDown() override {
        BackendFactory::setBackend(nullptr);
    }
};

TEST_F(BackendFactoryTest, CreateMockBackend) {
    auto backend = BackendFactory::createBackend(BackendChoice::MOCK);
    EXPECT_NE(backend, nullptr);
    EXPECT_EQ(backend->getType(), BackendType::MOCK);
}

TEST_F(BackendFactoryTest, CreateOpenCLBackend) {
    try {
        auto backend = BackendFactory::createBackend(BackendChoice::OPENCL);
        EXPECT_NE(backend, nullptr);
        EXPECT_EQ(backend->getType(), BackendType::OPENCL);
    } catch (const std::exception& e) {
        GTEST_SKIP() << "OpenCL not available: " << e.what();
    }
}

TEST_F(BackendFactoryTest, AutoDetectOpenCL) {
    auto backend = BackendFactory::createBackend(BackendChoice::AUTO);
    EXPECT_NE(backend, nullptr);
    // Type depends on system, but should succeed
}

TEST_F(BackendFactoryTest, EnvironmentVariableMock) {
    setenv("FL_BACKEND", "MOCK", 1);
    auto backend = BackendFactory::createBackend(BackendChoice::AUTO);
    EXPECT_EQ(backend->getType(), BackendType::MOCK);
    unsetenv("FL_BACKEND");
}

TEST_F(BackendFactoryTest, EnvironmentVariableOpenCL) {
    setenv("FL_BACKEND", "OPENCL", 1);
    try {
        auto backend = BackendFactory::createBackend(BackendChoice::AUTO);
        EXPECT_EQ(backend->getType(), BackendType::OPENCL);
    } catch (const std::exception& e) {
        GTEST_SKIP() << "OpenCL not available: " << e.what();
    }
    unsetenv("FL_BACKEND");
}

TEST_F(BackendFactoryTest, SingletonInstance) {
    // First call should create backend
    auto& instance1 = BackendFactory::getInstance();
    if (!instance1.isInitialized()) {
        instance1.initialize();
    }
    EXPECT_TRUE(instance1.isInitialized());
    
    // Second call should return same instance
    auto& instance2 = BackendFactory::getInstance();
    EXPECT_EQ(&instance1, &instance2);
}

TEST_F(BackendFactoryTest, SetBackendExplicitly) {
    auto mock = std::make_unique<MockBackend>();
    BackendFactory::setBackend(std::move(mock));
    
    auto& instance = BackendFactory::getInstance();
    EXPECT_EQ(instance.getType(), BackendType::MOCK);
}
