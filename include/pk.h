// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Path Keeper Contributors
// This file is part of Path Keeper.
// Path Keeper is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// Path Keeper is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
// You should have received a copy of the GNU General Public License
// along with Path Keeper. If not, see <https://www.gnu.org/licenses/>.

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
#include "terminal.h"
class PathKeeper
{
private:
    File file;
    Shell shell;

    void displayCommands(const Json::Value &commands, int parent_index = 0);

    void displayRecentMark(const Json::Value &config, const Json::Value &paths);

    std::string getInputIndex(const std::string &provided_index,
                              const std::string &prompt);

    void saveRecentRecord(Json::Value &config, const std::string &directory,
                          int cmd_idx);

    void processIndexSelection(const std::string &index_str, const Json::Value &paths,
                               Json::Value &config, bool execute_command,
                               bool set_recent = true);

public:
    PathKeeper();
    void addRecord();
    void runCommand(const std::string &directory, const std::string &command,const std::string &extra);
    void runExtra(const std::string &index,const std::string &extra);
    void setRecent(const std::string &cmd_index = "");
    void runRecent();
    void setCommand();
    void search();
    Json::Value showRecord(const bool show = true);
    void runPoint(const std::string &cmd_index = "",const std::string &extra="");
    void selectRun(const std::string &cmd_index = "",
                   const bool set_recent = true, const bool show = true);
    bool parseIndex(const std::string &index_str,
                    std::vector<std::string> &valid_dirs, Json::Value &paths,
                    std::string &directory, int &cmd_idx);
    std::string cwd;
};
