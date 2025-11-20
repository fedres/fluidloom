#include "fluidloom/adaptation/MergeEngine.h"
#include "fluidloom/adaptation/CellDescriptor.h"
#include "fluidloom/common/FluidLoomError.h"
#include "fluidloom/common/Logger.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <iostream>
#include <algorithm>
#include <unordered_map>

namespace fluidloom {
namespace adaptation {

MergeEngine::MergeEngine(cl_context context, cl_command_queue queue, const AdaptationConfig& config)
    : m_context(context), m_queue(queue), m_config(config), m_program(nullptr),
      m_kernel_mark_siblings(nullptr), m_kernel_merge_fields(nullptr), m_kernel_create_parents(nullptr),
      m_hash_table(nullptr), m_hash_table_size(0) {
    compileKernels();
}

MergeEngine::~MergeEngine() {
    releaseResources();
}

void MergeEngine::releaseResources() {
    if (m_kernel_mark_siblings) clReleaseKernel(m_kernel_mark_siblings);
    if (m_kernel_merge_fields) clReleaseKernel(m_kernel_merge_fields);
    if (m_kernel_create_parents) clReleaseKernel(m_kernel_create_parents);
    if (m_program) clReleaseProgram(m_program);
    if (m_hash_table) clReleaseMemObject(m_hash_table);
}

std::string MergeEngine::loadKernelSource(const std::string& filename) {
    std::string path = "/Users/karthikt/Ideas/FluidLoom/fluidloom/src/adaptation/kernels/" + filename;
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open kernel source: " + path);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void MergeEngine::compileKernels() {
    std::string hilbert_src = loadKernelSource("hilbert_encode_3d.cl");
    std::string merge_src = loadKernelSource("merge_cells.cl");
    
    size_t include_pos = merge_src.find("#include \"hilbert_encode_3d.cl\"");
    if (include_pos != std::string::npos) {
        merge_src.replace(include_pos, 29, "// #include \"hilbert_encode_3d.cl\"");
    }
    
    std::string full_src = hilbert_src + "\n" + merge_src;
    
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
        throw std::runtime_error("Failed to build MergeEngine program");
    }
    
    m_kernel_mark_siblings = clCreateKernel(m_program, "mark_sibling_groups", &err);
    if (err != CL_SUCCESS) throw std::runtime_error("Failed to create mark_sibling_groups kernel");
    
    m_kernel_merge_fields = clCreateKernel(m_program, "merge_fields", &err);
    if (err != CL_SUCCESS) throw std::runtime_error("Failed to create merge_fields kernel");
    
    m_kernel_create_parents = clCreateKernel(m_program, "create_parent_cells", &err);
    if (err != CL_SUCCESS) throw std::runtime_error("Failed to create create_parent_cells kernel");
}

void MergeEngine::buildHashTable(cl_mem x, cl_mem y, cl_mem z, size_t num_cells) {
    // Host-side hash table build for now
    // 1. Read coordinates
    std::vector<int> h_x(num_cells), h_y(num_cells), h_z(num_cells);
    clEnqueueReadBuffer(m_queue, x, CL_TRUE, 0, num_cells * sizeof(int), h_x.data(), 0, nullptr, nullptr);
    clEnqueueReadBuffer(m_queue, y, CL_TRUE, 0, num_cells * sizeof(int), h_y.data(), 0, nullptr, nullptr);
    clEnqueueReadBuffer(m_queue, z, CL_TRUE, 0, num_cells * sizeof(int), h_z.data(), 0, nullptr, nullptr);
    
    // 2. Compute Hilbert indices and fill hash table
    // Size should be power of 2 and > num_cells to reduce collisions
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
            FL_LOG(ERROR) << "Hash table full in MergeEngine!";
        }
    }
    
    // 3. Upload to GPU
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

