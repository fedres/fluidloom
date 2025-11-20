#include "fluidloom/core/backend/BackendFactory.h"
#include "fluidloom/core/backend/MockBackend.h"
#include "fluidloom/core/backend/OpenCLBackend.h"
#include "fluidloom/common/Logger.h"
#include <cstdlib>

namespace fluidloom {

std::unique_ptr<IBackend> BackendFactory::createBackend(BackendChoice choice) {
    switch (choice) {
        case BackendChoice::MOCK:
            FL_LOG(INFO) << "Creating MockBackend (explicit)";
            return std::make_unique<MockBackend>();
            
        case BackendChoice::OPENCL:
            FL_LOG(INFO) << "Creating OpenCLBackend (explicit)";
            return std::make_unique<OpenCLBackend>();
            
        case BackendChoice::AUTO:
        default:
            // Check environment variable first
            const char* env_backend = std::getenv("FL_BACKEND");
            if (env_backend) {
                std::string backend_str(env_backend);
                if (backend_str == "MOCK") {
                    FL_LOG(INFO) << "FL_BACKEND=MOCK, creating MockBackend";
                    return std::make_unique<MockBackend>();
                } else if (backend_str == "OPENCL") {
                    FL_LOG(INFO) << "FL_BACKEND=OPENCL, creating OpenCLBackend";
                    return std::make_unique<OpenCLBackend>();
                } else {
                    FL_LOG(WARN) << "Unknown FL_BACKEND value: " << backend_str << ", falling back to AUTO";
                }
            }
            
            // Try OpenCL first
            try {
                auto backend = std::make_unique<OpenCLBackend>();
                FL_LOG(INFO) << "Auto-detected OpenCL, creating OpenCLBackend";
                return backend;
            } catch (const std::exception& e) {
                FL_LOG(INFO) << "OpenCL not available (" << e.what() << "), falling back to MockBackend";
                return std::make_unique<MockBackend>();
            }
    }
}

IBackend& BackendFactory::getInstance() {
    auto& instance = getBackendInstance();
    if (!instance) {
        // Lazy initialization with AUTO choice
        instance = createBackend(BackendChoice::AUTO);
    }
    return *instance;
}

void BackendFactory::setBackend(std::unique_ptr<IBackend> backend) {
    getBackendInstance() = std::move(backend);
}

} // namespace fluidloom
