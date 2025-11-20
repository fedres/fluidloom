#pragma once

#include <stdexcept>
#include <string>
#include <sstream>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

namespace fluidloom {

class FluidLoomError : public std::runtime_error {
public:
    FluidLoomError(const std::string& message, const std::string& file, int line)
        : std::runtime_error(formatMessage(message, file, line))
        , m_file(file)
        , m_line(line) {}

    const std::string& getFile() const { return m_file; }
    int getLine() const { return m_line; }

private:
    std::string formatMessage(const std::string& msg, const std::string& file, int line) {
        std::ostringstream oss;
        oss << "[FluidLoomError] " << msg << " at " << file << ":" << line;
        return oss.str();
    }

    std::string m_file;
    int m_line;
};

// Specific exception types for Module 1
class BackendError : public FluidLoomError {
public:
    BackendError(const std::string& msg, const std::string& file, int line)
        : FluidLoomError("[Backend] " + msg, file, line) {}
};

class OpenCLError : public BackendError {
public:
    OpenCLError(cl_int error_code, const std::string& msg, const std::string& file, int line)
        : BackendError(formatOpenCLMessage(error_code, msg), file, line)
        , m_error_code(error_code) {}

    cl_int getErrorCode() const { return m_error_code; }

private:
    static std::string formatOpenCLMessage(cl_int err, const std::string& msg) {
        const char* error_str = getOpenCLErrorString(err);
        std::ostringstream oss;
        oss << msg << " (OpenCL error: " << error_str << ")";
        return oss.str();
    }

    static const char* getOpenCLErrorString(cl_int error) {
        // Return readable error strings
        switch (error) {
            case CL_SUCCESS: return "CL_SUCCESS";
            case CL_DEVICE_NOT_FOUND: return "CL_DEVICE_NOT_FOUND";
            case CL_DEVICE_NOT_AVAILABLE: return "CL_DEVICE_NOT_AVAILABLE";
            case CL_COMPILER_NOT_AVAILABLE: return "CL_COMPILER_NOT_AVAILABLE";
            case CL_MEM_OBJECT_ALLOCATION_FAILURE: return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
            case CL_OUT_OF_RESOURCES: return "CL_OUT_OF_RESOURCES";
            case CL_OUT_OF_HOST_MEMORY: return "CL_OUT_OF_HOST_MEMORY";
            default: return "Unknown OpenCL error";
        }
    }

    cl_int m_error_code;
};

// Macro for throwing with file/line information
#define FL_THROW(exception_type, message) \
    throw exception_type(message, __FILE__, __LINE__)

#define FL_THROW_OPENCL(error_code, message) \
    throw fluidloom::OpenCLError(error_code, message, __FILE__, __LINE__)

} // namespace fluidloom
