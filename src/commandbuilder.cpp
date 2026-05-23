#include "commandbuilder.h"
#include <sstream>
#include <ctime>
#include <iomanip>

std::string CommandBuilder::shellEscape(const std::string& str)
{
    // 用单引号包裹，内部单引号替换为 '\''
    std::string escaped = "'";
    for (char c : str)
    {
        if (c == '\'')
            escaped += "'\\''";
        else
            escaped += c;
    }
    escaped += "'";
    return escaped;
}

std::string CommandBuilder::build(const std::string& directory,
                                  const std::string& command,
                                  const Logger& logger)
{
    std::ostringstream cmd;

    // 目录切换
    cmd << "cd " << shellEscape(directory);

    // 如果有命令，用 && 连接
    if (!command.empty())
    {
        cmd << " && " << command;
    }

    // 如果日志启用，使用 tee 将输出同时写入日志文件
    if (logger.enabled())
    {
        std::string logfile = logger.getLogFilePath();
        std::string inner = cmd.str();
        cmd.str("");
        cmd << "( " << inner << " ) 2>&1 | tee " << shellEscape(logfile);
    }

    return cmd.str();
}
