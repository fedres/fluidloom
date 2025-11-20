#pragma once

#include "fluidloom/core/backend/IBackend.h"
#include <memory>
#include <string>

namespace fluidloom {

enum class BackendChoice {
    AUTO,    // Use OpenCL if available, else Mock
    MOCK,    // Force Mock
    OPENCL   // Force OpenCL (fail if not available)
};

class BackendFactory {
public:
    // Create backend based on environment and choice
    static std::unique_ptr<IBackend> createBackend(BackendChoice choice = BackendChoice::AUTO);
    
    // Get the singleton instance (for global access)
    static IBackend& getInstance();
    
    // Explicitly set the global backend (takes ownership)
    static void setBackend(std::unique_ptr<IBackend> backend);
    
private:
    static std::unique_ptr<IBackend>& getBackendInstance() {
        static std::unique_ptr<IBackend> instance;
        return instance;
    }
};

} // namespace fluidloom
