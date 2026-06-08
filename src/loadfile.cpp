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

#include "loadfile.h"

#include <sys/stat.h>

#include <algorithm>
#include <chrono>
#include <ctime>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

#include "colors.h"

// 在类外部定义静态成员变量
std::vector<std::string> File::path_keys_order;

namespace
{
// 常量定义
const Json::Value DEFAULT_CONFIG = []()
{
    Json::Value config;
    config["global_log"] = false;  // 新增全局日志开关
    config["path"] = Json::objectValue;
    config["recent"] = Json::nullValue;
    config["shell"] = "sh";
    config["editor"] = "";
    return config;
}();

// 简单的转义字符映射
struct JsonEscape
{
    static std::string escapeChar(char c)
    {
        switch (c)
        {
        case '"':
            return "\\\"";
        case '\\':
            return "\\\\";
        case '\b':
            return "\\b";
        case '\f':
            return "\\f";
        case '\n':
            return "\\n";
        case '\r':
            return "\\r";
        case '\t':
            return "\\t";
        default:
            if (static_cast<unsigned char>(c) < 0x20 || c == 0x7f)
            {
                char buf[7];
                snprintf(buf, sizeof(buf), "\\u%04x",
                         static_cast<unsigned char>(c));
                return buf;
            }
            return std::string(1, c);
        }
    }
};
}  // namespace

// 辅助类：JSON格式化器
void JsonFormatter::writeIndent()
{
    for (int i = 0; i < indent_; ++i)
    {
        os_ << ' ';
    }
}

void JsonFormatter::writeString(const std::string& str)
{
    os_ << '"';
    for (char c : str)
    {
        os_ << JsonEscape::escapeChar(c);
    }
    os_ << '"';
}

void JsonFormatter::writeValue(const Json::Value& value, int extraIndent)
{
    switch (value.type())
    {
    case Json::nullValue:
        os_ << "null";
        break;
    case Json::intValue:
        os_ << value.asInt();
        break;
    case Json::uintValue:
        os_ << value.asUInt();
        break;
    case Json::realValue:
        os_ << value.asDouble();
        break;
    case Json::stringValue:
        writeString(value.asString());
        break;
    case Json::booleanValue:
        os_ << (value.asBool() ? "true" : "false");
        break;
    case Json::arrayValue:
        writeArray(value, extraIndent);
        break;
    case Json::objectValue:
        writeObject(value, extraIndent);
        break;
    }
}

void JsonFormatter::writeArray(const Json::Value& array, int extraIndent)
{
    if (array.empty())
    {
        os_ << "[]";
        return;
    }

    os_ << "[";
    int newIndent = indent_ + extraIndent;

    for (Json::ArrayIndex i = 0; i < array.size(); ++i)
    {
        if (i > 0)
            os_ << ",";
        os_ << "\n";
        for (int j = 0; j < newIndent; ++j) os_ << ' ';

        JsonFormatter formatter(os_, newIndent);
        formatter.writeValue(array[i], extraIndent);
    }

    os_ << "\n";
    for (int j = 0; j < indent_; ++j) os_ << ' ';
    os_ << "]";
}

// 原始函数：按字母顺序写入对象
void JsonFormatter::writeObject(const Json::Value& obj, int extraIndent)
{
    if (obj.empty())
    {
        os_ << "{}";
        return;
    }

    os_ << "{";
    int newIndent = indent_ + extraIndent;
    bool first = true;

    Json::Value::Members members = obj.getMemberNames();
    std::sort(members.begin(), members.end());  // 按字母排序

    for (size_t i = 0; i < members.size(); ++i)
    {
        if (!first)
            os_ << ",";
        os_ << "\n";
        for (int j = 0; j < newIndent; ++j) os_ << ' ';

        writeString(members[i]);
        os_ << ": ";

        JsonFormatter formatter(os_, newIndent);
        formatter.writeValue(obj[members[i]], extraIndent);
        first = false;
    }

    os_ << "\n";
    for (int j = 0; j < indent_; ++j) os_ << ' ';
    os_ << "}";
}

