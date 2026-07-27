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
#include <iostream>
#include <string>
#include <vector>

#include "colors.h"
#include "json/value.h"
#include "readline.h"
#include "search.h"

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
    // 初始化 readline（只需调用一次，建议放在程序启动时）
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

    if (directory == ".")
    {
        directory = cwd;
    }

    // 检查目录是否已存在
    Json::Value commands = Json::arrayValue;

    if (fs::exists(directory) && fs::is_directory(directory))
    {
        std::cerr << Colors::GREEN << directory << Colors::RESET << std::endl;
        commands = config["path"][directory];
        displayCommands(commands);
    }

    // Use readline to read commands (with completion)
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
        // 存储为对象格式
        Json::Value newCmd;
        newCmd["cmd"] = default_command;
        newCmd["hash"] = file.computeHash(directory + cmd);
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

        // 使用新的 getEffectiveCommand 获取实际要执行的命令（支持别名）
        std::string command = file.getEffectiveCommand(directory, idx2);
        if (command.empty())
        {
            std::cerr << Colors::RED
                      << QCoreApplication::translate("runRecent", "命令无效!")
                             .toStdString()
                      << Colors::RESET << std::endl;
            return;
        }

        runCommand(directory, command);
    }
    catch (const std::exception &)
    {
        std::cerr << Colors::RED
                  << QCoreApplication::translate("runRecent", "最近记录无效!")
                         .toStdString()
                  << Colors::RESET << std::endl;
    }
}

Json::Value PathKeeper::showRecord(const bool show)
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
                            const std::string &command)
{
    std::cerr
        << Colors::GREEN
        << QCoreApplication::translate("runCommand", "目录:").toStdString()
        << Colors::RESET << " " << directory << std::endl;
    std::cerr
        << Colors::GREEN
        << QCoreApplication::translate("runCommand", "命令:").toStdString()
        << Colors::RESET << " " << Colors::CYAN << command << Colors::RESET
        << std::endl;

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
                          << command << Colors::RESET << std::endl;
                shell.shellCommand(command, directory, true);
            }
            else
            {
                std::cerr << Colors::YELLOW
                          << QCoreApplication::translate("runCommand",
                                                         "执行命令: ")
                                 .toStdString()
                          << command << Colors::RESET << std::endl;
                shell.shellCommand(command, directory);
            }
        }
        else
        {
            std::cerr << Colors::YELLOW
                      << QCoreApplication::translate("runCommand", "执行命令: ")
                             .toStdString()
                      << command << Colors::RESET << std::endl;
            shell.shellCommand(command, directory);
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

    Json::Value paths = config["path"];
    std::string index_str = getInputIndex(
        cmd_index, QCoreApplication::translate("setRecent", "请输入目标编号: ")
                       .toStdString());
    if (index_str.empty())
        return;

    processIndexSelection(index_str, paths, config, false);
}

void PathKeeper::selectRun(const std::string &cmd_index, const bool set_recent,
                           const bool show)
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
        QCoreApplication::translate("selectRun", "请输入要执行的编号: ")
            .toStdString());
    if (index_str.empty())
    {
        runRecent();
        return;
    }

    processIndexSelection(index_str, paths, config, true, set_recent);
}

void PathKeeper::runPoint(const std::string &cmd_index)
{
    if (cmd_index.empty())
    {
        selectRun(cmd_index, false, true);
    }
    else
    {
        selectRun(cmd_index, false, false);
    }
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

        if (parent_index > 0)
        {
            std::cerr << "    " << Colors::CYAN << "[" << parent_index << "."
                      << (j + 1) << "]" << Colors::RESET << " " << cmd_str
                      << std::endl;
        }
        else
        {
            std::cerr << "    " << Colors::CYAN << "[" << (j + 1) << "]"
                      << Colors::RESET << " " << cmd_str << std::endl;
        }
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

bool PathKeeper::parseIndex(const std::string &index_str,
                            std::vector<std::string> &valid_dirs,
                            Json::Value &paths, std::string &directory,
                            int &cmd_idx)
{
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

            // 验证命令索引是否有效（不一定需要立即获取命令字符串）
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

void PathKeeper::processIndexSelection(const std::string &index_str,
                                       Json::Value &paths, Json::Value &config,
                                       bool execute_command, bool set_recent)
{
    std::vector<std::string> valid_dirs = file.get_valid_directories(paths);
    std::string directory;
    int cmd_idx;

    if (!parseIndex(index_str, valid_dirs, paths, directory, cmd_idx))
    {
        std::cerr << Colors::RED
                  << QCoreApplication::translate("processIndexSelection",
                                                 "无效编号!")
                         .toStdString()
                  << Colors::RESET << std::endl;
        return;
    }

    // 使用新接口获取实际要执行的命令（支持别名）
    std::string command = file.getEffectiveCommand(directory, cmd_idx);
    if (command.empty())
    {
        std::cerr << Colors::RED
                  << QCoreApplication::translate("processIndexSelection",
                                                 "命令无效或不存在!")
                         .toStdString()
                  << Colors::RESET << std::endl;
        return;
    }

    if (execute_command)
    {
        if (set_recent)
        {
            saveRecentRecord(config, directory, cmd_idx);
        }

        runCommand(directory, command);
    }
    else
    {
        if (set_recent)
        {
            saveRecentRecord(config, directory, cmd_idx);
        }

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
            std::cerr << QCoreApplication::translate("processIndexSelection",
                                                     "配置完成")
                             .toStdString()
                      << std::endl;
        }
    }
}

void PathKeeper::search()
{
    Json::Value config = file.loadConfig();
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

    // 执行命令（传递目录、命令字符串和索引，以便记录日志）
    runCommand(result.directory, result.effectiveCommand);
}
