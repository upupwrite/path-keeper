#pragma once
#include <string>

#include "logger.h"

class CommandBuilder
{
public:
    // log_set: 是否明确指定了该命令的日志行为
    // log_value: 当 log_set==true 时，true 表示强制记录，false 表示强制不记录
    static std::string build(const std::string& directory,
                             const std::string& command, const Logger& logger,
                             bool log_set = false, bool log_value = false);

private:
    static std::string shellEscape(const std::string& str);
};
