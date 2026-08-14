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

#include "pk.h"

#include <QCoreApplication>
#include <algorithm>
#include <cstdlib>  // getenv
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "colors.h"
#include "json/value.h"
#include "readline.h"
#include "search.h"
namespace fs = std::filesystem;

// 归一化路径：展开 ~，转为绝对路径，去除尾部斜杠
static std::string normalizePath(const std::string &input,
                                 const std::string &cwd)
{
    std::string path = input;
    // 展开 ~
    if (!path.empty() && path[0] == '~')
    {
        const char *home = getenv("HOME");
        if (home)
        {
            path = std::string(home) + path.substr(1);
        }
    }
    // 构建绝对路径
    fs::path absPath;
    if (path.empty() || path[0] != '/')
    {
        absPath = fs::path(cwd) / path;
    }
    else
    {
        absPath = fs::path(path);
    }
    // 规范化（解析 . 和 ..，不要求路径存在）
    try
    {
        fs::path canonical = fs::weakly_canonical(absPath);
        std::string result = canonical.string();
        // 移除尾部斜杠（根目录除外）
        while (result.size() > 1 && result.back() == '/')
        {
            result.pop_back();
        }
        return result;
    }
    catch (...)
    {
        // 若规范化失败，返回原始输入（尽力而为）
        return path;
    }
}

PathKeeper::PathKeeper()
{
    // Get current working directory
    char buffer[1024];
    if (getcwd(buffer, sizeof(buffer)) != nullptr)
    {
        cwd = buffer;
    }
    else
    {
        cwd = ".";
    }

    file.load_key_order();
}

void PathKeeper::addRecord()
{
    Json::Value config = file.loadConfig();
    static bool initialized = false;
    if (!initialized)
    {
        ReadlineHelper::initialize();
        initialized = true;
    }

    std::string directory = ReadlineHelper::read_line(
        Colors::CYAN +
        QCoreApplication::translate("addRecord", "请输入记录目录:")
            .toStdString() +
        Colors::RESET);

    if (directory.empty())
    {
        std::cerr << Colors::YELLOW
                  << QCoreApplication::translate("addRecord", "目录不能为空!")
                         .toStdString()
                  << Colors::RESET << std::endl;
        return;
    }

    // 归一化为绝对路径（统一格式）
    directory = normalizePath(directory, cwd);

    // 检查配置中是否已有该目录
    Json::Value commands = Json::arrayValue;
    if (config["path"].isMember(directory))
    {
        std::cerr << Colors::GREEN << directory << Colors::RESET << std::endl;
        commands = config["path"][directory];
        displayCommands(commands);
    }

    std::string cmd = ReadlineHelper::read_line(
        Colors::CYAN +
        QCoreApplication::translate("addRecord", "请输入命令:").toStdString() +
        Colors::RESET);

    if (cmd.empty() && commands.empty())
    {
        std::string default_command = "ls -l";
        std::cerr << QCoreApplication::translate("addRecord", "使用默认命令: ")
                         .toStdString()
                  << default_command << std::endl;
        Json::Value newCmd;
        newCmd["cmd"] = default_command;
        newCmd["hash"] = file.computeHash(directory + default_command);
        commands.append(newCmd);
    }
    else if (!cmd.empty())
    {
        Json::Value newCmd;
        newCmd["cmd"] = cmd;
        newCmd["hash"] = file.computeHash(directory + cmd);
        commands.append(newCmd);
    }

    config["path"][directory] = commands;
    file.saveConfig(config);

    std::cerr
        << Colors::GREEN
        << QCoreApplication::translate("addRecord", "记录已保存!").toStdString()
        << Colors::RESET << std::endl;
}

