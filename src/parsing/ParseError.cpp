#include "fluidloom/parsing/ParseError.h"
#include <sstream>

namespace fluidloom {
namespace parsing {

std::string ParseError::toString() const {
    std::stringstream ss;
    ss << "Line " << line << ":" << column << " - " << message;
    if (!context.empty()) {
        ss << "\nContext: " << context;
    }
    return ss.str();
}

} // namespace parsing
} // namespace fluidloom
