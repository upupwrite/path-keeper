#include "commandbuilder.h"

#include <ctime>
#include <iomanip>
#include <sstream>

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
                                  const Logger& logger, bool log_set,
                                  bool log_value)
{
    // 决定是否记录日志：命令级设置优先，否则使用全局设置
    bool should_log = log_set ? log_value : logger.enabled();
    std::string logfile = logger.getLogFilePath();

    std::ostringstream cmd;

    if (should_log)
    {
        // 记录执行信息（时间戳、命令）到日志文件
        cmd << "printf '[%s] %s\\n' \"$(date '+%Y-%m-%d %H:%M:%S')\" "
            << "\"executing: " << command << "\""
            << " >> " << shellEscape(logfile) << "; ";
    }

    // 目录切换 + 实际命令
    cmd << "cd " << shellEscape(directory);
    if (!command.empty())
    {
        cmd << " && " << command;
    }

    if (should_log)
    {
        // 将命令输出（stdout+stderr）同时追加到日志文件
        std::string inner = cmd.str();
        cmd.str("");
        cmd << "( " << inner << " ) 2>&1 | tee -a " << shellEscape(logfile);
    }

    return cmd.str();
}