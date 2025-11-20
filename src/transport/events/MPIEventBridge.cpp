#include "fluidloom/transport/MPIEventBridge.h"
#include "fluidloom/common/Logger.h"

namespace fluidloom {
namespace transport {

MPIEventBridge::MPIEventBridge(IBackend* backend) 
    : backend(backend), stop_polling(false) {
    (void)this->backend; // Suppress unused warning (will be used when getContext is available)
    
    polling_thread = std::thread(&MPIEventBridge::pollingLoop, this);
}

MPIEventBridge::~MPIEventBridge() {
    shutdown();
}

void MPIEventBridge::shutdown() {
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        stop_polling = true;
    }
    queue_cv.notify_all();
    if (polling_thread.joinable()) {
        polling_thread.join();
    }
}

cl_event MPIEventBridge::bridgeMPIRequest(MPIRequestWrapper* request) {
    // If it's already an OpenCL event (P2P), just return it
    if (auto* event = request->getCLEvent()) {
        // Need to retain it because the caller might release it or use it
        clRetainEvent(*event);
        return *event;
    }
    
    // For MPI requests, we create a user event and signal it when MPI completes
    // cl_context context = backend->getContext(); // IBackend doesn't expose this yet
    // For now, we can't create user events without context
    // We'll need to extend IBackend or use a different approach
    // Placeholder: return nullptr for now
    (void)request; // Suppress unused warning
    return nullptr; // TODO: Implement proper event bridging
    
    /*
    cl_int err;
    cl_context context = nullptr; // backend->getContext();
    // Assuming IBackend has getContext. If not, we might need to add it or use a workaround.
    // Module 7 didn't use getContext.
    // Let's assume we can get it or create a user event via backend.
    // If IBackend doesn't expose it, we are stuck.
    // Let's check IBackend.h later. For now, assume we can get it.
    // Or better, use a callback mechanism if available.
    // But OpenCL 1.2 user events are standard.
    
    // Workaround: If IBackend doesn't expose context, we can't create user event easily.
    // But we can use a marker event? No.
    // We need to modify IBackend to expose context or create user event.
    // I'll assume `backend->createUserEvent()` exists or I'll add it.
    // Or I'll use `clCreateUserEvent` with `backend->getContext()`.
    
    // For now, let's assume `backend->getContext()` is available.
    // If not, I'll have to add it.
    
    cl_event user_event = clCreateUserEvent(context, &err);
    if (err != CL_SUCCESS) {
        FL_LOG(ERROR) << "Failed to create user event: " << err;
        return nullptr;
    }
    
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        pending_tests.push({request, user_event});
    }
    queue_cv.notify_one();
    
    return user_event;
    */
}

bool MPIEventBridge::isMPIComplete(MPIRequestWrapper* request) {
    return request->test();
}

void MPIEventBridge::pollingLoop() {
    while (true) {
        std::unique_lock<std::mutex> lock(queue_mutex);
        queue_cv.wait(lock, [this] { return stop_polling || !pending_tests.empty(); });
        
        if (stop_polling && pending_tests.empty()) {
            break;
        }
        
        // Process pending tests
        // We iterate and check. If not complete, push back.
        // This is a busy loop if we are not careful.
        // Better to pop all, check, and push back incomplete ones.
        
        size_t count = pending_tests.size();
        for (size_t i = 0; i < count; ++i) {
            auto item = pending_tests.front();
            pending_tests.pop();
            
            if (isMPIComplete(item.first)) {
                clSetUserEventStatus(item.second, CL_COMPLETE);
                clReleaseEvent(item.second); // Release our reference
            } else {
                pending_tests.push(item);
            }
        }
        
        lock.unlock();
        
        if (!pending_tests.empty()) {
            std::this_thread::sleep_for(std::chrono::microseconds(10)); // Avoid busy wait
        }
    }
}

} // namespace transport
} // namespace fluidloom