void PathKeeper::runRecent()
{
    Json::Value config = file.loadConfig();

    if (config["recent"].isNull())
    {
        std::cerr << Colors::YELLOW
                  << QCoreApplication::translate("runRecent", "没有最近记录!")
                         .toStdString()
                  << Colors::RESET << std::endl;
        return;
    }

    if (!config["recent"].isArray() || config["recent"].size() != 2)
    {
        std::cerr << Colors::RED
                  << QCoreApplication::translate("runRecent",
                                                 "最近记录格式无效!")
                         .toStdString()
                  << Colors::RESET << std::endl;
        return;
    }

    try
    {
        int orig_idx1 = config["recent"][0].asInt();
        int idx2 = config["recent"][1].asInt();
        Json::Value paths = config["path"];

        // Check using the original index
        if (orig_idx1 < 0 ||
            orig_idx1 >= static_cast<int>(file.path_keys_order.size()))
        {
            std::cerr << Colors::RED
                      << QCoreApplication::translate("runRecent",
                                                     "最近记录无效!")
                             .toStdString()
                      << Colors::RESET << std::endl;
            return;
        }

        std::string directory = file.path_keys_order[orig_idx1];
        if (!paths.isMember(directory))
        {
            std::cerr << Colors::RED
                      << QCoreApplication::translate(
                             "runRecent", "最近记录对应的目录已不存在!")
                             .toStdString()
                      << Colors::RESET << std::endl;
            return;
        }

        std::string command = file.getEffectiveCommand(directory, idx2);
        if (command.empty())
        {
            std::cerr << Colors::RED
                      << QCoreApplication::translate("runRecent", "命令无效!")
                             .toStdString()
                      << Colors::RESET << std::endl;
            return;
        }
        std::string extra;

        runCommand(directory, command, extra);
    }
    catch (const std::exception &)
    {
        std::cerr << Colors::RED
                  << QCoreApplication::translate("runRecent", "最近记录无效!")
                         .toStdString()
                  << Colors::RESET << std::endl;
    }
}

Json::Value PathKeeper::showRecord(const bool& show)
{
    Json::Value config = file.loadConfig();
    Json::Value paths = config["path"];
    if (paths.empty())
    {
        std::cerr << Colors::YELLOW
                  << QCoreApplication::translate("showRecord", "没有记录!")
                         .toStdString()
                  << Colors::RESET << std::endl;
        return Json::Value();
    }
    if (show)
    {
        int i = 1;
        std::vector<std::string> valid_dirs = file.get_valid_directories(paths);

        for (const auto &directory : valid_dirs)
        {
            std::cerr << Colors::BLUE << "[" << i << "]" << Colors::RESET << " "
                      << Colors::GREEN << directory << Colors::RESET
                      << std::endl;

            Json::Value commands = paths[directory];

            displayCommands(commands, i);

            i++;
        }
    }

    displayRecentMark(config, paths);
    return config;
}

void PathKeeper::runCommand(const std::string &directory,
                            const std::string &command,
                            const std::string &extra)
{
    std::string full_command = command;
    if (!extra.empty()) {
        full_command += " " + extra;
    }

    std::cerr
        << Colors::GREEN
        << QCoreApplication::translate("runCommand", "目录:").toStdString()
        << Colors::RESET << " " << directory << std::endl;
    std::cerr
        << Colors::GREEN
        << QCoreApplication::translate("runCommand", "命令:").toStdString()
        << Colors::RESET << " " << Colors::CYAN << full_command << Colors::RESET
        << std::endl;

    if (access(directory.c_str(), F_OK) != 0)
    {
        std::cerr << Colors::RED
                  << QCoreApplication::translate("runCommand",
                                                 "目标目录不存在!")
                         .toStdString()
                  << Colors::RESET << std::endl;
        return;
    }

    int idx = file.getCommandIndex(directory, command);
    if (file.verifyCommandHash(directory, idx))
    {
    RUN:
        if (file.isGlobalLogEnabled())
        {
            if (file.isCommandLogEnabled(directory, idx))
            {
                std::cerr << Colors::YELLOW
                          << QCoreApplication::translate("runCommand",
                                                         "执行命令")
                                 .toStdString()
                          << QCoreApplication::translate("runCommand",
                                                         "(录制中): ")
                                 .toStdString()
                          << full_command << Colors::RESET << std::endl;
                shell.shellCommand(full_command, directory, true);
            }
            else
            {
                std::cerr << Colors::YELLOW
                          << QCoreApplication::translate("runCommand",
                                                         "执行命令: ")
                                 .toStdString()
                          << full_command << Colors::RESET << std::endl;
                shell.shellCommand(full_command, directory);
            }
        }
        else
        {
            std::cerr << Colors::YELLOW
                      << QCoreApplication::translate("runCommand", "执行命令: ")
                             .toStdString()
                      << full_command << Colors::RESET << std::endl;
            shell.shellCommand(full_command, directory);
        }
    }
    else
    {
        std::string select;
        std::cerr << Colors::RED
                  << QCoreApplication::translate(
                         "runCommand",
                         "这个命令似乎被更改过,是否信任执行(Y/n): ")
                         .toStdString()
                  << Colors::RESET;
        std::getline(std::cin, select);
        if (select == "Y" || select == "y")
        {
            file.syncCommandHash(directory, idx);
            goto RUN;
        }
        else
        {
            return;
        }
    }
}

