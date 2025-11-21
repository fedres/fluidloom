#pragma once

#include "fluidloom/core/backend/IBackend.h"
#include "fluidloom/core/backend/BackendFactory.h"
#include &lt;memory&gt;

namespace fluidloom {
namespace testing {

/**
 * @brief Adapter to convert BackendType to BackendChoice
 * 
 * TestHarness uses BackendType enum, but BackendFactory uses BackendChoice.
 * This adapter provides the conversion layer.
 */
class BackendAdapter {
public:
    static std::unique_ptr&lt;IBackend&gt; create(BackendType type) {
        BackendChoice choice;
        
        switch (type) {
            case BackendType::MOCK:
                choice = BackendChoice::MOCK;
                break;
            case BackendType::OPENCL:
                choice = BackendChoice::OPENCL;
                break;
            default:
                choice = BackendChoice::AUTO;
                break;
        }
        
        return BackendFactory::createBackend(choice);
    }
};

} // namespace testing
} // namespace fluidloom