// 新增重载函数：按照指定顺序写入对象
void JsonFormatter::writeObject(const Json::Value& obj, int extraIndent,
                                const std::vector<std::string>& keyOrder)
{
    if (obj.empty())
    {
        os_ << "{}";
        return;
    }

    os_ << "{";
    int newIndent = indent_ + extraIndent;
    bool first = true;

    // 使用指定的顺序
    for (const auto& key : keyOrder)
    {
        if (!obj.isMember(key))
            continue;

        if (!first)
            os_ << ",";
        os_ << "\n";
        for (int j = 0; j < newIndent; ++j) os_ << ' ';

        writeString(key);
        os_ << ": ";

        JsonFormatter formatter(os_, newIndent);
        formatter.writeValue(obj[key], extraIndent);
        first = false;
    }

    os_ << "\n";
    for (int j = 0; j < indent_; ++j) os_ << ' ';
    os_ << "}";
}

// ---文件处理函数---

File::File() { CONFIG_FILE = Achieve::CONFIG_FILE; }

// 升级旧配置格式到新格式（字符串数组 → 对象数组）
void File::upgradeConfigFormat(Json::Value& config)
{
    bool changed = false;
    Json::Value& paths = config["path"];
    if (!paths.isObject())
        return;

    Json::Value::Members dirs = paths.getMemberNames();
    for (const auto& dir : dirs)
    {
        Json::Value& commands = paths[dir];
        if (commands.isArray())
        {
            for (Json::ArrayIndex i = 0; i < commands.size(); ++i)
            {
                if (commands[i].isString())
                {
                    // 旧格式：字符串，转换为 { "cmd": "..." }
                    std::string oldCmd = commands[i].asString();
                    Json::Value newCmd;
                    newCmd["cmd"] = oldCmd;
                    commands[i] = newCmd;
                    changed = true;
                }
            }
        }
    }

    if (changed)
    {
        // 确保 global_log 存在
        if (!config.isMember("global_log"))
            config["global_log"] = false;
        saveConfig(config);
    }
}

Json::Value File::loadConfig()
{
    std::ifstream file(CONFIG_FILE);
    if (!file.is_open())
    {
        Json::Value defaultConfig = DEFAULT_CONFIG;
        saveConfig(defaultConfig);
        return defaultConfig;
    }

    Json::Value config;
    try
    {
        file >> config;
        // 确保必要的字段存在
        if (!config.isMember("path"))
            config["path"] = Json::objectValue;
        if (!config.isMember("recent"))
            config["recent"] = Json::nullValue;
        if (!config.isMember("shell"))
            config["shell"] = "sh";
        if (!config.isMember("editor"))
            config["editor"] = "";
        if (!config.isMember("global_log"))
            config["global_log"] = false;

        // 升级旧格式（字符串数组 → 对象数组）
        upgradeConfigFormat(config);
    }
    catch (const Json::Exception& e)
    {
        std::cerr << Colors::RED << "解析配置文件错误: " << e.what()
                  << Colors::RESET << std::endl;
    }

    return config;
}

void File::saveConfig(const Json::Value& config)
{
    std::ofstream file(CONFIG_FILE);
    if (!file.is_open())
        return;

    // 确保键的顺序：首先是现有的顺序，然后是新的键
    Json::Value paths = config["path"];
    Json::Value::Members memberNames = paths.getMemberNames();

    // 合并顺序：保持现有顺序，添加新键到末尾
    std::vector<std::string> orderedKeys = path_keys_order;
    for (Json::Value::Members::const_iterator it = memberNames.begin();
         it != memberNames.end(); ++it)
    {
        if (std::find(orderedKeys.begin(), orderedKeys.end(), *it) ==
            orderedKeys.end())
        {
            orderedKeys.push_back(*it);
        }
    }

    // 使用与文件1相同的格式：先写起始大括号和global_log、path字段
    file << "{\n";

    // 写入 global_log
    file << "  \"global_log\": ";
    JsonFormatter(file, 2).writeValue(config["global_log"], 2);

    // 写入 path 部分，使用特定的键顺序
    file << ",\n  \"path\": ";
    {
        JsonFormatter formatter(file, 2);
        formatter.writeObject(paths, 2, orderedKeys);
    }

    // 写入其他字段
    if (config.isMember("recent") && !config["recent"].isNull())
    {
        file << ",\n  \"recent\": ";
        JsonFormatter(file, 2).writeValue(config["recent"], 2);
    }

    if (config.isMember("shell"))
    {
        file << ",\n  \"shell\": ";
        JsonFormatter(file, 2).writeValue(config["shell"], 2);
    }

    if (config.isMember("editor"))
    {
        file << ",\n  \"editor\": ";
        JsonFormatter(file, 2).writeValue(config["editor"], 2);
    }

    file << "\n}";

    // 更新键顺序
    path_keys_order = orderedKeys;
}

