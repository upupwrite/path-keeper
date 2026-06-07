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

#include <pwd.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

class ReadlineHelper
{
public:
    // 初始化 readline 设置
    static void initialize()
    {
        // 设置程序名称（用于 .inputrc 配置）
        rl_readline_name = "PathKeeper";

        // 启用彩色补全（readline 8.0+）
#ifdef RL_STATE_COLOR
        rl_colored_stats = 1;              // 文件类型颜色
        rl_colored_completion_prefix = 1;  // 公共前缀颜色
#endif

        // 将 Tab 绑定到菜单补全（支持循环选择）
        rl_bind_key('\t', rl_menu_complete);

        // 设置自定义补全函数
        rl_attempted_completion_function = path_keeper_completion;
    }

    // 读取一行输入（带补全）
    static std::string read_line(const std::string& prompt)
    {
        char* line = readline(prompt.c_str());
        if (!line)
        {
            return "";
        }

        std::string result(line);
        free(line);
        return result;
    }

    // 获取系统命令列表（同原有实现）
    static std::vector<std::string> get_system_commands()
    {
        std::vector<std::string> commands;

        static const char* common_commands[] = {
            "ls",        "cd",         "pwd",   "mkdir", "rm",   "cp",
            "mv",        "cat",        "less",  "more",  "grep", "find",
            "head",      "tail",       "vim",   "nano",  "echo", "printf",
            "chmod",     "chown",      "ps",    "kill",  "top",  "htop",
            "git",       "docker",     "ssh",   "scp",   "tar",  "gzip",
            "gunzip",    "make",       "cmake", "g++",   "gcc",  "python",
            "python3",   "node",       "npm",   "yarn",  "java", "javac",
            "systemctl", "journalctl", "apt",   "yum",   "dnf",  "brew"};

        commands.insert(commands.end(), std::begin(common_commands),
                        std::end(common_commands));

        char* path_env = getenv("PATH");
        if (path_env)
        {
            std::string path_str(path_env);
            std::stringstream ss(path_str);
            std::string dir;

            while (std::getline(ss, dir, ':'))
            {
                try
                {
                    if (fs::exists(dir) && fs::is_directory(dir))
                    {
                        for (const auto& entry : fs::directory_iterator(dir))
                        {
                            if (entry.is_regular_file() &&
                                access(entry.path().c_str(), X_OK) == 0)
                            {
                                commands.push_back(
                                    entry.path().filename().string());
                            }
                        }
                    }
                }
                catch (...)
                {
                    // 忽略无法访问的目录
                }
            }
        }

        std::sort(commands.begin(), commands.end());
        commands.erase(std::unique(commands.begin(), commands.end()),
                       commands.end());

        return commands;
    }

private:
    // 命令补全生成器（同原有实现）
    static char* command_generator(const char* text, int state)
    {
        static std::vector<std::string> commands;
        static size_t list_index = 0;
        static size_t text_len = 0;

        if (state == 0)
        {
            commands = get_system_commands();
            list_index = 0;
            text_len = strlen(text);
        }

        while (list_index < commands.size())
        {
            const std::string& cmd = commands[list_index++];
            if (strncmp(cmd.c_str(), text, text_len) == 0)
            {
                return strdup(cmd.c_str());
            }
        }

        return nullptr;
    }

    // 文件名补全生成器（使用 readline 内置函数）
    static char* filename_generator(const char* text, int state)
    {
        static char** matches = nullptr;
        static int match_index = 0;

        if (state == 0)
        {
            if (matches)
            {
                for (char** p = matches; *p; p++)
                {
                    free(*p);
                }
                free(matches);
            }
            matches =
                rl_completion_matches(text, rl_filename_completion_function);
            match_index = 0;
        }

        if (matches && matches[match_index])
        {
            return strdup(matches[match_index++]);
        }

        return nullptr;
    }

    // 主补全函数：智能识别命令/路径
    static char** path_keeper_completion(const char* text, int start, int end)
    {
        (void)end;
        rl_attempted_completion_over = 1;

        // 行首位置：可能是命令，也可能是路径（包含 '/'）
        if (start == 0)
        {
            // 如果输入中包含 '/'，则当作路径补全
            if (strchr(text, '/') != nullptr)
            {
                return rl_completion_matches(text, filename_generator);
            }
            else
            {
                return rl_completion_matches(text, command_generator);
            }
        }
        else
        {
            // 参数位置：始终使用文件名补全
            return rl_completion_matches(text, filename_generator);
        }
    }
};
