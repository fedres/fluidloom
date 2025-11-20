#pragma once

#include <string>
#include <cstdlib>
#include "fluidloom/common/Logger.h"

namespace fluidloom {

class Config {
public:
    static void loadFromEnvironment() {
        const char* log_level = std::getenv("FL_LOG_LEVEL");
        if (log_level) {
            std::string level_str(log_level);
            if (level_str == "DEBUG") Logger::instance().setLevel(LogLevel::DEBUG);
            else if (level_str == "INFO") Logger::instance().setLevel(LogLevel::INFO);
            else if (level_str == "WARN") Logger::instance().setLevel(LogLevel::WARN);
            else if (level_str == "ERROR") Logger::instance().setLevel(LogLevel::ERROR);
            else if (level_str == "FATAL") Logger::instance().setLevel(LogLevel::FATAL);
        }
    }
};

} // namespace fluidloom