void File::load_key_order()
{
    path_keys_order.clear();

    std::ifstream file(CONFIG_FILE);
    if (!file.is_open())
        return;

    std::string line;
    bool in_path_section = false;
    int brace_count = 0;

    while (std::getline(file, line))
    {
        // 去除前导空白
        std::string::size_type firstNonSpace = line.find_first_not_of(" \t");
        if (firstNonSpace == std::string::npos)
            continue;

        // 检查是否进入path部分
        if (line.find("\"path\":") != std::string::npos)
        {
            in_path_section = true;
            // 查找path对象开始的大括号
            size_t brace_pos = line.find('{', firstNonSpace);
            if (brace_pos != std::string::npos)
            {
                brace_count = 1;
            }
            continue;
        }

        if (in_path_section)
        {
            // 统计大括号
            for (char c : line)
            {
                if (c == '{')
                    brace_count++;
                else if (c == '}')
                    brace_count--;
            }

            // 检查是否到达路径部分的结束
            if (brace_count == 0)
            {
                break;
            }

            // 提取键名（忽略注释行）
            if (line.find("//") != std::string::npos)
                continue;

            std::string::size_type quoteStart = line.find('"', firstNonSpace);
            if (quoteStart == std::string::npos)
                continue;

            std::string::size_type quoteEnd = line.find('"', quoteStart + 1);
            if (quoteEnd == std::string::npos)
                continue;

            // 确保后面有冒号（表示这是一个键值对）
            if (line.find(':', quoteEnd) == std::string::npos)
                continue;

            std::string key =
                line.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
            if (!key.empty())
            {
                path_keys_order.push_back(key);
            }
        }
    }
}

std::vector<std::string> File::get_valid_directories(const Json::Value& paths)
{
    std::vector<std::string> validDirs;

    for (std::vector<std::string>::const_iterator it = path_keys_order.begin();
         it != path_keys_order.end(); ++it)
    {
        if (paths.isMember(*it))
        {
            validDirs.push_back(*it);
        }
    }

    return validDirs;
}

int File::get_directory_index_by_display_number(int display_num,
                                                const Json::Value& paths)
{
    std::vector<std::string> validDirs = get_valid_directories(paths);

    if (display_num < 1 || display_num > static_cast<int>(validDirs.size()))
    {
        return -1;
    }

    std::string selectedDir = validDirs[display_num - 1];

    // 在原始顺序中查找
    for (size_t i = 0; i < path_keys_order.size(); ++i)
    {
        if (path_keys_order[i] == selectedDir && paths.isMember(selectedDir))
        {
            return static_cast<int>(i);
        }
    }

    return -1;
}

int File::get_display_number_by_directory_index(int orig_index,
                                                const Json::Value& paths)
{
    if (orig_index < 0 ||
        orig_index >= static_cast<int>(path_keys_order.size()))
    {
        return -1;
    }

    std::string directory = path_keys_order[orig_index];
    if (!paths.isMember(directory))
    {
        return -1;
    }

    std::vector<std::string> validDirs = get_valid_directories(paths);
    std::vector<std::string>::iterator it =
        std::find(validDirs.begin(), validDirs.end(), directory);

    if (it != validDirs.end())
    {
        return static_cast<int>(std::distance(validDirs.begin(), it) + 1);
    }

    return -1;
}

// ===================== 新增功能：命令级元数据管理 =====================

