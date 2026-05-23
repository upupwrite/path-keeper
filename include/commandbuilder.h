#pragma once
#include <string>
#include "logger.h"

class CommandBuilder {
public:
    static std::string build(const std::string& directory,
                             const std::string& command,
                             const Logger& logger);
private:
    static std::string shellEscape(const std::string& str);
};