void PathKeeper::setRecent(const std::string &cmd_index)
{
    Json::Value config = showRecord();
    if (config.isNull())
        return;

    std::string index_str = getInputIndex(
        cmd_index, QCoreApplication::translate("setRecent", "请输入目标编号: ")
                       .toStdString());
    if (index_str.empty())
        return;
    ParseArgs to_args{.index_str=index_str};
    to_args.set_recent=true;
    parseRun(to_args);
}

void PathKeeper::selectRun(const std::string &cmd_index, const bool set_recent,
                           const bool show, const std::string &extra,
                           bool allow_recent_fallback)
{
    Json::Value config = showRecord(show);
    if (config.isNull())
        return;

    Json::Value paths = config["path"];
    if (paths.empty())
    {
        std::cerr << Colors::YELLOW
                  << QCoreApplication::translate("selectRun", "没有记录!")
                         .toStdString()
                  << Colors::RESET << std::endl;
        return;
    }

    std::string index_str = getInputIndex(
        cmd_index,
        QCoreApplication::translate("selectRun", "请输入要执行的编号或别名: ")
            .toStdString());
    if (index_str.empty()) {
        if (allow_recent_fallback) {
            runRecent();
        } else {
            std::cerr << "未提供索引，取消执行" << std::endl;
        }
        return;
    }
    ParseArgs to_args{
        .index_str=index_str,
        .extra=extra
    };
    to_args.set_recent=set_recent;
    to_args.run_command=true;

    parseRun(to_args);
}

void PathKeeper::runPoint(const std::string &cmd_index, const std::string &extra)
{
    bool show = cmd_index.empty();
    selectRun(cmd_index, false, show, extra, false);
}



void PathKeeper::displayCommands(const Json::Value &commands, int parent_index)
{
    for (Json::ArrayIndex j = 0; j < commands.size(); j++)
    {
        // 获取命令的原始 cmd 字符串（用于显示）
        std::string cmd_str;
        if (commands[j].isObject() && commands[j].isMember("cmd"))
            cmd_str = commands[j]["cmd"].asString();
        else if (commands[j].isString())
            cmd_str = commands[j].asString();  // 兼容旧格式
        else
            cmd_str = "???";

        std::string alias_str;
        if (commands[j].isObject() && commands[j].isMember("alias"))
            alias_str = commands[j]["alias"].asString();

        if (parent_index > 0)
        {
            std::cerr << "    " << Colors::CYAN << "[" << parent_index << "."
                      << (j + 1) << "]" << Colors::RESET << " " << cmd_str;
        }
        else
        {
            std::cerr << "    " << Colors::CYAN << "[" << (j + 1) << "]"
                      << Colors::RESET << " " << cmd_str;
        }
        if (!alias_str.empty()) {
            std::cerr << " (" << Colors::YELLOW << alias_str << Colors::RESET << ")";
        }
        std::cerr << std::endl;
    }
}