// 辅助函数：获取命令对象的引用（用于读写）
Json::Value& File::getCommandObject(const std::string& dir, int cmdIndex)
{
    Json::Value config = loadConfig();  // 临时加载，但修改后需保存
    // 注意：此实现会多次加载配置，性能较低，实际使用时建议缓存 config 成员变量
    // 为简单起见，直接操作并保存，后续保存函数会写回文件。
    static Json::Value cachedConfig;  // 简单静态缓存，仅作演示
    static bool loaded = false;
    if (!loaded)
    {
        cachedConfig = loadConfig();
        loaded = true;
    }
    Json::Value& paths = cachedConfig["path"];
    if (!paths.isMember(dir))
    {
        throw std::out_of_range("Directory not found: " + dir);
    }
    Json::Value& cmdArray = paths[dir];
    if (!cmdArray.isArray() || cmdIndex < 0 || cmdIndex >= (int)cmdArray.size())
    {
        throw std::out_of_range("Command index out of range");
    }
    return cmdArray[cmdIndex];
}

// 获取全局日志开关状态
bool File::isGlobalLogEnabled()
{
    Json::Value config = loadConfig();
    return config["global_log"].asBool();
}

// 设置全局日志开关
void File::setGlobalLogEnabled(bool enabled)
{
    Json::Value config = loadConfig();
    config["global_log"] = enabled;
    saveConfig(config);
}

// 获取指定命令的日志开关（若命令单独定义则返回命令的log，否则返回全局log）
bool File::isCommandLogEnabled(const std::string& dir, int cmdIndex)
{
    try
    {
        Json::Value config = loadConfig();
        Json::Value& cmdObj =
            const_cast<Json::Value&>(config["path"][dir][cmdIndex]);
        if (cmdObj.isMember("log"))
        {
            return cmdObj["log"].asBool();
        }
        return config["global_log"].asBool();
    }
    catch (...)
    {
        return isGlobalLogEnabled();  // 出错时回退到全局设置
    }
}

// 设置指定命令的日志开关
void File::setCommandLogEnabled(const std::string& dir, int cmdIndex,
                                bool enabled)
{
    Json::Value config = loadConfig();
    Json::Value& cmdObj = config["path"][dir][cmdIndex];
    cmdObj["log"] = enabled;
    saveConfig(config);
}

// 获取命令别名
std::string File::getCommandAlias(const std::string& dir, int cmdIndex)
{
    try
    {
        Json::Value config = loadConfig();
        Json::Value& cmdObj =
            const_cast<Json::Value&>(config["path"][dir][cmdIndex]);
        if (cmdObj.isMember("alias") && cmdObj["alias"].isString())
        {
            return cmdObj["alias"].asString();
        }
    }
    catch (...)
    {
    }
    return "";
}

// 设置命令别名
void File::setCommandAlias(const std::string& dir, int cmdIndex,
                           const std::string& alias)
{
    Json::Value config = loadConfig();
    Json::Value& cmdObj = config["path"][dir][cmdIndex];
    if (alias.empty())
    {
        if (cmdObj.isMember("alias"))
            cmdObj.removeMember("alias");
    }
    else
    {
        cmdObj["alias"] = alias;
    }
    saveConfig(config);
}

// 获取实际执行的命令字符串（优先使用别名，否则使用原cmd）
std::string File::getEffectiveCommand(const std::string& dir, int cmdIndex)
{
    //    std::string alias = getCommandAlias(dir, cmdIndex);
    //    if (!alias.empty())
    //        return alias;
    try
    {
        Json::Value config = loadConfig();
        Json::Value& cmdObj =
            const_cast<Json::Value&>(config["path"][dir][cmdIndex]);
        if (cmdObj.isMember("cmd") && cmdObj["cmd"].isString())
        {
            return cmdObj["cmd"].asString();
        }
    }
    catch (...)
    {
    }
    return "";
}

