#include <gtest/gtest.h>
#include "fluidloom/io/VTKWriter.h"
#include "fluidloom/core/fields/SOAFieldManager.h"
#include "fluidloom/core/backend/OpenCLBackend.h"
#include <filesystem>
#include <fstream>

using namespace fluidloom;
using namespace fluidloom::io;

TEST(VTKTest, GeneratesVTPFile) {
    // Setup
    std::string output_dir = "test_output";
    if (std::filesystem::exists(output_dir)) {
        std::filesystem::remove_all(output_dir);
    }
    
    VTKWriter writer(output_dir);
    
    // Create dummy cells
    std::vector<CellCoord> cells;
    cells.push_back({0, 0, 0, 0});
    cells.push_back({1, 0, 0, 0});
    cells.push_back({0, 1, 0, 0});
    cells.push_back({0, 0, 1, 0});
    
    // Create dummy field manager (mock backend would be better, but using OpenCLBackend if avail)
    // For this test, we just pass a field manager, but since we haven't implemented field writing yet,
    // we can pass a partially initialized one or mock it.
    // Actually, VTKWriter takes const ref, so we need a real object.
    
    // We need a context for FieldManager.
    // Let's try to use MockBackend if possible, or just skip field writing for now.
    // The current VTKWriter implementation doesn't write fields yet (TODO comment).
    
    // We need to construct SOAFieldManager. It needs IBackend*.
    // Let's create a minimal backend or use nullptr if not dereferenced.
    // SOAFieldManager constructor uses backend to allocate things?
    // It takes IBackend*.
    
    // Let's just pass nullptr for backend for now, assuming we don't trigger allocation in this test.
    // Wait, SOAFieldManager might use backend in constructor?
    // Checked SimulationBuilder: m_field_manager = std::make_unique<fields::SOAFieldManager>(m_backend.get());
    
    // Let's check SOAFieldManager constructor.
    // If it's safe, we pass nullptr. If not, we need a MockBackend.
    
    // I'll skip FieldManager for now and pass a dummy one if I can, or just modify VTKWriter signature for test?
    // No, stick to signature.
    
    // I'll try to create a MockBackend.
    // fluidloom/core/backend/MockBackend.h
    
    // But MockBackend might not be exposed easily.
    // I'll just pass nullptr and hope it doesn't crash since we don't write fields yet.
    
    // Wait, I can't pass nullptr to reference. I need an object.
    // I'll create a dummy class inheriting from IBackend? No, too much work.
    
    // I'll use OpenCLBackend if available, or just rely on the fact that we don't call methods on it yet.
    // Actually, I'll just instantiate SOAFieldManager with a dummy backend pointer cast.
    // This is risky.
    
    // Better: Modify VTKWriter to take `const fields::SOAFieldManager*` and pass nullptr.
    // But signature is `const fields::SOAFieldManager&`.
    
    // I'll check if I can use MockBackend.
    
    // For now, I'll just comment out the field manager part in the test and pass a reinterpret_cast reference?
    // Ugly but works for "GeneratesVTPFile" if we only test geometry.
    
    // I'll try to include MockBackend.h.
    
    // Let's assume I can't easily instantiate FieldManager without a backend.
    // I will modify VTKWriter to NOT take FieldManager for now, or make it optional?
    // No, I should fix the test properly.
    
    // I'll create a minimal MockBackend in the test file.
    
    class MinimalBackend : public IBackend {
    public:
        BackendType getType() const override { return BackendType::MOCK; }
        std::string getName() const override { return "Minimal"; }
        int getDeviceCount() const override { return 1; }
        void initialize(int) override {}
        void shutdown() override {}
        DeviceBufferPtr allocateBuffer(size_t, const void*) override { return nullptr; }
        void copyHostToDevice(const void*, DeviceBuffer&, size_t) override {}
        void copyDeviceToHost(const DeviceBuffer&, void*, size_t) override {}
        void copyDeviceToDevice(const DeviceBuffer&, DeviceBuffer&, size_t) override {}
        void flush() override {}
        void finish() override {}
        size_t getMaxAllocationSize() const override { return 1024; }
        size_t getTotalMemory() const override { return 1024; }
        bool isInitialized() const override { return true; }
        KernelHandle compileKernel(const std::string&, const std::string&, const std::string&) override { return KernelHandle(nullptr); }
        void launchKernel(const KernelHandle&, size_t, size_t, const std::vector<KernelArg>&) override {}
        void releaseKernel(const KernelHandle&) override {}
    };
    
    MinimalBackend backend;
    fields::SOAFieldManager field_manager(&backend);
    
    writer.writeSnapshot(0, 0.0, cells, field_manager);
    
    // Verify file exists
    std::string expected_file = output_dir + "/snapshot_000000.vtp";
    ASSERT_TRUE(std::filesystem::exists(expected_file));
    
    // Verify content
    std::ifstream file(expected_file);
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    
    EXPECT_TRUE(content.find("<VTKFile") != std::string::npos);
    EXPECT_TRUE(content.find("NumberOfPoints=\"4\"") != std::string::npos);
}
