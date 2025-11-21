#include "fluidloom/profiling/Profiler.h"
#include "fluidloom/common/Logger.h"
#include <iostream>
#include <sstream>

#ifdef _WIN32
#include <process.h>
#define GET_PID _getpid
#else
#include <unistd.h>
#define GET_PID getpid
#endif

namespace fluidloom {
namespace profiling {

Profiler& Profiler::getInstance() {
    static Profiler instance;
    return instance;
}

Profiler::Profiler() : m_output_path("trace.json") {
    m_pid = GET_PID();
    auto now = std::chrono::steady_clock::now();
    m_start_time_us = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
}

Profiler::~Profiler() {
    flush();
}

void Profiler::setOutputPath(const std::string& path) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_output_path = path;
}

void Profiler::beginEvent(const std::string& name, const std::string& category) {
    auto now = std::chrono::steady_clock::now();
    long long ts = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();

    std::lock_guard<std::mutex> lock(m_mutex);
    m_events.push_back({name, category, "B", ts, 0, std::this_thread::get_id(), m_pid});
}

void Profiler::endEvent(const std::string& name, const std::string& category) {
    auto now = std::chrono::steady_clock::now();
    long long ts = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();

    std::lock_guard<std::mutex> lock(m_mutex);
    m_events.push_back({name, category, "E", ts, 0, std::this_thread::get_id(), m_pid});
}

void Profiler::recordCompleteEvent(const std::string& name, const std::string& category, long long start_us, long long duration_us) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_events.push_back({name, category, "X", start_us, duration_us, std::this_thread::get_id(), m_pid});
}

void Profiler::flush() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_events.empty()) return;

    FL_LOG(INFO) << "Flushing " << m_events.size() << " trace events to " << m_output_path;

    std::ofstream file(m_output_path);
    if (!file.is_open()) {
        FL_LOG(ERROR) << "Failed to open trace output file: " << m_output_path;
        return;
    }

    file << "[\n";
    for (size_t i = 0; i < m_events.size(); ++i) {
        const auto& e = m_events[i];
        file << "  {";
        file << "\"name\": \"" << e.name << "\", ";
        file << "\"cat\": \"" << e.category << "\", ";
        file << "\"ph\": \"" << e.phase << "\", ";
        file << "\"ts\": " << e.timestamp_us << ", ";
        if (e.phase == "X") {
            file << "\"dur\": " << e.duration_us << ", ";
        }
        file << "\"pid\": " << e.pid << ", ";
        
        // Thread ID hash
        std::hash<std::thread::id> hasher;
        file << "\"tid\": " << hasher(e.thread_id);
        
        file << "}";
        if (i < m_events.size() - 1) {
            file << ",\n";
        }
    }
    file << "\n]\n";
    file.close();
    
    m_events.clear();
}

ScopedEvent::ScopedEvent(const std::string& name, const std::string& category)
    : m_name(name), m_category(category) {
    Profiler::getInstance().beginEvent(m_name, m_category);
}

ScopedEvent::~ScopedEvent() {
    Profiler::getInstance().endEvent(m_name, m_category);
}

} // namespace profiling
} // namespace fluidloom