// 计算命令的哈希值（基于 cmd 字段）
std::string File::computeCommandHash(const std::string& dir, int cmdIndex)
{
    std::string cmdStr;
    try
    {
        Json::Value config = loadConfig();
        Json::Value& cmdObj =
            const_cast<Json::Value&>(config["path"][dir][cmdIndex]);
        if (cmdObj.isMember("cmd") && cmdObj["cmd"].isString())
        {
            cmdStr = cmdObj["cmd"].asString();
        }
        else
        {
            return "";
        }
    }
    catch (...)
    {
        return "";
    }
    std::size_t h = std::hash<std::string>{}(cmdStr);
    std::stringstream ss;
    ss << std::hex << std::setw(sizeof(h) * 2) << std::setfill('0') << h;
    return ss.str();
}

// 获取存储的命令哈希值
std::string File::getCommandHash(const std::string& dir, int cmdIndex)
{
    try
    {
        Json::Value config = loadConfig();
        Json::Value& cmdObj =
            const_cast<Json::Value&>(config["path"][dir][cmdIndex]);
        if (cmdObj.isMember("hash") && cmdObj["hash"].isString())
        {
            return cmdObj["hash"].asString();
        }
    }
    catch (...)
    {
    }
    return "";
}

// 设置（更新）命令哈希值
void File::setCommandHash(const std::string& dir, int cmdIndex,
                          const std::string& hash)
{
    Json::Value config = loadConfig();
    Json::Value& cmdObj = config["path"][dir][cmdIndex];
    if (hash.empty())
    {
        if (cmdObj.isMember("hash"))
            cmdObj.removeMember("hash");
    }
    else
    {
        cmdObj["hash"] = hash;
    }
    saveConfig(config);
}

// 验证单个命令哈希是否一致
bool File::verifyCommandHash(const std::string& dir, int cmdIndex)
{
    std::string stored = getCommandHash(dir, cmdIndex);
    std::string computed = computeCommandHash(dir, cmdIndex);
    if (stored.empty() && computed.empty())
        return true;  // 无哈希视为一致
    return stored == computed;
}

// 验证所有命令的哈希是否一致
bool File::verifyAllCommandsHash()
{
    Json::Value config = loadConfig();
    Json::Value paths = config["path"];
    Json::Value::Members dirs = paths.getMemberNames();
    for (const auto& dir : dirs)
    {
        const Json::Value& cmds = paths[dir];
        if (!cmds.isArray())
            continue;
        for (Json::ArrayIndex i = 0; i < cmds.size(); ++i)
        {
            if (!verifyCommandHash(dir, i))
                return false;
        }
    }
    return true;
}

// 同步（重新计算并更新）单个命令的哈希
void File::syncCommandHash(const std::string& dir, int cmdIndex)
{
    std::string newHash = computeCommandHash(dir, cmdIndex);
    setCommandHash(dir, cmdIndex, newHash);
}

// 同步所有命令的哈希
void File::syncAllCommandsHash()
{
    Json::Value config = loadConfig();
    Json::Value paths = config["path"];
    Json::Value::Members dirs = paths.getMemberNames();
    for (const auto& dir : dirs)
    {
        Json::Value& cmds = paths[dir];
        if (!cmds.isArray())
            continue;
        for (Json::ArrayIndex i = 0; i < cmds.size(); ++i)
        {
            std::string newHash = computeCommandHash(dir, i);
            cmds[i]["hash"] = newHash;
        }
    }
    saveConfig(config);
}

int File::getCommandIndex(const std::string& dir, const std::string& commandStr)
{
    Json::Value config = loadConfig();
    Json::Value paths = config["path"];
    if (!paths.isMember(dir))
        return -1;

    Json::Value cmds = paths[dir];
    if (!cmds.isArray())
        return -1;

    for (Json::ArrayIndex i = 0; i < cmds.size(); ++i)
    {
        // 获取原始命令
        std::string cmd;
        if (cmds[i].isObject() && cmds[i].isMember("cmd"))
            cmd = cmds[i]["cmd"].asString();
        else if (cmds[i].isString())
            cmd = cmds[i].asString();  // 兼容旧格式
        else
            continue;

        // 获取别名（如果有）
        std::string alias;
        if (cmds[i].isObject() && cmds[i].isMember("alias"))
            alias = cmds[i]["alias"].asString();

        // 匹配原始命令或别名
        if (cmd == commandStr || (!alias.empty() && alias == commandStr))
            return static_cast<int>(i);
    }
    return -1;
}


