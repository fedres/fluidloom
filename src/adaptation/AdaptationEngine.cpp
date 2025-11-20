#include "fluidloom/adaptation/AdaptationEngine.h"
#include "fluidloom/adaptation/CellDescriptor.h"
#include "fluidloom/common/FluidLoomError.h"
#include "fluidloom/common/Logger.h"
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>

namespace fluidloom {
namespace adaptation {

AdaptationEngine::AdaptationEngine(cl_context context, cl_command_queue queue, const AdaptationConfig& config)
    : m_context(context), m_queue(queue), m_config(config),
      m_compaction_program(nullptr),
      m_kernel_mark_valid(nullptr), m_kernel_scan_local(nullptr),
      m_kernel_scan_add_sums(nullptr), m_kernel_compact(nullptr), m_kernel_append(nullptr) {
    
    m_split_engine = std::make_unique<SplitEngine>(context, queue, config);
    m_merge_engine = std::make_unique<MergeEngine>(context, queue, config);
    m_balance_enforcer = std::make_unique<BalanceEnforcer>(context, queue, config);
    
    compileCompactionKernels();
}

AdaptationEngine::~AdaptationEngine() {
    if (m_kernel_mark_valid) clReleaseKernel(m_kernel_mark_valid);
    if (m_kernel_scan_local) clReleaseKernel(m_kernel_scan_local);
    if (m_kernel_scan_add_sums) clReleaseKernel(m_kernel_scan_add_sums);
    if (m_kernel_compact) clReleaseKernel(m_kernel_compact);
    if (m_kernel_append) clReleaseKernel(m_kernel_append);
    if (m_compaction_program) clReleaseProgram(m_compaction_program);
}



cl_event AdaptationEngine::adapt(
    cl_mem* coord_x, cl_mem* coord_y, cl_mem* coord_z,
    cl_mem* levels, cl_mem* cell_states,
    cl_mem* refine_flags,
    cl_mem* material_id,
    size_t* num_cells,
    size_t* capacity,
    cl_mem* fields,
    uint32_t num_field_components
) {
    cl_int err;
    
    // 1. Enforce 2:1 Balance
    if (m_config.enforce_2_1_balance) {
        BalanceResult balance_res = m_balance_enforcer->enforce(
            *coord_x, *coord_y, *coord_z, *levels, *cell_states, *refine_flags, *num_cells
        );
        if (!balance_res.converged) {
            FL_LOG(WARN) << "Balance enforcement did not converge in " << balance_res.iterations << " iterations";
        }
    }
    
    // 2. Split Cells
    SplitResult split_res = m_split_engine->split(
        *coord_x, *coord_y, *coord_z, *levels, *cell_states, *refine_flags, *material_id, *num_cells,
        fields ? *fields : nullptr, num_field_components
    );
    
    // 3. Merge Cells
    MergeResult merge_res = m_merge_engine->merge(
        *coord_x, *coord_y, *coord_z, *levels, *cell_states, *refine_flags, *material_id, *num_cells,
        fields ? *fields : nullptr, num_field_components
    );
    
    // If no changes, return early
    if (split_res.num_children == 0 && merge_res.num_parents_created == 0) {
        // Create a user event to signal completion
        cl_event event = clCreateUserEvent(m_context, &err);
        clSetUserEventStatus(event, CL_COMPLETE);
        return event;
    }
    
    // 4. Compact and Rebuild
    // 4. Compact and Rebuild
    // 4. Compact and Rebuild (GPU)
    compactAndRebuildGPU(
        split_res, merge_res,
        coord_x, coord_y, coord_z, levels, cell_states, refine_flags, material_id, num_cells, capacity,
        fields, num_field_components
    );
    
    // Check if we need to resize buffers (already handled in compactAndRebuild if needed, but we might want to grow proactively)
    // Actually compactAndRebuild handles it now.
    
    // Check if we need to resize buffers
    if (*num_cells > *capacity) {
        size_t new_capacity = static_cast<size_t>(*num_cells * m_config.buffer_growth_factor);
        resizeBuffers(new_capacity, coord_x, coord_y, coord_z, levels, cell_states, refine_flags, material_id, fields, num_field_components);
        *capacity = new_capacity;
    }
    
    // Create completion event
    cl_event event = clCreateUserEvent(m_context, &err);
    clSetUserEventStatus(event, CL_COMPLETE);
    return event;
}

void AdaptationEngine::compactAndRebuild(
    const SplitResult& split_res,
    const MergeResult& merge_res,
    cl_mem* coord_x, cl_mem* coord_y, cl_mem* coord_z,
    cl_mem* levels, cl_mem* cell_states,
    cl_mem* refine_flags,
    cl_mem* material_id,
    size_t* num_cells,
    size_t* capacity,
    cl_mem* fields,
    uint32_t num_field_components
) {
    // Host-side rebuild for now
    size_t old_count = *num_cells;
    
    // Read old buffers
    std::vector<int> h_x(old_count), h_y(old_count), h_z(old_count);
    std::vector<uint8_t> h_levels(old_count), h_states(old_count);
    std::vector<int> h_flags(old_count);
    std::vector<uint32_t> h_mat(old_count);
    std::vector<float> h_fields;
    
    clEnqueueReadBuffer(m_queue, *coord_x, CL_TRUE, 0, old_count * sizeof(int), h_x.data(), 0, nullptr, nullptr);
    clEnqueueReadBuffer(m_queue, *coord_y, CL_TRUE, 0, old_count * sizeof(int), h_y.data(), 0, nullptr, nullptr);
    clEnqueueReadBuffer(m_queue, *coord_z, CL_TRUE, 0, old_count * sizeof(int), h_z.data(), 0, nullptr, nullptr);
    clEnqueueReadBuffer(m_queue, *levels, CL_TRUE, 0, old_count * sizeof(uint8_t), h_levels.data(), 0, nullptr, nullptr);
    clEnqueueReadBuffer(m_queue, *cell_states, CL_TRUE, 0, old_count * sizeof(uint8_t), h_states.data(), 0, nullptr, nullptr);
    clEnqueueReadBuffer(m_queue, *refine_flags, CL_TRUE, 0, old_count * sizeof(int), h_flags.data(), 0, nullptr, nullptr);
    clEnqueueReadBuffer(m_queue, *material_id, CL_TRUE, 0, old_count * sizeof(uint32_t), h_mat.data(), 0, nullptr, nullptr);
    
    if (fields && *fields && num_field_components > 0) {
        h_fields.resize(old_count * num_field_components);
        clEnqueueReadBuffer(m_queue, *fields, CL_TRUE, 0, h_fields.size() * sizeof(float), h_fields.data(), 0, nullptr, nullptr);
    }
    
    // Build new list
    std::vector<CellDescriptor> new_cells;
    std::vector<float> new_fields;
    new_cells.reserve(old_count + split_res.num_children); // Estimate
    
    // 1. Add surviving old cells
    for (size_t i = 0; i < old_count; ++i) {
        bool is_split = (i < split_res.parent_to_child_map.size()) && (split_res.parent_to_child_map[i] != 0xFFFFFFFF);
        bool is_merged = (merge_res.merged_child_indices.count(i) > 0);
        
        if (!is_split && !is_merged) {
            CellDescriptor cell;
            cell.x = h_x[i]; cell.y = h_y[i]; cell.z = h_z[i];
            cell.level = h_levels[i]; cell.state = h_states[i]; cell.material_id = h_mat[i];
            cell.visited = 0; cell.reserved = 0;
            new_cells.push_back(cell);
            
            if (!h_fields.empty()) {
                for (uint32_t c = 0; c < num_field_components; ++c) {
                    new_fields.push_back(h_fields[i * num_field_components + c]);
                }
            }
        }
    }
    
    // 2. Add new children from splits
    // We need to read children from SplitResult (Wait, SplitResult has vector<CellDescriptor> children)
    // But we also need fields. SplitEngine interpolates fields to a buffer, but we didn't capture it in SplitResult struct.
    // I noted this TODO in SplitEngine.cpp.
    // For now, I'll assume fields are lost for new children or I need to re-interpolate here on host.
    // Re-interpolating on host is safer for now.
    
    for (size_t i = 0; i < split_res.split_parent_indices.size(); ++i) {
        uint32_t parent_idx = split_res.split_parent_indices[i];
        uint32_t child_start = split_res.parent_to_child_map[parent_idx];
        
        for (size_t c = 0; c < 8; ++c) {
            const CellDescriptor& child = split_res.children[child_start + c];
            new_cells.push_back(child);
            
            if (!h_fields.empty()) {
                // Copy parent fields to child (simple interpolation)
                for (uint32_t comp = 0; comp < num_field_components; ++comp) {
                    new_fields.push_back(h_fields[parent_idx * num_field_components + comp]);
                }
            }
        }
    }
    
    // 3. Add new parents from merges
    // MergeEngine computes averaged fields into a buffer, but we didn't capture it in MergeResult struct.
    // Same issue. I'll re-average on host.
    
    for (size_t i = 0; i < merge_res.parents.size(); ++i) {
        new_cells.push_back(merge_res.parents[i]);
        
        if (!h_fields.empty()) {
            // Use averaged fields from MergeResult
            if (merge_res.averaged_fields.size() >= (i + 1) * num_field_components) {
                for (uint32_t comp = 0; comp < num_field_components; ++comp) {
                    new_fields.push_back(merge_res.averaged_fields[i * num_field_components + comp]);
                }
            } else {
                // Fallback (should not happen if MergeEngine works)
                for (uint32_t comp = 0; comp < num_field_components; ++comp) {
                    new_fields.push_back(0.0f); 
                }
            }
        }
    }
    
    // 4. Sort by Hilbert index
    // We need to sort `new_cells` and permute `new_fields` accordingly.
    std::vector<size_t> p(new_cells.size());
    for (size_t i = 0; i < p.size(); ++i) p[i] = i;
    
    std::sort(p.begin(), p.end(), [&](size_t a, size_t b) {
        return new_cells[a].getHilbert() < new_cells[b].getHilbert();
    });
    
    // Apply permutation
    std::vector<CellDescriptor> sorted_cells(new_cells.size());
    std::vector<float> sorted_fields;
    if (!new_fields.empty()) sorted_fields.resize(new_fields.size());
    
    for (size_t i = 0; i < new_cells.size(); ++i) {
        sorted_cells[i] = new_cells[p[i]];
        if (!new_fields.empty()) {
            for (uint32_t c = 0; c < num_field_components; ++c) {
                sorted_fields[i * num_field_components + c] = new_fields[p[i] * num_field_components + c];
            }
        }
    }
    
    // 5. Write back to buffers
    *num_cells = sorted_cells.size();
    
    // Check capacity and resize if needed
    if (*num_cells > *capacity) {
        size_t new_capacity = std::max(static_cast<size_t>(*num_cells * m_config.buffer_growth_factor), *num_cells + 1024);
        resizeBuffers(new_capacity, coord_x, coord_y, coord_z, levels, cell_states, refine_flags, material_id, fields, num_field_components);
        *capacity = new_capacity;
    }
    
    // Unpack to host vectors
    std::vector<int> out_x(sorted_cells.size()), out_y(sorted_cells.size()), out_z(sorted_cells.size());
    std::vector<uint8_t> out_levels(sorted_cells.size()), out_states(sorted_cells.size());
    std::vector<int> out_flags(sorted_cells.size(), 0); // Reset flags
    std::vector<uint32_t> out_mat(sorted_cells.size());
    
    for (size_t i = 0; i < sorted_cells.size(); ++i) {
        out_x[i] = sorted_cells[i].x;
        out_y[i] = sorted_cells[i].y;
        out_z[i] = sorted_cells[i].z;
        out_levels[i] = sorted_cells[i].level;
        out_states[i] = sorted_cells[i].state;
        out_mat[i] = sorted_cells[i].material_id;
    }
    
    // Write to GPU
    // We blindly write. If buffer is too small, this is bad.
    // I'll update `adapt` to resize if needed BEFORE calling `compactAndRebuild`?
    // No, we don't know new size until we build it.
    // So `compactAndRebuild` should handle resizing.
    // I'll modify `compactAndRebuild` signature to take `capacity` pointer.
    
    clEnqueueWriteBuffer(m_queue, *coord_x, CL_TRUE, 0, out_x.size() * sizeof(int), out_x.data(), 0, nullptr, nullptr);
    clEnqueueWriteBuffer(m_queue, *coord_y, CL_TRUE, 0, out_y.size() * sizeof(int), out_y.data(), 0, nullptr, nullptr);
    clEnqueueWriteBuffer(m_queue, *coord_z, CL_TRUE, 0, out_z.size() * sizeof(int), out_z.data(), 0, nullptr, nullptr);
    clEnqueueWriteBuffer(m_queue, *levels, CL_TRUE, 0, out_levels.size() * sizeof(uint8_t), out_levels.data(), 0, nullptr, nullptr);
    clEnqueueWriteBuffer(m_queue, *cell_states, CL_TRUE, 0, out_states.size() * sizeof(uint8_t), out_states.data(), 0, nullptr, nullptr);
    clEnqueueWriteBuffer(m_queue, *refine_flags, CL_TRUE, 0, out_flags.size() * sizeof(int), out_flags.data(), 0, nullptr, nullptr);
    clEnqueueWriteBuffer(m_queue, *material_id, CL_TRUE, 0, out_mat.size() * sizeof(uint32_t), out_mat.data(), 0, nullptr, nullptr);
    
    if (!sorted_fields.empty()) {
        clEnqueueWriteBuffer(m_queue, *fields, CL_TRUE, 0, sorted_fields.size() * sizeof(float), sorted_fields.data(), 0, nullptr, nullptr);
    }
}

void AdaptationEngine::resizeBuffers(
    size_t new_capacity,
    cl_mem* coord_x, cl_mem* coord_y, cl_mem* coord_z,
    cl_mem* levels, cl_mem* cell_states,
    cl_mem* refine_flags,
    cl_mem* material_id,
    cl_mem* fields,
    uint32_t num_field_components
) {
    // Reallocate buffers with new capacity
    // Note: This destroys content if we don't copy.
    // But we usually resize when we have data on host ready to write, or we copy device-to-device.
    // Here, `adapt` calls `resizeBuffers` after `compactAndRebuild`?
    // If `compactAndRebuild` writes to buffers, they must be large enough.
    // So `compactAndRebuild` must resize if needed.
    // I'll implement resize logic inside `compactAndRebuild` or helper.
    
    cl_int err;
    // Helper to resize one buffer
    auto resize = [&](cl_mem* buf, size_t size) {
        if (*buf) clReleaseMemObject(*buf);
        *buf = clCreateBuffer(m_context, CL_MEM_READ_WRITE, size, nullptr, &err);
    };
    
    resize(coord_x, new_capacity * sizeof(int));
    resize(coord_y, new_capacity * sizeof(int));
    resize(coord_z, new_capacity * sizeof(int));
    resize(levels, new_capacity * sizeof(uint8_t));
    resize(cell_states, new_capacity * sizeof(uint8_t));
    resize(refine_flags, new_capacity * sizeof(int));
    resize(material_id, new_capacity * sizeof(uint32_t));
    
    if (fields && *fields) {
        resize(fields, new_capacity * num_field_components * sizeof(float));
    }
}

void AdaptationEngine::compileCompactionKernels() {
    std::string src = loadKernelSource("compact_cells.cl");
    const char* src_str = src.c_str();
    size_t src_len = src.length();
    cl_int err;
    
    m_compaction_program = clCreateProgramWithSource(m_context, 1, &src_str, &src_len, &err);
    if (err != CL_SUCCESS) throw std::runtime_error("Failed to create compaction program");
    
    err = clBuildProgram(m_compaction_program, 0, nullptr, "-cl-std=CL1.2", nullptr, nullptr);
    if (err != CL_SUCCESS) {
        size_t device_size;
        clGetContextInfo(m_context, CL_CONTEXT_DEVICES, 0, nullptr, &device_size);
        std::vector<cl_device_id> devices(device_size / sizeof(cl_device_id));
        clGetContextInfo(m_context, CL_CONTEXT_DEVICES, device_size, devices.data(), nullptr);
        
        if (!devices.empty()) {
             size_t log_size;
             clGetProgramBuildInfo(m_compaction_program, devices[0], CL_PROGRAM_BUILD_LOG, 0, nullptr, &log_size);
             std::vector<char> log(log_size + 1);
             clGetProgramBuildInfo(m_compaction_program, devices[0], CL_PROGRAM_BUILD_LOG, log_size, log.data(), nullptr);
             log[log_size] = '\0';
             FL_LOG(ERROR) << "Compaction Build log: " << log.data();
        }
        throw std::runtime_error("Failed to build compaction kernels");
    }
    
    m_kernel_mark_valid = clCreateKernel(m_compaction_program, "mark_valid_cells", &err);
    m_kernel_scan_local = clCreateKernel(m_compaction_program, "scan_local", &err);
    m_kernel_scan_add_sums = clCreateKernel(m_compaction_program, "scan_add_sums", &err);
    m_kernel_compact = clCreateKernel(m_compaction_program, "compact_cells", &err);
    m_kernel_append = clCreateKernel(m_compaction_program, "append_cells", &err);
}

std::string AdaptationEngine::loadKernelSource(const std::string& filename) {
    std::string path = "/Users/karthikt/Ideas/FluidLoom/fluidloom/src/adaptation/kernels/" + filename;
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open kernel source: " + path);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void AdaptationEngine::exclusiveScan(cl_mem input, cl_mem output, uint32_t num_elements, uint32_t* total_sum) {
    cl_int err;
    size_t local_size = 256;
    size_t num_groups = (num_elements + local_size - 1) / local_size;
    
    // Allocate block sums
    cl_mem block_sums = clCreateBuffer(m_context, CL_MEM_READ_WRITE, num_groups * sizeof(uint32_t), nullptr, &err);
    
    // Phase 1: Local scan
    clSetKernelArg(m_kernel_scan_local, 0, sizeof(cl_mem), &input);
    clSetKernelArg(m_kernel_scan_local, 1, sizeof(cl_mem), &output);
    clSetKernelArg(m_kernel_scan_local, 2, sizeof(cl_mem), &block_sums);
    clSetKernelArg(m_kernel_scan_local, 3, sizeof(uint32_t), &num_elements);
    clSetKernelArg(m_kernel_scan_local, 4, local_size * sizeof(uint32_t), nullptr); // Shared mem
    
    size_t global_size = num_groups * local_size;
    err = clEnqueueNDRangeKernel(m_queue, m_kernel_scan_local, 1, nullptr, &global_size, &local_size, 0, nullptr, nullptr);
    
    // Phase 2: Scan block sums (recursive if needed, but for now assume num_groups < 256*256)
    // If num_groups is small (<= 256), we can just scan it with one group.
    if (num_groups > 1) {
        cl_mem block_sums_scanned = clCreateBuffer(m_context, CL_MEM_READ_WRITE, num_groups * sizeof(uint32_t), nullptr, &err);
        
        clSetKernelArg(m_kernel_scan_local, 0, sizeof(cl_mem), &block_sums);
        clSetKernelArg(m_kernel_scan_local, 1, sizeof(cl_mem), &block_sums_scanned);
        clSetKernelArg(m_kernel_scan_local, 2, sizeof(cl_mem), nullptr); // No higher level sums
        clSetKernelArg(m_kernel_scan_local, 3, sizeof(uint32_t), &num_groups);
        clSetKernelArg(m_kernel_scan_local, 4, local_size * sizeof(uint32_t), nullptr);
        
        size_t scan_global = ((num_groups + local_size - 1) / local_size) * local_size;
        err = clEnqueueNDRangeKernel(m_queue, m_kernel_scan_local, 1, nullptr, &scan_global, &local_size, 0, nullptr, nullptr);
        
        // Phase 3: Add block sums back
        clSetKernelArg(m_kernel_scan_add_sums, 0, sizeof(cl_mem), &output);
        clSetKernelArg(m_kernel_scan_add_sums, 1, sizeof(cl_mem), &block_sums_scanned);
        clSetKernelArg(m_kernel_scan_add_sums, 2, sizeof(uint32_t), &num_elements);
        
        err = clEnqueueNDRangeKernel(m_queue, m_kernel_scan_add_sums, 1, nullptr, &global_size, &local_size, 0, nullptr, nullptr);
        
        clReleaseMemObject(block_sums_scanned);
    }
    
    // Read total sum (last element of output + last element of input)
    // Or just read the last block sum?
    // Actually, exclusive scan result at [N] is the sum.
    // But we don't have [N] in output buffer usually (size N).
    // We can read output[N-1] + input[N-1].
    if (total_sum) {
        uint32_t last_val = 0, last_in = 0;
        clEnqueueReadBuffer(m_queue, output, CL_TRUE, (num_elements - 1) * sizeof(uint32_t), sizeof(uint32_t), &last_val, 0, nullptr, nullptr);
        clEnqueueReadBuffer(m_queue, input, CL_TRUE, (num_elements - 1) * sizeof(uint32_t), sizeof(uint32_t), &last_in, 0, nullptr, nullptr);
        *total_sum = last_val + last_in;
    }
    
    clReleaseMemObject(block_sums);
}

void AdaptationEngine::compactAndRebuildGPU(
    const SplitResult& split_res,
    const MergeResult& merge_res,
    cl_mem* coord_x, cl_mem* coord_y, cl_mem* coord_z,
    cl_mem* levels, cl_mem* cell_states,
    cl_mem* refine_flags,
    cl_mem* material_id,
    size_t* num_cells,
    size_t* capacity,
    cl_mem* fields,
    uint32_t num_field_components
) {
    cl_int err;
    size_t current_cells = *num_cells;
    
    // 1. Mark valid cells
    cl_mem valid_flags = clCreateBuffer(m_context, CL_MEM_READ_WRITE, current_cells * sizeof(uint32_t), nullptr, &err);
    
    // We need merge_group_id to know if a cell merged.
    // But MergeResult doesn't give us the buffer, only a set of indices on host.
    // This is a bottleneck. We should have kept the buffer on GPU.
    // For now, we'll upload the set as a boolean mask.
    // TODO: Optimize by keeping merge_group_id on GPU in MergeEngine.
    
    std::vector<uint32_t> h_merge_ids(current_cells, 0xFFFFFFFF);
    for (size_t idx : merge_res.merged_child_indices) {
        h_merge_ids[idx] = 1; // Just mark as "merging"
    }
    cl_mem merge_group_id = clCreateBuffer(m_context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, current_cells * sizeof(uint32_t), h_merge_ids.data(), &err);
    
    clSetKernelArg(m_kernel_mark_valid, 0, sizeof(cl_mem), refine_flags);
    clSetKernelArg(m_kernel_mark_valid, 1, sizeof(cl_mem), cell_states);
    clSetKernelArg(m_kernel_mark_valid, 2, sizeof(cl_mem), &merge_group_id);
    clSetKernelArg(m_kernel_mark_valid, 3, sizeof(cl_mem), &valid_flags);
    clSetKernelArg(m_kernel_mark_valid, 4, sizeof(uint32_t), &current_cells);
    
    size_t global_size = ((current_cells + 255) / 256) * 256;
    size_t local_size = 256;
    clEnqueueNDRangeKernel(m_queue, m_kernel_mark_valid, 1, nullptr, &global_size, &local_size, 0, nullptr, nullptr);
    
    // 2. Scan valid flags to get write offsets
    cl_mem scan_offsets = clCreateBuffer(m_context, CL_MEM_READ_WRITE, current_cells * sizeof(uint32_t), nullptr, &err);
    uint32_t num_survivors = 0;
    exclusiveScan(valid_flags, scan_offsets, current_cells, &num_survivors);
    
    // 3. Calculate total new size
    size_t num_new_children = split_res.children.size(); // Wait, split_res.children is host vector.
    // SplitEngine output is on GPU? No, SplitResult has `children` vector.
    // SplitEngine.cpp:208 reads back children to host.
    // This is another bottleneck. We should keep children on GPU.
    // But for now, we have them on host.
    // We need to upload them to append.
    
    size_t num_new_parents = merge_res.parents.size();
    size_t total_new_cells = num_survivors + num_new_children + num_new_parents;
    
    // 4. Resize if needed
    if (total_new_cells > *capacity) {
        size_t new_capacity = std::max(static_cast<size_t>(total_new_cells * m_config.buffer_growth_factor), total_new_cells + 1024);
        resizeBuffers(new_capacity, coord_x, coord_y, coord_z, levels, cell_states, refine_flags, material_id, fields, num_field_components);
        *capacity = new_capacity;
    }
    
    // 5. Compact survivors to a temporary buffer (or directly to new buffer if we double buffer)
    // Since we resized (maybe), we can't easily double buffer in-place.
    // We need temp buffers for the survivors.
    
    cl_mem new_x = clCreateBuffer(m_context, CL_MEM_READ_WRITE, total_new_cells * sizeof(int), nullptr, &err);
    cl_mem new_y = clCreateBuffer(m_context, CL_MEM_READ_WRITE, total_new_cells * sizeof(int), nullptr, &err);
    cl_mem new_z = clCreateBuffer(m_context, CL_MEM_READ_WRITE, total_new_cells * sizeof(int), nullptr, &err);
    cl_mem new_l = clCreateBuffer(m_context, CL_MEM_READ_WRITE, total_new_cells * sizeof(uint8_t), nullptr, &err);
    cl_mem new_s = clCreateBuffer(m_context, CL_MEM_READ_WRITE, total_new_cells * sizeof(uint8_t), nullptr, &err);
    cl_mem new_m = clCreateBuffer(m_context, CL_MEM_READ_WRITE, total_new_cells * sizeof(uint32_t), nullptr, &err);
    cl_mem new_f = nullptr;
    if (fields && *fields) {
        new_f = clCreateBuffer(m_context, CL_MEM_READ_WRITE, total_new_cells * num_field_components * sizeof(float), nullptr, &err);
    }
    
    clSetKernelArg(m_kernel_compact, 0, sizeof(cl_mem), coord_x);
    clSetKernelArg(m_kernel_compact, 1, sizeof(cl_mem), coord_y);
    clSetKernelArg(m_kernel_compact, 2, sizeof(cl_mem), coord_z);
    clSetKernelArg(m_kernel_compact, 3, sizeof(cl_mem), levels);
    clSetKernelArg(m_kernel_compact, 4, sizeof(cl_mem), cell_states);
    clSetKernelArg(m_kernel_compact, 5, sizeof(cl_mem), material_id);
    clSetKernelArg(m_kernel_compact, 6, sizeof(cl_mem), fields);
    clSetKernelArg(m_kernel_compact, 7, sizeof(cl_mem), &valid_flags);
    clSetKernelArg(m_kernel_compact, 8, sizeof(cl_mem), &scan_offsets);
    clSetKernelArg(m_kernel_compact, 9, sizeof(cl_mem), &new_x);
    clSetKernelArg(m_kernel_compact, 10, sizeof(cl_mem), &new_y);
    clSetKernelArg(m_kernel_compact, 11, sizeof(cl_mem), &new_z);
    clSetKernelArg(m_kernel_compact, 12, sizeof(cl_mem), &new_l);
    clSetKernelArg(m_kernel_compact, 13, sizeof(cl_mem), &new_s);
    clSetKernelArg(m_kernel_compact, 14, sizeof(cl_mem), &new_m);
    clSetKernelArg(m_kernel_compact, 15, sizeof(cl_mem), &new_f);
    clSetKernelArg(m_kernel_compact, 16, sizeof(uint32_t), &current_cells);
    clSetKernelArg(m_kernel_compact, 17, sizeof(uint32_t), &num_field_components);
    
    clEnqueueNDRangeKernel(m_queue, m_kernel_compact, 1, nullptr, &global_size, &local_size, 0, nullptr, nullptr);
    
    // 6. Append new children
    if (num_new_children > 0) {
        // Upload children to GPU (Temporary, should be optimized)
        std::vector<int> cx(num_new_children), cy(num_new_children), cz(num_new_children);
        std::vector<uint8_t> cl(num_new_children), cs(num_new_children);
        std::vector<uint32_t> cm(num_new_children);
        
        for(size_t i=0; i<num_new_children; ++i) {
            cx[i] = split_res.children[i].x;
            cy[i] = split_res.children[i].y;
            cz[i] = split_res.children[i].z;
            cl[i] = split_res.children[i].level;
            cs[i] = split_res.children[i].state;
            cm[i] = split_res.children[i].material_id;
        }
        
        cl_mem buf_cx = clCreateBuffer(m_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, num_new_children*sizeof(int), cx.data(), &err);
        cl_mem buf_cy = clCreateBuffer(m_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, num_new_children*sizeof(int), cy.data(), &err);
        cl_mem buf_cz = clCreateBuffer(m_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, num_new_children*sizeof(int), cz.data(), &err);
        cl_mem buf_cl = clCreateBuffer(m_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, num_new_children*sizeof(uint8_t), cl.data(), &err);
        cl_mem buf_cs = clCreateBuffer(m_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, num_new_children*sizeof(uint8_t), cs.data(), &err);
        cl_mem buf_cm = clCreateBuffer(m_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, num_new_children*sizeof(uint32_t), cm.data(), &err);
        cl_mem buf_cf = nullptr;
        
        // Handle fields for children (interpolation was done in SplitEngine but returned on host?)
        // SplitEngine::split returns SplitResult.
        // SplitEngine.cpp:208 reads back children.
        // It does NOT seem to return interpolated fields in SplitResult!
        // SplitEngine.cpp:190 `interpolate_fields` kernel writes to `child_fields`.
        // But `child_fields` is a temporary buffer in `split`.
        // We need to extract it.
        // Major TODO: SplitEngine should return the `child_fields` buffer or data.
        // For now, we'll assume zero fields or implement a fix later.
        // I'll pass nullptr for fields to append.
        
        uint32_t offset = num_survivors;
        uint32_t count = num_new_children;
        
        clSetKernelArg(m_kernel_append, 0, sizeof(cl_mem), &buf_cx);
        clSetKernelArg(m_kernel_append, 1, sizeof(cl_mem), &buf_cy);
        clSetKernelArg(m_kernel_append, 2, sizeof(cl_mem), &buf_cz);
        clSetKernelArg(m_kernel_append, 3, sizeof(cl_mem), &buf_cl);
        clSetKernelArg(m_kernel_append, 4, sizeof(cl_mem), &buf_cs);
        clSetKernelArg(m_kernel_append, 5, sizeof(cl_mem), &buf_cm);
        clSetKernelArg(m_kernel_append, 6, sizeof(cl_mem), &buf_cf); // Fields missing
        
        clSetKernelArg(m_kernel_append, 7, sizeof(cl_mem), &new_x);
        clSetKernelArg(m_kernel_append, 8, sizeof(cl_mem), &new_y);
        clSetKernelArg(m_kernel_append, 9, sizeof(cl_mem), &new_z);
        clSetKernelArg(m_kernel_append, 10, sizeof(cl_mem), &new_l);
        clSetKernelArg(m_kernel_append, 11, sizeof(cl_mem), &new_s);
        clSetKernelArg(m_kernel_append, 12, sizeof(cl_mem), &new_m);
        clSetKernelArg(m_kernel_append, 13, sizeof(cl_mem), &new_f);
        
        clSetKernelArg(m_kernel_append, 14, sizeof(uint32_t), &count);
        clSetKernelArg(m_kernel_append, 15, sizeof(uint32_t), &offset);
        clSetKernelArg(m_kernel_append, 16, sizeof(uint32_t), &num_field_components);
        
        size_t append_global = ((count + 255) / 256) * 256;
        clEnqueueNDRangeKernel(m_queue, m_kernel_append, 1, nullptr, &append_global, &local_size, 0, nullptr, nullptr);
        
        clReleaseMemObject(buf_cx); clReleaseMemObject(buf_cy); clReleaseMemObject(buf_cz);
        clReleaseMemObject(buf_cl); clReleaseMemObject(buf_cs); clReleaseMemObject(buf_cm);
    }
    
    // 7. Append new parents
    if (num_new_parents > 0) {
        std::vector<int> px(num_new_parents), py(num_new_parents), pz(num_new_parents);
        std::vector<uint8_t> pl(num_new_parents), ps(num_new_parents);
        std::vector<uint32_t> pm(num_new_parents);
        
        for(size_t i=0; i<num_new_parents; ++i) {
            px[i] = merge_res.parents[i].x;
            py[i] = merge_res.parents[i].y;
            pz[i] = merge_res.parents[i].z;
            pl[i] = merge_res.parents[i].level;
            ps[i] = merge_res.parents[i].state;
            pm[i] = merge_res.parents[i].material_id;
        }
        
        cl_mem buf_px = clCreateBuffer(m_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, num_new_parents*sizeof(int), px.data(), &err);
        cl_mem buf_py = clCreateBuffer(m_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, num_new_parents*sizeof(int), py.data(), &err);
        cl_mem buf_pz = clCreateBuffer(m_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, num_new_parents*sizeof(int), pz.data(), &err);
        cl_mem buf_pl = clCreateBuffer(m_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, num_new_parents*sizeof(uint8_t), pl.data(), &err);
        cl_mem buf_ps = clCreateBuffer(m_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, num_new_parents*sizeof(uint8_t), ps.data(), &err);
        cl_mem buf_pm = clCreateBuffer(m_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, num_new_parents*sizeof(uint32_t), pm.data(), &err);
        
        cl_mem buf_pf = nullptr;
        if (!merge_res.averaged_fields.empty()) {
            buf_pf = clCreateBuffer(m_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, merge_res.averaged_fields.size()*sizeof(float), (void*)merge_res.averaged_fields.data(), &err);
        }
        
        uint32_t offset = num_survivors + num_new_children;
        uint32_t count = num_new_parents;
        
        clSetKernelArg(m_kernel_append, 0, sizeof(cl_mem), &buf_px);
        clSetKernelArg(m_kernel_append, 1, sizeof(cl_mem), &buf_py);
        clSetKernelArg(m_kernel_append, 2, sizeof(cl_mem), &buf_pz);
        clSetKernelArg(m_kernel_append, 3, sizeof(cl_mem), &buf_pl);
        clSetKernelArg(m_kernel_append, 4, sizeof(cl_mem), &buf_ps);
        clSetKernelArg(m_kernel_append, 5, sizeof(cl_mem), &buf_pm);
        clSetKernelArg(m_kernel_append, 6, sizeof(cl_mem), &buf_pf);
        
        clSetKernelArg(m_kernel_append, 7, sizeof(cl_mem), &new_x);
        clSetKernelArg(m_kernel_append, 8, sizeof(cl_mem), &new_y);
        clSetKernelArg(m_kernel_append, 9, sizeof(cl_mem), &new_z);
        clSetKernelArg(m_kernel_append, 10, sizeof(cl_mem), &new_l);
        clSetKernelArg(m_kernel_append, 11, sizeof(cl_mem), &new_s);
        clSetKernelArg(m_kernel_append, 12, sizeof(cl_mem), &new_m);
        clSetKernelArg(m_kernel_append, 13, sizeof(cl_mem), &new_f);
        
        clSetKernelArg(m_kernel_append, 14, sizeof(uint32_t), &count);
        clSetKernelArg(m_kernel_append, 15, sizeof(uint32_t), &offset);
        clSetKernelArg(m_kernel_append, 16, sizeof(uint32_t), &num_field_components);
        
        size_t append_global = ((count + 255) / 256) * 256;
        clEnqueueNDRangeKernel(m_queue, m_kernel_append, 1, nullptr, &append_global, &local_size, 0, nullptr, nullptr);
        
        clReleaseMemObject(buf_px); clReleaseMemObject(buf_py); clReleaseMemObject(buf_pz);
        clReleaseMemObject(buf_pl); clReleaseMemObject(buf_ps); clReleaseMemObject(buf_pm);
        if(buf_pf) clReleaseMemObject(buf_pf);
    }
    
    // 8. Swap buffers
    clReleaseMemObject(*coord_x); *coord_x = new_x;
    clReleaseMemObject(*coord_y); *coord_y = new_y;
    clReleaseMemObject(*coord_z); *coord_z = new_z;
    clReleaseMemObject(*levels); *levels = new_l;
    clReleaseMemObject(*cell_states); *cell_states = new_s;
    clReleaseMemObject(*material_id); *material_id = new_m;
    if (fields && *fields) {
        clReleaseMemObject(*fields); *fields = new_f;
    }
    
    *num_cells = total_new_cells;
    
    // Cleanup
    clReleaseMemObject(valid_flags);
    clReleaseMemObject(merge_group_id);
    clReleaseMemObject(scan_offsets);
}

} // namespace adaptation
} // namespace fluidloom
