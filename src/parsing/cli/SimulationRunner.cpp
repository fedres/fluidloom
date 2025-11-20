#include "fluidloom/parsing/SimulationBuilder.h"
#include "fluidloom/common/Logger.h"
#include <iostream>
#include <fstream>
#include <sstream>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

using namespace fluidloom;

int main(int argc, char** argv) {
    std::string filename = (argc >= 2) ? argv[1] : "benchmarks/lid_driven_cavity.fl";
    
    FL_LOG(INFO) << "FluidLoom AMR with ANTLR Parser";
    FL_LOG(INFO) << "Loading: " << filename;
    
    // Read .fl file
    std::string script_content;
    std::ifstream file(filename);
    if (file.is_open()) {
        std::stringstream buffer;
        buffer << file.rdbuf();
        script_content = buffer.str();
        FL_LOG(INFO) << "Loaded " << script_content.length() << " bytes from " << filename;
    } else {
        FL_LOG(WARN) << "Could not open " << filename << ", using stub";
    }
    
    // Initialize OpenCL
    cl_int err;
    cl_platform_id platform;
    clGetPlatformIDs(1, &platform, nullptr);
    
    cl_device_id device;
    clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, nullptr);
    
    cl_context context = clCreateContext(nullptr, 1, &device, nullptr, nullptr, &err);
    cl_command_queue queue = clCreateCommandQueue(context, device, 0, &err);
    
    FL_LOG(INFO) << "OpenCL context initialized";
    
    try {
        // Build execution graph
        parsing::SimulationBuilder builder(context, queue);
        auto graph = script_content.empty() ? builder.buildStub() : builder.buildFromScript(script_content);
        
        FL_LOG(INFO) << "Execution graph built with " << graph->getNodeCount() << " nodes";
        
        // Execute
        FL_LOG(INFO) << "Executing simulation...";
        graph->execute();
        
        FL_LOG(INFO) << "Simulation complete!";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    // Cleanup
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    
    return 0;
}
