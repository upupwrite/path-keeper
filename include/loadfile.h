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
#include <json/writer.h>
#include <jsoncpp/json/json.h>
#include <pty.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/stat.h>
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

#include "json/value.h"

namespace Achieve
{
const std::string CONFIG_FILE = []() -> std::string
{
    const char* home = std::getenv("HOME");
    if (home)
    {
        return std::string(home) + "/.pk.json";
    }
    return "./.pk.json";
}();
}  // namespace Achieve

class JsonFormatter
{
private:
    std::ostream& os_;
    int indent_;

public:
    JsonFormatter(std::ostream& os, int indent = 0) : os_(os), indent_(indent)
    {
    }

    ~JsonFormatter() = default;  // 使用默认析构函数

    void writeIndent();
    void writeString(const std::string& str);
    void writeValue(const Json::Value& value, int extraIndent);
    void writeArray(const Json::Value& array, int extraIndent);
    void writeObject(const Json::Value& obj, int extraIndent);
    void writeObject(const Json::Value& obj, int extraIndent,
                     const std::vector<std::string>& keyOrder);
};

class File
{
public:
    std::string CONFIG_FILE;
    static std::vector<std::string> path_keys_order;
    File();
    Json::Value loadConfig();
    void saveConfig(const Json::Value& config);

    void load_key_order();

    std::vector<std::string> get_valid_directories(const Json::Value& paths);

    int get_directory_index_by_display_number(int display_num,
                                              const Json::Value& paths);

    int get_display_number_by_directory_index(int orig_index,
                                              const Json::Value& paths);
};

class Editor
{
public:
    Editor();
    File file;
    const std::string CONFIG_FILE;
    const std::vector<std::string> COMMON_EDITORS;

    Json::Value config = file.loadConfig();
    void printMenu(
        const std::vector<std::pair<std::string, std::string>>& editors);
    std::string getUserChoice(
        const std::vector<std::pair<std::string, std::string>>& editors);
    std::string findInPath(const std::string& prog);
    std::vector<std::pair<std::string, std::string>> getAvailableEditors();
    std::string getEditor();
    void setEditor(const std::string& newEditor);
};
