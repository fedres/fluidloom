#include "fluidloom/adaptation/BalanceEnforcer.h"
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

BalanceEnforcer::BalanceEnforcer(cl_context context, cl_command_queue queue, const AdaptationConfig& config)
    : m_context(context), m_queue(queue), m_config(config), m_program(nullptr),
      m_kernel_detect_violations(nullptr), m_kernel_mark_cascading(nullptr), m_kernel_update_shadow_levels(nullptr),
      m_hash_table(nullptr), m_hash_table_size(0) {
    compileKernels();
}

BalanceEnforcer::~BalanceEnforcer() {
    releaseResources();
}

void BalanceEnforcer::releaseResources() {
    if (m_kernel_detect_violations) clReleaseKernel(m_kernel_detect_violations);
    if (m_kernel_mark_cascading) clReleaseKernel(m_kernel_mark_cascading);
    if (m_kernel_update_shadow_levels) clReleaseKernel(m_kernel_update_shadow_levels);
    if (m_program) clReleaseProgram(m_program);
    if (m_hash_table) clReleaseMemObject(m_hash_table);
}

std::string BalanceEnforcer::loadKernelSource(const std::string& filename) {
    std::string path = "/Users/karthikt/Ideas/FluidLoom/fluidloom/src/adaptation/kernels/" + filename;
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open kernel source: " + path);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void BalanceEnforcer::compileKernels() {
    std::string hilbert_src = loadKernelSource("hilbert_encode_3d.cl");
    std::string balance_src = loadKernelSource("balance_enforce.cl");
    
    size_t include_pos = balance_src.find("#include \"hilbert_encode_3d.cl\"");
    if (include_pos != std::string::npos) {
        balance_src.replace(include_pos, 29, "// #include \"hilbert_encode_3d.cl\"");
    }
    
    std::string update_shadow_src = 
        "__kernel void update_shadow_levels("
        "    __global const uchar* restrict levels,"
        "    __global const int* restrict refine_flags,"
        "    __global uchar* restrict shadow_levels,"
        "    const uint num_cells) {"
        "    const uint idx = get_global_id(0);"
        "    if (idx >= num_cells) return;"
        "    shadow_levels[idx] = levels[idx] + (refine_flags[idx] > 0 ? 1 : 0);"
        "}";
        
    std::string full_src = hilbert_src + "\n" + balance_src + "\n" + update_shadow_src;
    
    const char* src_str = full_src.c_str();
    size_t src_len = full_src.length();
    cl_int err;
    m_program = clCreateProgramWithSource(m_context, 1, &src_str, &src_len, &err);
    if (err != CL_SUCCESS) throw std::runtime_error("Failed to create program");
    
    err = clBuildProgram(m_program, 0, nullptr, "-cl-std=CL1.2", nullptr, nullptr);
    if (err != CL_SUCCESS) {
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
        throw std::runtime_error("Failed to build BalanceEnforcer program");
    }
    
    m_kernel_detect_violations = clCreateKernel(m_program, "detect_balance_violations", &err);
    if (err != CL_SUCCESS) throw std::runtime_error("Failed to create detect_balance_violations kernel");
    
    m_kernel_mark_cascading = clCreateKernel(m_program, "mark_cascading_refinement", &err);
    if (err != CL_SUCCESS) throw std::runtime_error("Failed to create mark_cascading_refinement kernel");

    m_kernel_update_shadow_levels = clCreateKernel(m_program, "update_shadow_levels", &err);
    if (err != CL_SUCCESS) throw std::runtime_error("Failed to create update_shadow_levels kernel");
}

