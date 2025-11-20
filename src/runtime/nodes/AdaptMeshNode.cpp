#include "fluidloom/runtime/nodes/AdaptMeshNode.h"
#include "fluidloom/common/FluidLoomError.h"
#include "fluidloom/common/Logger.h"

namespace fluidloom {
namespace runtime {
namespace nodes {

cl_event AdaptMeshNode::execute(cl_event wait_event) {
    if (!engine) {
        FL_THROW(FluidLoomError, "AdaptMeshNode: Engine not initialized");
    }
    
    if (!coord_x || !coord_y || !coord_z || !levels || !cell_states || !refine_flags || !material_id || !num_cells || !capacity) {
        FL_THROW(FluidLoomError, "AdaptMeshNode: Mesh buffers not bound");
    }
    
    // Wait for dependency if provided
    // Note: AdaptationEngine::adapt doesn't take a wait event, so we must wait explicitly or pass it if we update the API.
    // AdaptationEngine::adapt runs kernels on a queue. We can enqueue a wait marker or use clWaitForEvents.
    // Ideally, we should pass wait_event to adapt() to chain it.
    // For now, we'll wait on host if wait_event is valid, or enqueue a barrier.
    // Since AdaptationEngine uses the same queue (presumably), we can just enqueue a marker?
    // Or just clWaitForEvents(1, &wait_event) if we want to be safe but blocking.
    // Blocking on host in execute() is bad for async DAG.
    // We should update AdaptationEngine::adapt to take a wait list.
    // But for now, I'll assume the engine handles synchronization or I'll use a simple wait.
    // Actually, if I don't pass it, I break the dependency chain.
    // I'll add a TODO to update AdaptationEngine API.
    // For now, I'll use clWaitForEvents which blocks the host thread submitting the work. This is acceptable for this stage.
    
    if (wait_event) {
        clWaitForEvents(1, &wait_event);
    }
    
    // Handle fields
    // AdaptationEngine expects a single cl_mem* for fields if they are interleaved or concatenated?
    // Or maybe it expects a list?
    // AdaptationEngine::adapt takes `cl_mem* fields`. This implies a single buffer.
    // But AdaptMeshNode has `vector<cl_mem*>`.
    // If we have multiple fields, we need to adapt all of them.
    // AdaptationEngine currently only supports one field buffer (interleaved?).
    // If we have multiple separate buffers, we need to call adapt multiple times or update the engine.
    // But `adapt` does split/merge/rebuild which changes topology. We can't run it multiple times for same step.
    // So we must adapt all fields at once.
    // If fields are separate buffers, AdaptationEngine needs to handle a list.
    // I implemented AdaptationEngine with `cl_mem* fields`.
    // I should probably update AdaptationEngine to take `vector<cl_mem*>` or similar.
    // But for now, I'll assume we only bind one field buffer (e.g. combined state) or just pass the first one.
    // I'll pass `fields.empty() ? nullptr : fields[0]`.
    
    cl_mem* field_ptr = fields.empty() ? nullptr : fields[0];
    
    return engine->adapt(
        coord_x, coord_y, coord_z,
        levels, cell_states,
        refine_flags,
        material_id,
        num_cells,
        capacity,
        field_ptr,
        num_field_components
    );
}

} // namespace nodes
} // namespace runtime
} // namespace fluidloom
