#include <gtest/gtest.h>
#include "fluidloom/core/fields/FieldDescriptor.h"
#include "fluidloom/core/registry/FieldRegistry.h"
#include "fluidloom/core/fields/SOAFieldManager.h"
#include "fluidloom/core/backend/BackendFactory.h"

using namespace fluidloom;
using namespace fluidloom::fields;
using namespace fluidloom::registry;

// ========== FieldDescriptor Tests ==========

TEST(FieldDescriptorTest, BasicConstruction) {
    FieldDescriptor desc("density", FieldType::FLOAT32, 1);
    
    EXPECT_EQ(desc.name, "density");
    EXPECT_EQ(desc.type, FieldType::FLOAT32);
    EXPECT_EQ(desc.num_components, 1);
    EXPECT_EQ(desc.halo_depth, 0);
    EXPECT_TRUE(desc.isValid());
}

TEST(FieldDescriptorTest, WithHaloDepth) {
    FieldDescriptor desc("velocity", FieldType::FLOAT32, 3, 1);
    
    EXPECT_EQ(desc.name, "velocity");
    EXPECT_EQ(desc.num_components, 3);
    EXPECT_EQ(desc.halo_depth, 1);
    EXPECT_TRUE(desc.isValid());
}

TEST(FieldDescriptorTest, InvalidNameEmpty) {
    EXPECT_THROW({
        FieldDescriptor desc("", FieldType::FLOAT32, 1);
    }, std::invalid_argument);
}

TEST(FieldDescriptorTest, InvalidNameTooLong) {
    std::string long_name(65, 'a');  // 65 characters
    EXPECT_THROW({
        FieldDescriptor desc(long_name, FieldType::FLOAT32, 1);
    }, std::invalid_argument);
}

TEST(FieldDescriptorTest, InvalidComponentsZero) {
    EXPECT_THROW({
        FieldDescriptor desc("test", FieldType::FLOAT32, 0);
    }, std::invalid_argument);
}

TEST(FieldDescriptorTest, InvalidComponentsTooMany) {
    EXPECT_THROW({
        FieldDescriptor desc("test", FieldType::FLOAT32, 33);
    }, std::invalid_argument);
}

TEST(FieldDescriptorTest, InvalidHaloDepth) {
    EXPECT_THROW({
        FieldDescriptor desc("test", FieldType::FLOAT32, 1, 3);  // Halo depth > 2
    }, std::invalid_argument);
}

TEST(FieldDescriptorTest, BytesPerCell) {
    FieldDescriptor desc1("f32", FieldType::FLOAT32, 1);
    EXPECT_EQ(desc1.bytesPerCell(), 4);  // 4 bytes
    
    FieldDescriptor desc2("f64", FieldType::FLOAT64, 1);
    EXPECT_EQ(desc2.bytesPerCell(), 8);  // 8 bytes
    
    FieldDescriptor desc3("vec3", FieldType::FLOAT32, 3);
    EXPECT_EQ(desc3.bytesPerCell(), 12);  // 4 * 3 = 12 bytes
    
    FieldDescriptor desc4("mask", FieldType::UINT8, 1);
    EXPECT_EQ(desc4.bytesPerCell(), 1);  // 1 byte
}

TEST(FieldDescriptorTest, UniqueIDs) {
    FieldDescriptor desc1("density", FieldType::FLOAT32, 1);
    FieldDescriptor desc2("pressure", FieldType::FLOAT32, 1);
    FieldDescriptor desc3("density", FieldType::FLOAT64, 1);  // Same name, different type
    
    // Different names should have different IDs
    EXPECT_NE(desc1.id, desc2.id);
    
    // Same name should have same ID (ID is based on name)
    EXPECT_EQ(desc1.id, desc3.id);
}