void BalanceEnforcer::buildHashTable(cl_mem x, cl_mem y, cl_mem z, size_t num_cells) {
    // Host-side hash table build
    std::vector<int> h_x(num_cells), h_y(num_cells), h_z(num_cells);
    clEnqueueReadBuffer(m_queue, x, CL_TRUE, 0, num_cells * sizeof(int), h_x.data(), 0, nullptr, nullptr);
    clEnqueueReadBuffer(m_queue, y, CL_TRUE, 0, num_cells * sizeof(int), h_y.data(), 0, nullptr, nullptr);
    clEnqueueReadBuffer(m_queue, z, CL_TRUE, 0, num_cells * sizeof(int), h_z.data(), 0, nullptr, nullptr);
    
    size_t table_size = 1;
    while (table_size < num_cells * 2) table_size *= 2;
    if (table_size < 1024) table_size = 1024;
    
    std::vector<uint32_t> h_table(table_size, INVALID_INDEX);
    
    for (size_t i = 0; i < num_cells; ++i) {
        uint64_t hilbert = hilbert_encode_3d(h_x[i], h_y[i], h_z[i], MAX_REFINEMENT_LEVEL);
        uint32_t hash = hilbert % table_size;
        // Linear probing
        size_t probes = 0;
        while (h_table[hash] != INVALID_INDEX && probes < table_size) {
            hash = (hash + 1) % table_size;
            probes++;
        }
        
        if (h_table[hash] == INVALID_INDEX) {
            h_table[hash] = static_cast<uint32_t>(i);
        } else {
            FL_LOG(ERROR) << "Hash table full in BalanceEnforcer!";
        }
    }
    
    if (m_hash_table && m_hash_table_size != table_size) {
        clReleaseMemObject(m_hash_table);
        m_hash_table = nullptr;
    }
    
    if (!m_hash_table) {
        cl_int err;
        m_hash_table = clCreateBuffer(m_context, CL_MEM_READ_WRITE, table_size * sizeof(uint32_t), nullptr, &err);
        if (err != CL_SUCCESS) throw std::runtime_error("Failed to allocate hash table");
        m_hash_table_size = table_size;
    }
    
    clEnqueueWriteBuffer(m_queue, m_hash_table, CL_TRUE, 0, table_size * sizeof(uint32_t), h_table.data(), 0, nullptr, nullptr);
}

