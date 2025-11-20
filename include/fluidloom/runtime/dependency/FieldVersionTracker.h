#pragma once
// @stable - Thread-safe field version tracking for hazard detection

#include <cstdint>
#include <string>
#include <unordered_map>
#include <shared_mutex>
#include <atomic>

namespace fluidloom {
namespace runtime {
namespace dependency {

/**
 * @brief Thread-safe, per-field version tracking for hazard detection
 * 
 * Each field in the SOA has a monotonically increasing version number.
 * When a node writes to a field, the version increments. The version is
 * **immutable** for read operations, allowing lock-free RAW hazard detection.
 * 
 * The version is 64-bit to avoid overflow (2^64 writes before wraparound).
 */
class FieldVersionTracker {
private:
    // Field name → current version (atomic for lock-free reads)
    std::unordered_map<std::string, std::atomic<uint64_t>> versions;
    
    // Field name → last writer node ID (for WAW/WAR detection)
    std::unordered_map<std::string, int64_t> last_writer;
    
    // Global epoch for barrier synchronization
    std::atomic<uint64_t> global_epoch{0};
    
    // Mutex for writer updates (rare compared to reads)
    mutable std::shared_mutex writer_mutex;
    
public:
    FieldVersionTracker() = default;
    ~FieldVersionTracker() = default;
    
    // Non-copyable (singleton per Runtime)
    FieldVersionTracker(const FieldVersionTracker&) = delete;
    FieldVersionTracker& operator=(const FieldVersionTracker&) = delete;
    
    /**
     * @brief Register a new field with version = 0
     * @param field_name Must match FieldRegistry exactly
     */
    void registerField(const std::string& field_name);
    
    /**
     * @brief Get current version (lock-free read)
     * @return Version at time of call (may be stale, but safe for hazard check)
     */
    uint64_t getVersion(const std::string& field_name) const;
    
    /**
     * @brief Increment version after write (called by Executor after kernel completion)
     * @param field_name Field that was written
     * @param node_id ID of node that performed the write
     * @return New version number
     */
    uint64_t incrementVersion(const std::string& field_name, int64_t node_id);
    
    /**
     * @brief Get last writer node ID for WAW detection
     */
    int64_t getLastWriter(const std::string& field_name) const;
    
    /**
     * @brief Increment global epoch (called at barrier nodes)
     * @return New epoch value
     */
    uint64_t incrementEpoch();
    
    /**
     * @brief Get current epoch (for barrier dependency)
     */
    uint64_t getCurrentEpoch() const;
    
    /**
     * @brief Reset all versions (called at simulation start)
     */
    void resetAll();
    
    /**
     * @brief Serialize state for debugging
     */
    std::string toString() const;
};

} // namespace dependency
} // namespace runtime
} // namespace fluidloom
