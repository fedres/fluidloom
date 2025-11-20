#include "fluidloom/adaptation/SplitEngine.h"
#include "fluidloom/adaptation/CellDescriptor.h"
#include "fluidloom/common/FluidLoomError.h"
#include "fluidloom/common/Logger.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <iostream>
#include <algorithm>

namespace fluidloom {
namespace adaptation {

SplitEngine::SplitEngine(cl_context context, cl_command_queue queue, const AdaptationConfig& config)
    : m_context(context), m_queue(queue), m_config(config), m_program(nullptr),
      m_kernel_count_allocate(nullptr), m_kernel_generate_children(nullptr), m_kernel_interpolate(nullptr) {
    compileKernels();
}

SplitEngine::~SplitEngine() {
    releaseResources();
}

void SplitEngine::releaseResources() {
    if (m_kernel_count_allocate) clReleaseKernel(m_kernel_count_allocate);
    if (m_kernel_generate_children) clReleaseKernel(m_kernel_generate_children);
    if (m_kernel_interpolate) clReleaseKernel(m_kernel_interpolate);
    if (m_program) clReleaseProgram(m_program);
}

std::string SplitEngine::loadKernelSource(const std::string& filename) {
    // Hardcoded path for this environment as per user context
    std::string path = "/Users/karthikt/Ideas/FluidLoom/fluidloom/src/adaptation/kernels/" + filename;
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open kernel source: " + path);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void SplitEngine::compileKernels() {
    // Load sources
    std::string hilbert_src = loadKernelSource("hilbert_encode_3d.cl");
    std::string split_src = loadKernelSource("split_cells.cl");
    
    // Combine sources. We prepend hilbert_src.
    // We also need to comment out the #include "hilbert_encode_3d.cl" in split_cells.cl to avoid errors
    size_t include_pos = split_src.find("#include \"hilbert_encode_3d.cl\"");
    if (include_pos != std::string::npos) {
        split_src.replace(include_pos, 29, "// #include \"hilbert_encode_3d.cl\"");
    }
    
    std::string full_src = hilbert_src + "\n" + split_src;
    
    // Create program
    const char* src_str = full_src.c_str();
    size_t src_len = full_src.length();
    cl_int err;
    m_program = clCreateProgramWithSource(m_context, 1, &src_str, &src_len, &err);
    if (err != CL_SUCCESS) throw std::runtime_error("Failed to create program");
    
    // Build program
    err = clBuildProgram(m_program, 0, nullptr, "-cl-std=CL1.2", nullptr, nullptr);
    if (err != CL_SUCCESS) {
        // Get build log
        size_t device_size;
        clGetContextInfo(m_context, CL_CONTEXT_DEVICES, 0, nullptr, &device_size);
        std::vector<cl_device_id> devices(device_size / sizeof(cl_device_id));
        clGetContextInfo(m_context, CL_CONTEXT_DEVICES, device_size, devices.data(), nullptr);
        
        if (!devices.empty()) {
             size_t log_size;
             clGetProgramBuildInfo(m_program, devices[0], CL_PROGRAM_BUILD_LOG, 0, nullptr, &log_size);
             std::vector<char> log(log_size + 1);
             clGetProgramBuildInfo(m_program, devices[0], CL_PROGRAM_BUILD_LOG, log_size, log.data(), nullptr);
             log[log_size] = '\0';
             FL_LOG(ERROR) << "Build log: " << log.data();
        }
        throw std::runtime_error("Failed to build SplitEngine program");
    }
    
    // Create kernels
    m_kernel_count_allocate = clCreateKernel(m_program, "split_count_and_allocate", &err);
    if (err != CL_SUCCESS) throw std::runtime_error("Failed to create split_count_and_allocate kernel");
    
    m_kernel_generate_children = clCreateKernel(m_program, "split_generate_children", &err);
    if (err != CL_SUCCESS) throw std::runtime_error("Failed to create split_generate_children kernel");
    
    m_kernel_interpolate = clCreateKernel(m_program, "interpolate_split_fields", &err);
    if (err != CL_SUCCESS) throw std::runtime_error("Failed to create interpolate_split_fields kernel");
}

SplitResult SplitEngine::split(
    cl_mem parent_x, cl_mem parent_y, cl_mem parent_z,
    cl_mem parent_level, cl_mem parent_states,
    cl_mem refine_flags,
    cl_mem parent_material_id,
    size_t num_parents,
    cl_mem parent_fields,
    uint32_t num_field_components
) {
    SplitResult result;
    cl_int err;
    
    if (num_parents == 0) return result;
    
    // 1. Allocate temporary buffers
    cl_mem cell_scratch = clCreateBuffer(m_context, CL_MEM_READ_WRITE, num_parents * sizeof(uint32_t), nullptr, &err);
    if (err != CL_SUCCESS) throw std::runtime_error("Failed to allocate cell_scratch");
    
    cl_mem child_block_start = clCreateBuffer(m_context, CL_MEM_READ_WRITE, num_parents * sizeof(uint32_t), nullptr, &err);
    if (err != CL_SUCCESS) {
        clReleaseMemObject(cell_scratch);
        throw std::runtime_error("Failed to allocate child_block_start");
    }
    
    // 2. Run count kernel
    clSetKernelArg(m_kernel_count_allocate, 0, sizeof(cl_mem), &parent_x);
    clSetKernelArg(m_kernel_count_allocate, 1, sizeof(cl_mem), &parent_y);
    clSetKernelArg(m_kernel_count_allocate, 2, sizeof(cl_mem), &parent_z);
    clSetKernelArg(m_kernel_count_allocate, 3, sizeof(cl_mem), &parent_level);
    clSetKernelArg(m_kernel_count_allocate, 4, sizeof(cl_mem), &refine_flags);
    clSetKernelArg(m_kernel_count_allocate, 5, sizeof(cl_mem), &parent_states);
    clSetKernelArg(m_kernel_count_allocate, 6, sizeof(cl_mem), &child_block_start);
    clSetKernelArg(m_kernel_count_allocate, 7, sizeof(cl_mem), &cell_scratch);
    cl_uint num_parents_uint = static_cast<cl_uint>(num_parents);
    clSetKernelArg(m_kernel_count_allocate, 8, sizeof(cl_uint), &num_parents_uint);
    
    size_t global_work_size = ((num_parents + 255) / 256) * 256;
    size_t local_work_size = 256;
    
    err = clEnqueueNDRangeKernel(m_queue, m_kernel_count_allocate, 1, nullptr, &global_work_size, &local_work_size, 0, nullptr, nullptr);
    if (err != CL_SUCCESS) throw std::runtime_error("Failed to enqueue count kernel");
    
    // 3. Read back scratch buffer for host-side scan
    std::vector<uint32_t> host_scratch(num_parents);
    err = clEnqueueReadBuffer(m_queue, cell_scratch, CL_TRUE, 0, num_parents * sizeof(uint32_t), host_scratch.data(), 0, nullptr, nullptr);
    if (err != CL_SUCCESS) throw std::runtime_error("Failed to read scratch buffer");
    
    // 4. Perform prefix sum on host
    std::vector<uint32_t> host_block_start(num_parents);
    uint32_t total_children = 0;
    uint32_t parents_split_count = 0;
    
    result.parent_to_child_map.resize(num_parents);
    result.split_parent_indices.reserve(num_parents / 8); // Heuristic
    
    for (size_t i = 0; i < num_parents; ++i) {
        if (host_scratch[i] == 1) {
            host_block_start[i] = total_children;
            result.parent_to_child_map[i] = total_children;
            result.split_parent_indices.push_back(i);
            total_children += 8;
            parents_split_count++;
        } else {
            host_block_start[i] = 0xFFFFFFFF; // INVALID_INDEX
            result.parent_to_child_map[i] = 0xFFFFFFFF;
        }
    }
    
    result.num_children = total_children;
    result.num_parents_split = parents_split_count;
    
    if (total_children == 0) {
        clReleaseMemObject(cell_scratch);
        clReleaseMemObject(child_block_start);
        return result;
    }
    
    // 5. Write back block start indices
    err = clEnqueueWriteBuffer(m_queue, child_block_start, CL_TRUE, 0, num_parents * sizeof(uint32_t), host_block_start.data(), 0, nullptr, nullptr);
    if (err != CL_SUCCESS) throw std::runtime_error("Failed to write block start buffer");
    
    // 6. Allocate child buffers
    cl_mem child_x = clCreateBuffer(m_context, CL_MEM_READ_WRITE, total_children * sizeof(int), nullptr, &err);
    cl_mem child_y = clCreateBuffer(m_context, CL_MEM_READ_WRITE, total_children * sizeof(int), nullptr, &err);
    cl_mem child_z = clCreateBuffer(m_context, CL_MEM_READ_WRITE, total_children * sizeof(int), nullptr, &err);
    cl_mem child_level = clCreateBuffer(m_context, CL_MEM_READ_WRITE, total_children * sizeof(uint8_t), nullptr, &err);
    cl_mem child_states = clCreateBuffer(m_context, CL_MEM_READ_WRITE, total_children * sizeof(uint8_t), nullptr, &err);
    cl_mem child_mat_id = clCreateBuffer(m_context, CL_MEM_READ_WRITE, total_children * sizeof(uint32_t), nullptr, &err);
    cl_mem child_hilbert = clCreateBuffer(m_context, CL_MEM_READ_WRITE, total_children * sizeof(uint64_t), nullptr, &err);
    
    // 7. Run generate children kernel
    clSetKernelArg(m_kernel_generate_children, 0, sizeof(cl_mem), &parent_x);
    clSetKernelArg(m_kernel_generate_children, 1, sizeof(cl_mem), &parent_y);
    clSetKernelArg(m_kernel_generate_children, 2, sizeof(cl_mem), &parent_z);
    clSetKernelArg(m_kernel_generate_children, 3, sizeof(cl_mem), &parent_level);
    clSetKernelArg(m_kernel_generate_children, 4, sizeof(cl_mem), &parent_states);
    clSetKernelArg(m_kernel_generate_children, 5, sizeof(cl_mem), &parent_material_id);
    clSetKernelArg(m_kernel_generate_children, 6, sizeof(cl_mem), &child_block_start);
    clSetKernelArg(m_kernel_generate_children, 7, sizeof(cl_mem), &child_x);
    clSetKernelArg(m_kernel_generate_children, 8, sizeof(cl_mem), &child_y);
    clSetKernelArg(m_kernel_generate_children, 9, sizeof(cl_mem), &child_z);
    clSetKernelArg(m_kernel_generate_children, 10, sizeof(cl_mem), &child_level);
    clSetKernelArg(m_kernel_generate_children, 11, sizeof(cl_mem), &child_states);
    clSetKernelArg(m_kernel_generate_children, 12, sizeof(cl_mem), &child_mat_id);
    clSetKernelArg(m_kernel_generate_children, 13, sizeof(cl_mem), &child_hilbert);
    clSetKernelArg(m_kernel_generate_children, 14, sizeof(cl_uint), &num_parents_uint);
    
    global_work_size = ((num_parents + 255) / 256) * 256;
    err = clEnqueueNDRangeKernel(m_queue, m_kernel_generate_children, 1, nullptr, &global_work_size, &local_work_size, 0, nullptr, nullptr);
    if (err != CL_SUCCESS) throw std::runtime_error("Failed to enqueue generate children kernel");
    
    // 8. Interpolate fields if provided
    if (parent_fields && num_field_components > 0) {
        cl_mem child_fields = clCreateBuffer(m_context, CL_MEM_READ_WRITE, total_children * num_field_components * sizeof(float), nullptr, &err);
        
        clSetKernelArg(m_kernel_interpolate, 0, sizeof(cl_mem), &child_block_start);
        clSetKernelArg(m_kernel_interpolate, 1, sizeof(cl_mem), &parent_fields);
        clSetKernelArg(m_kernel_interpolate, 2, sizeof(cl_mem), &child_fields);
        clSetKernelArg(m_kernel_interpolate, 3, sizeof(cl_uint), &num_field_components);
        clSetKernelArg(m_kernel_interpolate, 4, sizeof(cl_uint), &num_parents_uint);
        
        err = clEnqueueNDRangeKernel(m_queue, m_kernel_interpolate, 1, nullptr, &global_work_size, &local_work_size, 0, nullptr, nullptr);
        if (err != CL_SUCCESS) throw std::runtime_error("Failed to enqueue interpolate kernel");
        
        clReleaseMemObject(child_fields);
    }
    
    // 9. Read back results
    std::vector<int> h_child_x(total_children);
    std::vector<int> h_child_y(total_children);
    std::vector<int> h_child_z(total_children);
    std::vector<uint8_t> h_child_level(total_children);
    std::vector<uint8_t> h_child_states(total_children);
    std::vector<uint32_t> h_child_mat_id(total_children);
    std::vector<uint64_t> h_child_hilbert(total_children);
    
    clEnqueueReadBuffer(m_queue, child_x, CL_TRUE, 0, total_children * sizeof(int), h_child_x.data(), 0, nullptr, nullptr);
    clEnqueueReadBuffer(m_queue, child_y, CL_TRUE, 0, total_children * sizeof(int), h_child_y.data(), 0, nullptr, nullptr);
    clEnqueueReadBuffer(m_queue, child_z, CL_TRUE, 0, total_children * sizeof(int), h_child_z.data(), 0, nullptr, nullptr);
    clEnqueueReadBuffer(m_queue, child_level, CL_TRUE, 0, total_children * sizeof(uint8_t), h_child_level.data(), 0, nullptr, nullptr);
    clEnqueueReadBuffer(m_queue, child_states, CL_TRUE, 0, total_children * sizeof(uint8_t), h_child_states.data(), 0, nullptr, nullptr);
    clEnqueueReadBuffer(m_queue, child_mat_id, CL_TRUE, 0, total_children * sizeof(uint32_t), h_child_mat_id.data(), 0, nullptr, nullptr);
    clEnqueueReadBuffer(m_queue, child_hilbert, CL_TRUE, 0, total_children * sizeof(uint64_t), h_child_hilbert.data(), 0, nullptr, nullptr);
    
    // Populate result.children
    result.children.resize(total_children);
    for (size_t i = 0; i < total_children; ++i) {
        CellDescriptor& cell = result.children[i];
        cell.x = h_child_x[i];
        cell.y = h_child_y[i];
        cell.z = h_child_z[i];
        cell.level = h_child_level[i];
        cell.state = h_child_states[i];
        cell.material_id = h_child_mat_id[i];
        cell.visited = 0;
        cell.reserved = 0;
    }
    
    result.success = true;
    
    // Cleanup
    clReleaseMemObject(cell_scratch);
    clReleaseMemObject(child_block_start);
    clReleaseMemObject(child_x);
    clReleaseMemObject(child_y);
    clReleaseMemObject(child_z);
    clReleaseMemObject(child_level);
    clReleaseMemObject(child_states);
    clReleaseMemObject(child_mat_id);
    clReleaseMemObject(child_hilbert);
    
    return result;
}

} // namespace adaptation
} // namespace fluidloom