BalanceResult BalanceEnforcer::enforce(
    cl_mem coord_x, cl_mem coord_y, cl_mem coord_z,
    cl_mem levels, cl_mem cell_states,
    cl_mem refine_flags,
    size_t num_cells
) {
    BalanceResult result;
    cl_int err;
    
    if (num_cells == 0) return result;
    
    // 1. Build hash table
    buildHashTable(coord_x, coord_y, coord_z, num_cells);
    
    // 2. Allocate temporary buffers
    cl_mem violation_flags = clCreateBuffer(m_context, CL_MEM_READ_WRITE, num_cells * sizeof(uint8_t), nullptr, &err);
    cl_mem violation_count = clCreateBuffer(m_context, CL_MEM_READ_WRITE, sizeof(uint32_t), nullptr, &err);
    cl_mem marked_count = clCreateBuffer(m_context, CL_MEM_READ_WRITE, sizeof(uint32_t), nullptr, &err);
    
    // Shadow levels buffer for cascading
    cl_mem shadow_levels = clCreateBuffer(m_context, CL_MEM_READ_WRITE, num_cells * sizeof(uint8_t), nullptr, &err);
    // Initialize shadow levels with current levels
    clEnqueueCopyBuffer(m_queue, levels, shadow_levels, 0, 0, num_cells * sizeof(uint8_t), 0, nullptr, nullptr);
    
    size_t global_work_size = ((num_cells + 255) / 256) * 256;
    size_t local_work_size = 256;
    
    // 3. Iterative loop
    for (uint32_t iter = 0; iter < m_config.max_balance_iterations; ++iter) {
        // Reset counters
        uint32_t zero = 0;
        clEnqueueWriteBuffer(m_queue, violation_count, CL_TRUE, 0, sizeof(uint32_t), &zero, 0, nullptr, nullptr);
        clEnqueueWriteBuffer(m_queue, marked_count, CL_TRUE, 0, sizeof(uint32_t), &zero, 0, nullptr, nullptr);
        
        // A. Detect violations using SHADOW levels
        clSetKernelArg(m_kernel_detect_violations, 0, sizeof(cl_mem), &coord_x);
        clSetKernelArg(m_kernel_detect_violations, 1, sizeof(cl_mem), &coord_y);
        clSetKernelArg(m_kernel_detect_violations, 2, sizeof(cl_mem), &coord_z);
        clSetKernelArg(m_kernel_detect_violations, 3, sizeof(cl_mem), &shadow_levels); // Use shadow levels
        clSetKernelArg(m_kernel_detect_violations, 4, sizeof(cl_mem), &cell_states);
        clSetKernelArg(m_kernel_detect_violations, 5, sizeof(cl_mem), nullptr); // cell_hilbert
        clSetKernelArg(m_kernel_detect_violations, 6, sizeof(cl_mem), &m_hash_table);
        cl_uint table_size_uint = static_cast<cl_uint>(m_hash_table_size);
        clSetKernelArg(m_kernel_detect_violations, 7, sizeof(cl_uint), &table_size_uint);
        clSetKernelArg(m_kernel_detect_violations, 8, sizeof(cl_mem), &violation_flags);
        clSetKernelArg(m_kernel_detect_violations, 9, sizeof(cl_mem), &violation_count);
        cl_uint num_cells_uint = static_cast<cl_uint>(num_cells);
        clSetKernelArg(m_kernel_detect_violations, 10, sizeof(cl_uint), &num_cells_uint);
        
        err = clEnqueueNDRangeKernel(m_queue, m_kernel_detect_violations, 1, nullptr, &global_work_size, &local_work_size, 0, nullptr, nullptr);
        if (err != CL_SUCCESS) throw std::runtime_error("Failed to enqueue detect violations kernel");
        
        // Read violation count
        uint32_t num_violations = 0;
        clEnqueueReadBuffer(m_queue, violation_count, CL_TRUE, 0, sizeof(uint32_t), &num_violations, 0, nullptr, nullptr);
        
        result.total_violations_detected += num_violations;
        
        if (num_violations == 0) {
            result.converged = true;
            result.iterations = iter + 1;
            break;
        }
        
        // B. Mark cascading refinement
        // Note: mark_cascading uses 'levels' to check if max level reached. 
        // It should arguably use 'shadow_levels' too, but using 'levels' is safe (conservative).
        // Actually, if we are at L=7 and shadow=8, we can't refine further.
        // So using 'levels' is correct for capacity check.
        clSetKernelArg(m_kernel_mark_cascading, 0, sizeof(cl_mem), &coord_x);
        clSetKernelArg(m_kernel_mark_cascading, 1, sizeof(cl_mem), &coord_y);
        clSetKernelArg(m_kernel_mark_cascading, 2, sizeof(cl_mem), &coord_z);
        clSetKernelArg(m_kernel_mark_cascading, 3, sizeof(cl_mem), &levels);
        clSetKernelArg(m_kernel_mark_cascading, 4, sizeof(cl_mem), &cell_states);
        clSetKernelArg(m_kernel_mark_cascading, 5, sizeof(cl_mem), &violation_flags);
        clSetKernelArg(m_kernel_mark_cascading, 6, sizeof(cl_mem), &refine_flags);
        clSetKernelArg(m_kernel_mark_cascading, 7, sizeof(cl_mem), &marked_count);
        clSetKernelArg(m_kernel_mark_cascading, 8, sizeof(cl_uint), &num_cells_uint);
        
        err = clEnqueueNDRangeKernel(m_queue, m_kernel_mark_cascading, 1, nullptr, &global_work_size, &local_work_size, 0, nullptr, nullptr);
        if (err != CL_SUCCESS) throw std::runtime_error("Failed to enqueue mark cascading kernel");
        
        // Read marked count
        uint32_t num_marked = 0;
        clEnqueueReadBuffer(m_queue, marked_count, CL_TRUE, 0, sizeof(uint32_t), &num_marked, 0, nullptr, nullptr);
        
        result.total_cells_marked_for_balance += num_marked;
        
        // Record stats
        BalanceResult::IterationStats stats;
        stats.violations_this_iter = num_violations;
        stats.cells_marked_this_iter = num_marked;
        result.per_iteration_stats.push_back(stats);
        
        if (num_marked == 0) {
            // Violations detected but no cells marked? This might happen if all violators are already at max level or locked.
            FL_LOG(WARN) << "Balance enforcement: Violations detected but no cells could be marked. Stopping.";
            result.converged = false;
            result.iterations = iter + 1;
            break;
        }
        
        // C. Update shadow levels
        clSetKernelArg(m_kernel_update_shadow_levels, 0, sizeof(cl_mem), &levels);
        clSetKernelArg(m_kernel_update_shadow_levels, 1, sizeof(cl_mem), &refine_flags);
        clSetKernelArg(m_kernel_update_shadow_levels, 2, sizeof(cl_mem), &shadow_levels);
        clSetKernelArg(m_kernel_update_shadow_levels, 3, sizeof(cl_uint), &num_cells_uint);
        
        err = clEnqueueNDRangeKernel(m_queue, m_kernel_update_shadow_levels, 1, nullptr, &global_work_size, &local_work_size, 0, nullptr, nullptr);
        if (err != CL_SUCCESS) throw std::runtime_error("Failed to enqueue update shadow levels kernel");
    }
    
    clReleaseMemObject(violation_flags);
    clReleaseMemObject(violation_count);
    clReleaseMemObject(marked_count);
    clReleaseMemObject(shadow_levels);
    
    return result;
}

} // namespace adaptation
} // namespace fluidloom
