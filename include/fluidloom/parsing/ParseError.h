#pragma once

#include <string>

namespace fluidloom {
namespace parsing {

struct ParseError {
    std::string message;
    int line;
    int column;
    std::string context;
    
    std::string toString() const;
};

} // namespace parsing
} // namespace fluidloom
