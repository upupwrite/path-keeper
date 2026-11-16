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

#include <qcoreapplication.h>
#include <qtranslator.h>

#include <QCoreApplication>
#include <algorithm>
#include <cctype>
#include <cstdlib>  // getenv
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cstdio>
#include <unistd.h>
#include <string>
#include <vector>

#include "colors.h"
#include "json/value.h"
#include "readline.h"
#include "search.h"
namespace fs = std::filesystem;

namespace
{
std::vector<std::string> splitCommandLine(const std::string& command)
{
    std::vector<std::string> tokens;
    std::string current;
    bool in_single_quote = false;
    bool in_double_quote = false;

    for (std::size_t i = 0; i < command.size(); ++i)
    {
        const char ch = command[i];

        if (ch == '\\' && !in_single_quote)
        {
            if (i + 1 < command.size())
            {
                current.push_back(command[++i]);
            }
            else
            {
                current.push_back(ch);
            }
            continue;
        }

        if (ch == '\'' && !in_double_quote)
        {
            in_single_quote = !in_single_quote;
            continue;
        }

        if (ch == '"' && !in_single_quote)
        {
            in_double_quote = !in_double_quote;
            continue;
        }

        if (std::isspace(static_cast<unsigned char>(ch)) &&
            !in_single_quote && !in_double_quote)
        {
            if (!current.empty())
            {
                tokens.push_back(current);
                current.clear();
            }
            continue;
        }

        current.push_back(ch);
    }

    if (!current.empty())
    {
        tokens.push_back(current);
    }

    return tokens;
}

int execCommandDirect(const std::string& command,
                      const std::string& workingDirectory)
{
    std::vector<std::string> argv = splitCommandLine(command);
    if (argv.empty())
    {
        return 127;
    }

    if (!workingDirectory.empty() && chdir(workingDirectory.c_str()) != 0)
    {
        return 127;
    }

    std::vector<char*> cargs;
    cargs.reserve(argv.size() + 1);
    for (auto& arg : argv)
    {
        cargs.push_back(arg.data());
    }
    cargs.push_back(nullptr);

    execvp(cargs[0], cargs.data());
    return 127;
}
}  // namespace

// 归一化路径：展开 ~，转为绝对路径，去除尾部斜杠
static std::string normalizePath(const std::string& input,
                                  const std::string& cwd)
{
    std::string path = input;
    // 展开 ~
    if (!path.empty() && path[0] == '~')
    {
        const char* home = getenv("HOME");
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

// Launch the configured editor and return the edited buffer.
std::string PathKeeper::runEditorForCommands()
{
    Editor editor;
    const std::string editorCommand = editor.getEditor();
    if (editorCommand.empty()) {
        std::cerr << Colors::RED << "No usable system editor found. Set VISUAL/EDITOR or run 'pk config -editor <command>'." << Colors::RESET << std::endl;
        return {};
    }
    char temporaryPath[] = "/tmp/path-keeper-edit-XXXXXX";
    const int descriptor = mkstemp(temporaryPath);
    if (descriptor < 0) { std::cerr << Colors::RED << "Unable to create editor temporary file." << Colors::RESET << std::endl; return {}; }
    close(descriptor);

    pid_t pid = fork();
    if (pid == 0)
    {
        std::vector<std::string> argv = splitCommandLine(editorCommand);
        argv.push_back(temporaryPath);
        std::vector<char*> cargs;
        cargs.reserve(argv.size() + 1);
        for (auto& arg : argv)
        {
            cargs.push_back(arg.data());
        }
        cargs.push_back(nullptr);

        execvp(cargs[0], cargs.data());
        _exit(127);
    }

    int status = 0;
    waitpid(pid, &status, 0);

    std::string content;
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
    {
        std::ifstream input(temporaryPath, std::ios::binary);
        if (input) content.assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
    }

    std::remove(temporaryPath);
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
    {
        std::cerr << Colors::RED << "Editor exited unsuccessfully." << Colors::RESET << std::endl;
    }
    return content;
}

void PathKeeper::addRecord(bool use_editor)
{
    Json::Value config = file.loadConfig();

    // ---- 1. 目录提示（两种模式共用） ---------------------------------
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

    // 若该目录下已有记录，先展示一遍，方便用户参考
    Json::Value commands = Json::arrayValue;
    if (config["path"].isMember(directory))
    {
        std::cerr << Colors::GREEN << directory << Colors::RESET << std::endl;
        commands = config["path"][directory];
        displayCommands(commands);
    }

    // ---- 2. 收集新命令 -----------------------------------------------
    if (use_editor)
    {
        // --edit：把编辑器整个缓冲区内容作为一条命令写入
        std::string content = runEditorForCommands();
        if (content.empty())
        {
            std::cerr << Colors::YELLOW
                      << QCoreApplication::translate(
                             "addRecord", "编辑器未返回任何内容,已取消!")
                             .toStdString()
                      << Colors::RESET << std::endl;
            return;
        }

        Json::Value newCmd;
        newCmd["cmd"] = content;
        newCmd["hash"] = file.computeHash(directory + content);
        commands.append(newCmd);

        std::cerr << Colors::GREEN
                  << QCoreApplication::translate("addRecord",
                                                 "已从编辑器读取内容")
                         .toStdString()
                  << Colors::RESET << std::endl;
    }
    else
    {
        // 原有 readline 行为不变
        std::string cmd = ReadlineHelper::read_line(
            Colors::CYAN +
            QCoreApplication::translate("addRecord", "请输入命令:")
                .toStdString() +
            Colors::RESET);

        if (cmd.empty() && commands.empty())
        {
            std::string default_command = "ls -l";
            std::cerr << QCoreApplication::translate("addRecord",
                                                     "使用默认命令: ")
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
    }

    // ---- 3. 写回配置 -------------------------------------------------
    config["path"][directory] = commands;
    file.saveConfig(config);

    std::cerr
        << Colors::GREEN
        << QCoreApplication::translate("addRecord", "记录已保存!").toStdString()
        << Colors::RESET << std::endl;
}
