#pragma once
#include <fcntl.h>
#include <jsoncpp/json/json.h>
#include <pty.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <termios.h>
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
class PathKeeper
{
private:
    std::atomic<bool> stop_output_thread{false};
    File file;

    void displayCommands(const Json::Value &commands, int parent_index = 0);

    void displayRecentMark(const Json::Value &config, const Json::Value &paths);

    std::string getInputIndex(const std::string &provided_index,
                              const std::string &prompt);
    bool parseIndex(const std::string &index_str,
                    std::vector<std::string> &valid_dirs, Json::Value &paths,
                    std::string &directory, int &cmd_idx);

    void saveRecentRecord(Json::Value &config, const std::string &directory,
                          int cmd_idx);

    void processIndexSelection(const std::string &index_str, Json::Value &paths,
                               Json::Value &config, bool execute_command,
                               bool set_recent = true);

public:
    PathKeeper();
    void addRecord();
    void runCommand(const std::string &directory, const std::string &command);
    void setRecent(const std::string &cmd_index = "");
    void runRecent();
    void setCommand();
    Json::Value showRecord(const bool show = true);
    void runPoint(const std::string &cmd_index = "");
    void selectRun(const std::string &cmd_index = "",
                   const bool set_recent = true, const bool show = true);
    void shellCommand(const std::string &command, const std::string &cwd);

    std::string cwd;
};
