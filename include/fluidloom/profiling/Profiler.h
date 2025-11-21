#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <chrono>
#include <fstream>
#include <thread>

namespace fluidloom {
namespace profiling {

struct TraceEvent {
    std::string name;
    std::string category;
    std::string phase; // "B" (begin), "E" (end), "X" (complete)
    long long timestamp_us;
    long long duration_us; // Only for "X" phase
    std::thread::id thread_id;
    int pid;
};

class Profiler {
public:
    static Profiler& getInstance();

    void beginEvent(const std::string& name, const std::string& category);
    void endEvent(const std::string& name, const std::string& category);
    void recordCompleteEvent(const std::string& name, const std::string& category, long long start_us, long long duration_us);

    void setOutputPath(const std::string& path);
    void flush();

private:
    Profiler();
    ~Profiler();

    std::vector<TraceEvent> m_events;
    std::mutex m_mutex;
    std::string m_output_path;
    int m_pid;
    long long m_start_time_us;
};

class ScopedEvent {
public:
    ScopedEvent(const std::string& name, const std::string& category = "default");
    ~ScopedEvent();

private:
    std::string m_name;
    std::string m_category;
};

} // namespace profiling
} // namespace fluidloom
