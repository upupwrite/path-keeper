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
const std::string LOG_FILE=[]()->std::string
{
        const char* home=std::getenv("HOME");
        if(home){
        return std::string(home)+"/.pk.log";

}
        return "./.pk.log";
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

    // ========== 新增：全局日志开关（去掉了 const） ==========
    bool isGlobalLogEnabled();      // 原为 const，现去掉
    void setGlobalLogEnabled(bool enabled);

    // ========== 新增：命令级日志（去掉了 const） ==========
    bool isCommandLogEnabled(const std::string& dir, int cmdIndex);   // 原 const 去掉
    void setCommandLogEnabled(const std::string& dir, int cmdIndex, bool enabled);

    // ========== 新增：命令别名管理（去掉了 const） ==========
    std::string getCommandAlias(const std::string& dir, int cmdIndex);   // 原 const 去掉
    void setCommandAlias(const std::string& dir, int cmdIndex, const std::string& alias);
    std::string getEffectiveCommand(const std::string& dir, int cmdIndex); // 原 const 去掉

    // ========== 新增：命令哈希管理（去掉了 const） ==========
    std::string computeCommandHash(const std::string& dir, int cmdIndex); // 原 const 去掉
    std::string getCommandHash(const std::string& dir, int cmdIndex);     // 原 const 去掉
    void setCommandHash(const std::string& dir, int cmdIndex, const std::string& hash);
    bool verifyCommandHash(const std::string& dir, int cmdIndex);         // 原 const 去掉
    bool verifyAllCommandsHash();                                         // 原 const 去掉
    void syncCommandHash(const std::string& dir, int cmdIndex);
    void syncAllCommandsHash();

private:
    // 辅助私有方法：获取命令对象的引用（用于修改配置）
    Json::Value& getCommandObject(const std::string& dir, int cmdIndex);
    // 配置格式升级（旧字符串数组 → 新对象数组）
    void upgradeConfigFormat(Json::Value& config);
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
