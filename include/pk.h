#pragma once
#include <jsoncpp/json/json.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <ostream>
#include <string>
#include <thread>
#include <vector>

#include "loadfile.h"
#include "commandbuilder.h"
#include "logger.h"

class PathKeeper
{
public:
    PathKeeper();

    void addRecord();                      // 添加记录
    void showRecord();                     // 显示记录（stderr）
    void setRecent();                      // 设置最近记录（不输出命令）
    void outputRecentCommand();            // 输出 recent 命令到 stdout
    void selectRun(const std::string& cmd_index = "",
                   bool set_recent = true,
                   bool show = true);     // 输出命令并可选更新 recent
    void runPoint(const std::string& cmd_index = "");  // 输出命令，不更新 recent

    std::string cwd;
    File file;

private:
    Logger logger;

    std::string getInputIndex(const std::string& provided_index,
                              const std::string& prompt);
    bool parseIndex(const std::string& index_str,
                    std::vector<std::string>& valid_dirs,
                    Json::Value& paths,
                    std::string& directory,
                    int& cmd_idx);

    void saveRecentRecord(Json::Value& config,
                          const std::string& directory,
                          int cmd_idx);

    void processIndexSelection(const std::string& index_str,
                               Json::Value& paths,
                               Json::Value& config,
                               bool output_command,
                               bool set_recent);

    void outputCommand(const std::string& cmd);

       void displayCommands(const Json::Value& commands, int parent_index = 0);
    void displayRecentMark(const Json::Value& config, const Json::Value& paths);
};