TEST(FieldDescriptorTest, DefaultValues) {
    FieldDescriptor desc("test", FieldType::FLOAT32, 3);
    
    // Default values should be initialized
    EXPECT_EQ(desc.default_value.size(), 3);
    EXPECT_DOUBLE_EQ(desc.default_value[0], 0.0);
    EXPECT_DOUBLE_EQ(desc.default_value[1], 0.0);
    EXPECT_DOUBLE_EQ(desc.default_value[2], 0.0);
}

// ========== FieldRegistry Tests ==========

TEST(FieldRegistryTest, SingletonInstance) {
    auto& registry1 = FieldRegistry::instance();
    auto& registry2 = FieldRegistry::instance();
    
    // Should return the same instance
    EXPECT_EQ(&registry1, &registry2);
}

TEST(FieldRegistryTest, RegisterAndLookupByName) {
    auto& registry = FieldRegistry::instance();
    
    FieldDescriptor desc("test_field_1", FieldType::FLOAT32, 1);
    bool registered = registry.registerField(desc);
    
    EXPECT_TRUE(registered);
    
    auto result = registry.lookupByName("test_field_1");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->name, "test_field_1");
    EXPECT_EQ(result->type, FieldType::FLOAT32);
}

TEST(FieldRegistryTest, RegisterDuplicateName) {
    auto& registry = FieldRegistry::instance();
    
    FieldDescriptor desc1("duplicate_test", FieldType::FLOAT32, 1);
    FieldDescriptor desc2("duplicate_test", FieldType::FLOAT64, 1);
    
    bool first = registry.registerField(desc1);
    bool second = registry.registerField(desc2);
    
    EXPECT_TRUE(first);
    EXPECT_FALSE(second);  // Should fail on duplicate
}

TEST(FieldRegistryTest, LookupByID) {
    auto& registry = FieldRegistry::instance();
    
    FieldDescriptor desc("lookup_id_test", FieldType::FLOAT32, 1);
    registry.registerField(desc);
    
    FieldHandle handle(desc.id);
    auto result = registry.lookupById(handle);
    
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->name, "lookup_id_test");
}

TEST(FieldRegistryTest, Exists) {
    auto& registry = FieldRegistry::instance();
    
    FieldDescriptor desc("exists_test", FieldType::FLOAT32, 1);
    registry.registerField(desc);
    
    EXPECT_TRUE(registry.exists("exists_test"));
    EXPECT_FALSE(registry.exists("nonexistent_field"));
}

TEST(FieldRegistryTest, GetAllNames) {
    auto& registry = FieldRegistry::instance();
    
    size_t initial_count = registry.getAllNames().size();
    
    FieldDescriptor desc1("names_test_1", FieldType::FLOAT32, 1);
    FieldDescriptor desc2("names_test_2", FieldType::FLOAT32, 1);
    
    registry.registerField(desc1);
    registry.registerField(desc2);
    
    auto names = registry.getAllNames();
    EXPECT_GE(names.size(), initial_count + 2);
}

// ========== SOAFieldManager Tests ==========

class SOAFieldManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        backend_ptr = BackendFactory::createBackend(BackendChoice::MOCK);
        backend = backend_ptr.get();
        backend->initialize();
        manager = std::make_unique<SOAFieldManager>(backend);
    }

    void TearDown() override {
        manager.reset();
        backend->shutdown();
    }

    std::unique_ptr<IBackend> backend_ptr;
    IBackend* backend;
    std::unique_ptr<SOAFieldManager> manager;
};

TEST_F(SOAFieldManagerTest, AllocateScalarField) {
    FieldDescriptor desc("density", FieldType::FLOAT32, 1);
    
    FieldHandle handle = manager->allocate(desc, 1000);
    
    EXPECT_NE(handle.id, 0);
    EXPECT_EQ(manager->getAllocationSize(handle), 1000);
}

