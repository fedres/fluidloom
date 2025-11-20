#pragma once

#include <string>
#include <sstream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <iostream>

namespace fluidloom {

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3,
    FATAL = 4
};

class Logger {
public:
    static Logger& instance() {
        static Logger instance;
        return instance;
    }

    // Non-copyable
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    void setLevel(LogLevel level) { m_level = level; }
    LogLevel getLevel() const { return m_level; }

    void log(LogLevel level, const std::string& file, int line, const std::string& message) {
        if (level < m_level) return;
        
        std::lock_guard<std::mutex> lock(m_mutex);
        
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::cerr << "[" << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S")
                  << "." << std::setfill('0') << std::setw(3) << ms.count()
                  << "] [" << logLevelToString(level) << "] "
                  << "[" << file << ":" << line << "] "
                  << message << std::endl;
    }

private:
    Logger() : m_level(LogLevel::INFO) {}
    ~Logger() = default;

    const char* logLevelToString(LogLevel level) {
        static const char* levels[] = {"DEBUG", "INFO", "WARN", "ERROR", "FATAL"};
        return levels[static_cast<int>(level)];
    }

    LogLevel m_level;
    std::mutex m_mutex;
};

// Helper function for streaming
class LogStream {
public:
    LogStream(LogLevel level, const std::string& file, int line) 
        : m_level(level), m_file(file), m_line(line) {}
    
    ~LogStream() {
        fluidloom::Logger::instance().log(m_level, m_file, m_line, m_stream.str());
    }
    
    template<typename T>
    LogStream& operator<<(const T& value) {
        m_stream << value;
        return *this;
    }

private:
    LogLevel m_level;
    std::string m_file;
    int m_line;
    std::ostringstream m_stream;
};

// Factory function
inline LogStream makeLoggerStream(LogLevel level, const std::string& file, int line) {
    return LogStream(level, file, line);
}

} // namespace fluidloom

// Macro for convenient logging
#define FL_LOG(level) \
    if (fluidloom::LogLevel::level >= fluidloom::Logger::instance().getLevel()) \
        fluidloom::makeLoggerStream(fluidloom::LogLevel::level, __FILE__, __LINE__)