Editor::Editor()
    : COMMON_EDITORS({"vim", "nvim", "nano", "emacs", "micro", "helix", "code",
                      "codium", "kate", "gedit", "mousepad", "ne", "jed",
                      "joe"})
{
}
std::string Editor::findInPath(const std::string& prog)
{
    // 使用 which 命令查找可执行文件的完整路径
    std::string cmd = "which " + prog + " 2>/dev/null";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe)
        return "";

    char buffer[128];
    std::string result;

    if (fgets(buffer, sizeof(buffer), pipe) != nullptr)
    {
        result = buffer;
        // 移除末尾的换行符
        if (!result.empty() && result.back() == '\n')
        {
            result.pop_back();
        }
    }

    pclose(pipe);

    // 验证找到的路径是否可执行
    if (!result.empty() && access(result.c_str(), X_OK) == 0)
    {
        return result;
    }

    return "";
}

std::vector<std::pair<std::string, std::string>> Editor::getAvailableEditors()
{
    std::vector<std::pair<std::string, std::string>> available;
    for (const auto& name : COMMON_EDITORS)
    {
        std::string path = findInPath(name);
        if (!path.empty())
        {
            available.emplace_back(name, path);
        }
    }
    return available;
}

void Editor::printMenu(
    const std::vector<std::pair<std::string, std::string>>& editors)
{
    std::cerr << "\nAvailable editors on your system:\n";
    for (size_t i = 0; i < editors.size(); ++i)
    {
        std::cerr << "  " << (i + 1) << ". " << editors[i].first << " ("
                  << editors[i].second << ")\n";
    }
    std::cerr << "  0. Enter a custom editor command\n";
}

std::string Editor::getUserChoice(
    const std::vector<std::pair<std::string, std::string>>& editors)
{
    while (true)
    {
        std::cerr << "\nEnter the number of your choice (or 0 for custom): ";
        std::string input;
        std::getline(std::cin, input);

        // 去除首尾空格
        input.erase(0, input.find_first_not_of(" \t\n\r"));
        input.erase(input.find_last_not_of(" \t\n\r") + 1);

        if (input.empty())
            continue;

        // 尝试解析为数字
        char* end;
        long choice = strtol(input.c_str(), &end, 10);
        if (*end != '\0')
        {
            std::cerr << "Please enter a valid number.\n";
            continue;
        }

        if (choice == 0)
        {
            std::cerr
                << "Enter the editor command (e.g., 'hx' or '/usr/bin/nvim'): ";
            std::string custom;
            std::getline(std::cin, custom);
            // 去除空格
            custom.erase(0, custom.find_first_not_of(" \t\n\r"));
            custom.erase(custom.find_last_not_of(" \t\n\r") + 1);
            if (custom.empty())
            {
                std::cerr
                    << "No custom command entered. Please choose again.\n";
                continue;
            }
            // 可选：检查是否存在，但即使不存在也允许（用户可能期望后续安装）
            std::string resolved = findInPath(custom);
            if (!resolved.empty())
            {
                return resolved;  // 返回完整路径
            }
            else if (custom.find('/') != std::string::npos &&
                     access(custom.c_str(), X_OK) == 0)
            {
                return custom;  // 直接输入的绝对路径
            }
            else
            {
                // 无法验证，但接受输入
                std::cerr
                    << Colors::YELLOW << "Warning: '" << Colors::RESET << custom
                    << "' does not seem to be executable. Using it anyway.\n";
                return custom;
            }
        }
        else if (choice >= 1 && static_cast<size_t>(choice) <= editors.size())
        {
            return editors[choice - 1].second;  // 返回路径
        }
        else
        {
            std::cerr << "Invalid number. Please try again.\n";
        }
    }
}

std::string Editor::getEditor() { return config["editor"].asString(); }

// 设置 editor 配置
void Editor::setEditor(const std::string& newEditor)
{
    config["editor"] = newEditor;
    file.saveConfig(config);
}