MergeResult MergeEngine::merge(
    cl_mem child_x, cl_mem child_y, cl_mem child_z,
    cl_mem child_level, cl_mem child_states,
    cl_mem refine_flags,
    cl_mem child_material_id,
    size_t num_children,
    cl_mem child_fields,
    uint32_t num_field_components
) {
    MergeResult result;
    cl_int err;
    
    if (num_children == 0) return result;
    
    // 1. Build hash table
    buildHashTable(child_x, child_y, child_z, num_children);
    
    // 2. Allocate temporary buffers
    cl_mem merge_group_id = clCreateBuffer(m_context, CL_MEM_READ_WRITE, num_children * sizeof(uint32_t), nullptr, &err);
    cl_mem group_counter = clCreateBuffer(m_context, CL_MEM_READ_WRITE, sizeof(uint32_t), nullptr, &err);
    
    // Initialize counter to 0
    uint32_t zero = 0;
    clEnqueueWriteBuffer(m_queue, group_counter, CL_TRUE, 0, sizeof(uint32_t), &zero, 0, nullptr, nullptr);
    
    // 3. Run mark siblings kernel
    clSetKernelArg(m_kernel_mark_siblings, 0, sizeof(cl_mem), &child_x);
    clSetKernelArg(m_kernel_mark_siblings, 1, sizeof(cl_mem), &child_y);
    clSetKernelArg(m_kernel_mark_siblings, 2, sizeof(cl_mem), &child_z);
    clSetKernelArg(m_kernel_mark_siblings, 3, sizeof(cl_mem), &child_level);
    clSetKernelArg(m_kernel_mark_siblings, 4, sizeof(cl_mem), &refine_flags);
    clSetKernelArg(m_kernel_mark_siblings, 5, sizeof(cl_mem), &child_states);
    clSetKernelArg(m_kernel_mark_siblings, 6, sizeof(cl_mem), &merge_group_id);
    clSetKernelArg(m_kernel_mark_siblings, 7, sizeof(cl_mem), &group_counter);
    clSetKernelArg(m_kernel_mark_siblings, 8, sizeof(cl_mem), nullptr); // cell_hilbert (not used if we compute on fly or pass nullptr)
    // Wait, kernel signature has cell_hilbert. If we don't have it precomputed, we can pass nullptr if kernel handles it?
    // The kernel computes sibling_hilbert but doesn't seem to use cell_hilbert input except maybe for optimization?
    // Checking kernel: "const ulong* restrict cell_hilbert". It is NOT used in the body shown in previous view_file.
    // It computes sibling_hilbert using hilbert_encode_3d.
    // So we can pass nullptr or a dummy buffer.
    // Actually, passing nullptr for __global pointer might be unsafe if kernel tries to access it.
    // But the kernel code I saw earlier didn't use it.
    // I'll pass a dummy buffer just in case or nullptr if I'm sure.
    // To be safe, I'll create a dummy buffer if needed, but clSetKernelArg with nullptr for __global memory usually means NULL pointer in kernel.
    // If kernel doesn't dereference it, it's fine.
    
    clSetKernelArg(m_kernel_mark_siblings, 9, sizeof(cl_mem), &m_hash_table);
    cl_uint table_size_uint = static_cast<cl_uint>(m_hash_table_size);
    clSetKernelArg(m_kernel_mark_siblings, 10, sizeof(cl_uint), &table_size_uint);
    cl_uint num_children_uint = static_cast<cl_uint>(num_children);
    clSetKernelArg(m_kernel_mark_siblings, 11, sizeof(cl_uint), &num_children_uint);
    
    size_t global_work_size = ((num_children + 255) / 256) * 256;
    size_t local_work_size = 256;
    
    err = clEnqueueNDRangeKernel(m_queue, m_kernel_mark_siblings, 1, nullptr, &global_work_size, &local_work_size, 0, nullptr, nullptr);
    if (err != CL_SUCCESS) throw std::runtime_error("Failed to enqueue mark siblings kernel");
    
    // 4. Read back group counter
    uint32_t num_groups = 0;
    clEnqueueReadBuffer(m_queue, group_counter, CL_TRUE, 0, sizeof(uint32_t), &num_groups, 0, nullptr, nullptr);
    
    if (num_groups == 0) {
        clReleaseMemObject(merge_group_id);
        clReleaseMemObject(group_counter);
        return result;
    }
    
    // 5. Create group_to_parent map
    // We need to map group_id -> parent_idx (0 to num_groups-1)
    // Since group IDs are allocated atomically from 0 to num_groups-1, the mapping is identity: group_id IS the parent_idx.
    // So group_to_parent[i] = i.
    // We create a buffer for this.
    
    std::vector<uint32_t> h_group_to_parent(num_groups);
    for (uint32_t i = 0; i < num_groups; ++i) h_group_to_parent[i] = i;
    
    cl_mem group_to_parent = clCreateBuffer(m_context, CL_MEM_READ_WRITE, num_groups * sizeof(uint32_t), nullptr, &err);
    clEnqueueWriteBuffer(m_queue, group_to_parent, CL_TRUE, 0, num_groups * sizeof(uint32_t), h_group_to_parent.data(), 0, nullptr, nullptr);
    
    // 6. Create parent buffers
    cl_mem parent_x = clCreateBuffer(m_context, CL_MEM_READ_WRITE, num_groups * sizeof(int), nullptr, &err);
    cl_mem parent_y = clCreateBuffer(m_context, CL_MEM_READ_WRITE, num_groups * sizeof(int), nullptr, &err);
    cl_mem parent_z = clCreateBuffer(m_context, CL_MEM_READ_WRITE, num_groups * sizeof(int), nullptr, &err);
    cl_mem parent_level = clCreateBuffer(m_context, CL_MEM_READ_WRITE, num_groups * sizeof(uint8_t), nullptr, &err);
    cl_mem parent_states = clCreateBuffer(m_context, CL_MEM_READ_WRITE, num_groups * sizeof(uint8_t), nullptr, &err);
    cl_mem parent_mat_id = clCreateBuffer(m_context, CL_MEM_READ_WRITE, num_groups * sizeof(uint32_t), nullptr, &err);
    
    // 7. Run create parents kernel
    clSetKernelArg(m_kernel_create_parents, 0, sizeof(cl_mem), &child_x);
    clSetKernelArg(m_kernel_create_parents, 1, sizeof(cl_mem), &child_y);
    clSetKernelArg(m_kernel_create_parents, 2, sizeof(cl_mem), &child_z);
    clSetKernelArg(m_kernel_create_parents, 3, sizeof(cl_mem), &child_level);
    clSetKernelArg(m_kernel_create_parents, 4, sizeof(cl_mem), &child_states);
    clSetKernelArg(m_kernel_create_parents, 5, sizeof(cl_mem), &child_material_id);
    clSetKernelArg(m_kernel_create_parents, 6, sizeof(cl_mem), &merge_group_id);
    clSetKernelArg(m_kernel_create_parents, 7, sizeof(cl_mem), &group_to_parent);
    clSetKernelArg(m_kernel_create_parents, 8, sizeof(cl_mem), &parent_x);
    clSetKernelArg(m_kernel_create_parents, 9, sizeof(cl_mem), &parent_y);
    clSetKernelArg(m_kernel_create_parents, 10, sizeof(cl_mem), &parent_z);
    clSetKernelArg(m_kernel_create_parents, 11, sizeof(cl_mem), &parent_level);
    clSetKernelArg(m_kernel_create_parents, 12, sizeof(cl_mem), &parent_states);
    clSetKernelArg(m_kernel_create_parents, 13, sizeof(cl_mem), &parent_mat_id);
    clSetKernelArg(m_kernel_create_parents, 14, sizeof(cl_uint), &num_children_uint);
    
    err = clEnqueueNDRangeKernel(m_queue, m_kernel_create_parents, 1, nullptr, &global_work_size, &local_work_size, 0, nullptr, nullptr);
    if (err != CL_SUCCESS) throw std::runtime_error("Failed to enqueue create parents kernel");
    
    // 8. Merge fields if provided
    if (child_fields && num_field_components > 0) {
        cl_mem parent_fields = clCreateBuffer(m_context, CL_MEM_READ_WRITE, num_groups * num_field_components * sizeof(float), nullptr, &err);
        
        // Initialize parent fields to 0
        float zero_f = 0.0f;
        clEnqueueFillBuffer(m_queue, parent_fields, &zero_f, sizeof(float), 0, num_groups * num_field_components * sizeof(float), 0, nullptr, nullptr);
        
        clSetKernelArg(m_kernel_merge_fields, 0, sizeof(cl_mem), &merge_group_id);
        clSetKernelArg(m_kernel_merge_fields, 1, sizeof(cl_mem), &group_to_parent);
        clSetKernelArg(m_kernel_merge_fields, 2, sizeof(cl_mem), &child_fields);
        clSetKernelArg(m_kernel_merge_fields, 3, sizeof(cl_mem), &parent_fields);
        clSetKernelArg(m_kernel_merge_fields, 4, sizeof(cl_uint), &num_field_components);
        
        // Averaging rule: 0=arithmetic, 1=volume_weighted. Config should specify.
        // Default to arithmetic (0) for now or parse config string.
        uint32_t rule = (m_config.default_averaging_rule == "volume_weighted") ? 1 : 0;
        clSetKernelArg(m_kernel_merge_fields, 5, sizeof(cl_uint), &rule);
        
        clSetKernelArg(m_kernel_merge_fields, 6, sizeof(cl_uint), &num_children_uint);
        
        err = clEnqueueNDRangeKernel(m_queue, m_kernel_merge_fields, 1, nullptr, &global_work_size, &local_work_size, 0, nullptr, nullptr);
        if (err != CL_SUCCESS) throw std::runtime_error("Failed to enqueue merge fields kernel");
        
        // Read back averaged fields
        result.averaged_fields.resize(num_groups * num_field_components);
        clEnqueueReadBuffer(m_queue, parent_fields, CL_TRUE, 0, result.averaged_fields.size() * sizeof(float), result.averaged_fields.data(), 0, nullptr, nullptr);
        
        clReleaseMemObject(parent_fields);
    }
    
    // 9. Read back results
    std::vector<int> h_parent_x(num_groups);
    std::vector<int> h_parent_y(num_groups);
    std::vector<int> h_parent_z(num_groups);
    std::vector<uint8_t> h_parent_level(num_groups);
    std::vector<uint8_t> h_parent_states(num_groups);
    std::vector<uint32_t> h_parent_mat_id(num_groups);
    
    clEnqueueReadBuffer(m_queue, parent_x, CL_TRUE, 0, num_groups * sizeof(int), h_parent_x.data(), 0, nullptr, nullptr);
    clEnqueueReadBuffer(m_queue, parent_y, CL_TRUE, 0, num_groups * sizeof(int), h_parent_y.data(), 0, nullptr, nullptr);
    clEnqueueReadBuffer(m_queue, parent_z, CL_TRUE, 0, num_groups * sizeof(int), h_parent_z.data(), 0, nullptr, nullptr);
    clEnqueueReadBuffer(m_queue, parent_level, CL_TRUE, 0, num_groups * sizeof(uint8_t), h_parent_level.data(), 0, nullptr, nullptr);
    clEnqueueReadBuffer(m_queue, parent_states, CL_TRUE, 0, num_groups * sizeof(uint8_t), h_parent_states.data(), 0, nullptr, nullptr);
    clEnqueueReadBuffer(m_queue, parent_mat_id, CL_TRUE, 0, num_groups * sizeof(uint32_t), h_parent_mat_id.data(), 0, nullptr, nullptr);
    
    result.parents.resize(num_groups);
    for (size_t i = 0; i < num_groups; ++i) {
        CellDescriptor& cell = result.parents[i];
        cell.x = h_parent_x[i];
        cell.y = h_parent_y[i];
        cell.z = h_parent_z[i];
        cell.level = h_parent_level[i];
        cell.state = h_parent_states[i];
        cell.material_id = h_parent_mat_id[i];
        cell.visited = 0;
        cell.reserved = 0;
    }
    
    // Populate merged_child_indices
    std::vector<uint32_t> h_merge_group_id(num_children);
    clEnqueueReadBuffer(m_queue, merge_group_id, CL_TRUE, 0, num_children * sizeof(uint32_t), h_merge_group_id.data(), 0, nullptr, nullptr);
    
    for (size_t i = 0; i < num_children; ++i) {
        if (h_merge_group_id[i] != INVALID_INDEX) {
            result.merged_child_indices.insert(i);
        }
    }
    
    result.success = true;
    result.num_parents_created = num_groups;
    result.num_children_merged = result.merged_child_indices.size();
    result.group_to_parent_map = h_group_to_parent;
    
    // Cleanup
    clReleaseMemObject(merge_group_id);
    clReleaseMemObject(group_counter);
    clReleaseMemObject(group_to_parent);
    clReleaseMemObject(parent_x);
    clReleaseMemObject(parent_y);
    clReleaseMemObject(parent_z);
    clReleaseMemObject(parent_level);
    clReleaseMemObject(parent_states);
    clReleaseMemObject(parent_mat_id);
    
    return result;
}

} // namespace adaptation
} // namespace fluidloom
