#include "logger.h"

#include <pwd.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

namespace fs = std::filesystem;

static std::string expandTilde(const std::string& path)
{
    if (path.empty() || path[0] != '~')
        return path;
    const char* home = getenv("HOME");
    if (!home)
    {
        struct passwd* pw = getpwuid(getuid());
        if (pw)
            home = pw->pw_dir;
    }
    if (!home)
        return path;
    if (path.size() == 1)
        return home;
    return std::string(home) + path.substr(1);
}

std::string Logger::getLogDir() const { return expandTilde(log_dir_); }

std::string Logger::getLogFilePath() const
{
    std::string dir = getLogDir();
    fs::create_directories(dir);
    auto now = std::time(nullptr);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", std::localtime(&now));
    return dir + "/pk_" + buf + ".log";
}

void Logger::cleanupOldLogs() const
{
    std::string dir = getLogDir();
    if (!fs::exists(dir))
        return;

    auto now = std::time(nullptr);
    for (const auto& entry : fs::directory_iterator(dir))
    {
        if (!entry.is_regular_file() || entry.path().extension() != ".log")
            continue;

        struct stat fileStat;
        if (stat(entry.path().c_str(), &fileStat) != 0)
            continue;

        double days = std::difftime(now, fileStat.st_mtime) / (60 * 60 * 24);
        if (days > max_age_days_)
        {
            fs::remove(entry.path());
        }
    }
}

void Logger::listLogs() const
{
    std::string dir = getLogDir();
    if (!fs::exists(dir))
    {
        std::cerr << "Log directory does not exist: " << dir << std::endl;
        return;
    }
    std::vector<fs::path> logs;
    for (const auto& entry : fs::directory_iterator(dir))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".log")
            logs.push_back(entry.path());
    }
    std::sort(logs.begin(), logs.end());
    std::cerr << "Logs in " << dir << ":\n";
    int i = 1;
    for (const auto& log : logs)
    {
        std::cerr << "  " << i++ << ". " << log.filename().string()
                  << std::endl;
    }
}