void PathKeeper::displayRecentMark(const Json::Value &config,
                                   const Json::Value &paths)
{
    if (!config["recent"].isNull() && config["recent"].isArray() &&
        config["recent"].size() == 2)
    {
        int orig_idx1 = config["recent"][0].asInt();
        int idx2 = config["recent"][1].asInt();

        int display_num =
            file.get_display_number_by_directory_index(orig_idx1, paths);
        if (display_num != -1)
        {
            std::string recent_dir = file.path_keys_order[orig_idx1];
            Json::Value commands = paths[recent_dir];
            if (idx2 >= 0 && idx2 < static_cast<int>(commands.size()))
            {
                std::cerr << Colors::YELLOW
                          << QCoreApplication::translate("displayRecentMark",
                                                         "最近执行")
                                 .toStdString()
                          << ": [" << display_num << "." << (idx2 + 1) << "] "
                          << recent_dir << Colors::RESET << std::endl;
            }
        }
    }
}

std::string PathKeeper::getInputIndex(const std::string &provided_index,
                                      const std::string &prompt)
{
    if (!provided_index.empty())
        return provided_index;

    std::cerr << Colors::CYAN << prompt << Colors::RESET;
    std::string index_str;
    std::getline(std::cin, index_str);
    return index_str;
}

// 支持别名作为索引
bool PathKeeper::parseIndex(const std::string &index_str,
                            const Json::Value &paths,
                            std::string &directory,
                            int &cmd_idx)
{
    const auto valid_dirs = file.get_valid_directories(paths);
    // 1. 尝试解析为标准编号（数字 或 数字.数字）
    if (index_str.find('.') != std::string::npos)
    {
        size_t dot_pos = index_str.find('.');
        std::string part1 = index_str.substr(0, dot_pos);
        std::string part2 = index_str.substr(dot_pos + 1);

        try
        {
            int display_num = std::stoi(part1);
            cmd_idx = std::stoi(part2) - 1;

            if (display_num < 1 ||
                display_num > static_cast<int>(valid_dirs.size()))
                return false;

            directory = valid_dirs[display_num - 1];
            Json::Value commands = paths[directory];

            return (cmd_idx >= 0 &&
                    cmd_idx < static_cast<int>(commands.size()));
        }
        catch (const std::exception &)
        {
            return false;
        }
    }
    else if (!index_str.empty() && std::isdigit(index_str[0]))
    {
        // 纯数字索引
        try
        {
            int display_num = std::stoi(index_str);

            if (display_num < 1 ||
                display_num > static_cast<int>(valid_dirs.size()))
                return false;

            directory = valid_dirs[display_num - 1];
            Json::Value commands = paths[directory];

            if (commands.size() > 1)
            {
                std::cerr << Colors::GREEN
                          << QCoreApplication::translate("parseIndex", "目录:")
                                 .toStdString()
                          << Colors::RESET << " " << directory << std::endl;
                displayCommands(commands);

                std::cerr << Colors::CYAN
                          << QCoreApplication::translate("parseIndex",
                                                         "请选择命令编号: ")
                                 .toStdString()
                          << Colors::RESET;
                std::string cmd_idx_str;
                std::getline(std::cin, cmd_idx_str);

                try
                {
                    cmd_idx = std::stoi(cmd_idx_str) - 1;
                    return (cmd_idx >= 0 &&
                            cmd_idx < static_cast<int>(commands.size()));
                }
                catch (const std::exception &)
                {
                    return false;
                }
            }
            else
            {
                cmd_idx = 0;
                return true;
            }
        }
        catch (const std::exception &)
        {
            return false;
        }
    }
    else
    {
        // 2. 尝试作为别名解析
        std::vector<std::string> dirNames = paths.getMemberNames();
        for (const auto &dirName : dirNames)
        {
            const Json::Value &cmds = paths[dirName];
            if (!cmds.isArray()) continue;
            for (Json::ArrayIndex i = 0; i < cmds.size(); ++i)
            {
                if (cmds[i].isObject() && cmds[i].isMember("alias") &&
                    cmds[i]["alias"].asString() == index_str)
                {
                    directory = dirName;
                    cmd_idx = i;
                    if (std::find(valid_dirs.begin(), valid_dirs.end(), directory) != valid_dirs.end())
                    {
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }
            }
        }
        return false;
    }
}

void PathKeeper::saveRecentRecord(Json::Value &config,
                                  const std::string &directory, int cmd_idx)
{
    int orig_idx = -1;
    for (size_t i = 0; i < file.path_keys_order.size(); i++)
    {
        if (file.path_keys_order[i] == directory)
        {
            orig_idx = i;
            break;
        }
    }

    if (orig_idx != -1)
    {
        Json::Value recent(Json::arrayValue);
        recent.append(orig_idx);
        recent.append(cmd_idx);
        config["recent"] = recent;
        file.saveConfig(config);
    }
}

void PathKeeper::search()
{
    Json::Value config = file.loadConfig();
    std::string extra;
    if (config.isNull() || config["path"].empty())
    {
        std::cerr << Colors::YELLOW
                  << QCoreApplication::translate(
                         "runSearch", "没有记录，请先添加目录和命令。")
                         .toStdString()
                  << Colors::RESET << std::endl;
        return;
    }

    SearchResult result = Searcher::interactiveSearch(config, file);
    if (!result.valid)
    {
        std::cerr << Colors::YELLOW
                  << QCoreApplication::translate("runSearch",
                                                 "未选择任何命令。")
                         .toStdString()
                  << Colors::RESET << std::endl;
        return;
    }

    // 保存最近记录
    int origIdx = -1;
    for (size_t i = 0; i < file.path_keys_order.size(); ++i)
    {
        if (file.path_keys_order[i] == result.directory)
        {
            origIdx = i;
            break;
        }
    }
    if (origIdx != -1)
    {
        Json::Value recent(Json::arrayValue);
        recent.append(origIdx);
        recent.append(result.commandIndex);
        config["recent"] = recent;
        file.saveConfig(config);
    }

    runCommand(result.directory, result.effectiveCommand, extra);
}

// ===================== 实现 alias 功能 =====================

void PathKeeper::addAlias(const std::string &name, const std::string &indexStr)
{
    Json::Value config = file.loadConfig();
    Json::Value paths = config["path"];
    std::string directory;
    int cmd_idx;
    if (!parseIndex(indexStr, paths, directory, cmd_idx)) {
        std::cerr << Colors::RED << "无效索引: " << indexStr << Colors::RESET << std::endl;
        return;
    }
    file.setCommandAlias(directory, cmd_idx, name);
    std::cerr << Colors::GREEN << "别名 " << name << " 已添加到 " << indexStr << Colors::RESET << std::endl;
}

void PathKeeper::removeAlias(const std::string &name)
{
    Json::Value config = file.loadConfig();
    Json::Value paths = config["path"];
    bool found = false;
    for (const auto &dir : paths.getMemberNames()) {
        Json::Value &cmds = paths[dir];
        if (!cmds.isArray()) continue;
        for (Json::ArrayIndex i = 0; i < cmds.size(); ++i) {
            if (cmds[i].isObject() && cmds[i].isMember("alias") && cmds[i]["alias"].asString() == name) {
                // 使用 setCommandAlias 删除，保证保存
                file.setCommandAlias(dir, i, "");
                found = true;
                break;
            }
        }
        if (found) break;
    }
    if (found) {
        std::cerr << Colors::GREEN << "别名 " << name << " 已删除" << Colors::RESET << std::endl;
    } else {
        std::cerr << Colors::YELLOW << "别名 " << name << " 未找到" << Colors::RESET << std::endl;
    }
}

void PathKeeper::listAliases()
{
    Json::Value config = file.loadConfig();
    Json::Value paths = config["path"];
    std::vector<std::string> valid_dirs = file.get_valid_directories(paths);
    bool hasAlias = false;
    for (size_t dirIdx = 0; dirIdx < valid_dirs.size(); ++dirIdx) {
        const std::string &dir = valid_dirs[dirIdx];
        Json::Value &cmds = paths[dir];
        if (!cmds.isArray()) continue;
        for (Json::ArrayIndex i = 0; i < cmds.size(); ++i) {
            if (cmds[i].isObject() && cmds[i].isMember("alias")) {
                std::string alias = cmds[i]["alias"].asString();
                std::string cmdStr = cmds[i]["cmd"].asString();
                int displayNum = dirIdx + 1;
                std::cerr << Colors::CYAN << alias << Colors::RESET << " -> "
                          << Colors::GREEN << "[" << displayNum << "." << (i+1) << "] "
                          << cmdStr << Colors::RESET << std::endl;
                hasAlias = true;
            }
        }
    }
    if (!hasAlias) {
        std::cerr << Colors::YELLOW << "没有定义任何别名" << Colors::RESET << std::endl;
    }
}

void PathKeeper::installAliases()
{
    Json::Value config = file.loadConfig();
    Json::Value paths = config["path"];
    std::vector<std::string> valid_dirs = file.get_valid_directories(paths);
    std::string home = getenv("HOME") ? getenv("HOME") : ".";
    std::string aliasFile = home + "/.pk_aliases.sh";
    std::ofstream out(aliasFile);
    if (!out.is_open()) {
        std::cerr << Colors::RED << "无法写入 " << aliasFile << Colors::RESET << std::endl;
        return;
    }
    out << "#!/bin/bash\n";
    out << "# Auto-generated by pk alias install\n";
    for (size_t dirIdx = 0; dirIdx < valid_dirs.size(); ++dirIdx) {
        const std::string &dir = valid_dirs[dirIdx];
        Json::Value &cmds = paths[dir];
        if (!cmds.isArray()) continue;
        for (Json::ArrayIndex i = 0; i < cmds.size(); ++i) {
            if (cmds[i].isObject() && cmds[i].isMember("alias")) {
                std::string alias = cmds[i]["alias"].asString();
                int displayNum = dirIdx + 1;
                out << "alias " << alias << "='pk -e " << displayNum << "." << (i+1) << "'\n";
            }
        }
    }
    out.close();
    std::cerr << Colors::GREEN << "别名已安装到 " << aliasFile << Colors::RESET << std::endl;
}


void PathKeeper::parseRun(const ParseArgs &args_struct){
    const std::string index_str= args_struct.index_str;
    const std::string extra= args_struct.extra;

    const bool show_board=args_struct.show_board;
    const bool run_command=args_struct.run_command;
    const bool set_recent=args_struct.set_recent;



    auto config=showRecord(show_board);
    const auto paths=config["path"];

    if (index_str.empty()) {
        std::cerr << Colors::RED << "索引不能为空" << Colors::RESET << std::endl;
        return;
    }

    std::string directory;
    int cmd_idx;

    if (!parseIndex(index_str, paths, directory, cmd_idx))
    {
        std::cerr << Colors::RED
                  << QCoreApplication::translate("parseRun",
                                                 "无效编号!")
                         .toStdString()
                  << Colors::RESET << std::endl;
        return;
    }

    std::string command = file.getEffectiveCommand(directory, cmd_idx);
    if (command.empty())
    {
        std::cerr << Colors::RED
                  << QCoreApplication::translate("parseRun",
                                                 "命令无效或不存在!")
                         .toStdString()
                  << Colors::RESET << std::endl;
        return;
    }

    if (run_command)
    {
        if (set_recent)
        {
            saveRecentRecord(config, directory, cmd_idx);
        }

        runCommand(directory, command, extra);
    }
    else
    {
        if (set_recent)
        {
            saveRecentRecord(config, directory, cmd_idx);
        }
    }
}