TEST_F(SOAFieldManagerTest, AllocateVectorField) {
    FieldDescriptor desc("velocity", FieldType::FLOAT32, 3);
    
    FieldHandle handle = manager->allocate(desc, 1000);
    
    EXPECT_EQ(manager->getAllocationSize(handle), 1000);
    
    // Should be able to get device pointers for each component
    void* ptr0 = manager->getDevicePtr(handle, 0);
    void* ptr1 = manager->getDevicePtr(handle, 1);
    void* ptr2 = manager->getDevicePtr(handle, 2);
    
    EXPECT_NE(ptr0, nullptr);
    EXPECT_NE(ptr1, nullptr);
    EXPECT_NE(ptr2, nullptr);
}

TEST_F(SOAFieldManagerTest, ResizeField) {
    FieldDescriptor desc("test", FieldType::FLOAT32, 1);
    
    FieldHandle handle = manager->allocate(desc, 1000);
    EXPECT_EQ(manager->getAllocationSize(handle), 1000);
    
    manager->resize(handle, 2000);
    EXPECT_EQ(manager->getAllocationSize(handle), 2000);
}

TEST_F(SOAFieldManagerTest, VersionTracking) {
    FieldDescriptor desc("test", FieldType::FLOAT32, 1);
    
    FieldHandle handle = manager->allocate(desc, 1000);
    uint64_t version1 = manager->getVersion(handle);
    
    manager->markDirty(handle);
    uint64_t version2 = manager->getVersion(handle);
    
    EXPECT_GT(version2, version1);  // Version should increment
}

TEST_F(SOAFieldManagerTest, DirtyFlag) {
    FieldDescriptor desc("test", FieldType::FLOAT32, 1);
    
    FieldHandle handle = manager->allocate(desc, 1000);
    
    manager->markDirty(handle);
    EXPECT_TRUE(manager->isDirty(handle));
    
    manager->markClean(handle);
    EXPECT_FALSE(manager->isDirty(handle));
}

TEST_F(SOAFieldManagerTest, Deallocate) {
    FieldDescriptor desc("test", FieldType::FLOAT32, 1);
    
    FieldHandle handle = manager->allocate(desc, 1000);
    size_t memory_before = manager->getTotalMemoryUsage();
    
    manager->deallocate(handle);
    size_t memory_after = manager->getTotalMemoryUsage();
    
    EXPECT_LT(memory_after, memory_before);
}

TEST_F(SOAFieldManagerTest, MemoryUsage) {
    FieldDescriptor desc("test", FieldType::FLOAT32, 1);
    
    FieldHandle handle = manager->allocate(desc, 1000);
    
    size_t usage = manager->getMemoryUsage(handle);
    EXPECT_GT(usage, 0);
    
    size_t total = manager->getTotalMemoryUsage();
    EXPECT_GE(total, usage);
}

TEST_F(SOAFieldManagerTest, InvalidComponentAccess) {
    FieldDescriptor desc("test", FieldType::FLOAT32, 2);
    
    FieldHandle handle = manager->allocate(desc, 1000);
    
    // Valid access
    EXPECT_NO_THROW(manager->getDevicePtr(handle, 0));
    EXPECT_NO_THROW(manager->getDevicePtr(handle, 1));
    
    // Invalid access
    EXPECT_THROW(manager->getDevicePtr(handle, 2), std::out_of_range);
}

TEST_F(SOAFieldManagerTest, MultipleFields) {
    FieldDescriptor desc1("field1", FieldType::FLOAT32, 1);
    FieldDescriptor desc2("field2", FieldType::FLOAT64, 1);
    FieldDescriptor desc3("field3", FieldType::FLOAT32, 3);
    
    FieldHandle handle1 = manager->allocate(desc1, 1000);
    FieldHandle handle2 = manager->allocate(desc2, 2000);
    FieldHandle handle3 = manager->allocate(desc3, 3000);
    
    EXPECT_EQ(manager->getAllocationSize(handle1), 1000);
    EXPECT_EQ(manager->getAllocationSize(handle2), 2000);
    EXPECT_EQ(manager->getAllocationSize(handle3), 3000);
}
