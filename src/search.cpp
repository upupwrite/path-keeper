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

#include "search.h"

#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include <QCoreApplication>

#include "colors.h"
#include "loadfile.h"

// 生成可搜索列表，格式：目录显示号.命令显示号\t目录\t命令首行（完整命令，换行符替换为空格）
static std::string generateList(const Json::Value& config, File& file)
{
    std::ostringstream oss;
    Json::Value paths = config["path"];
    std::vector<std::string> valid = file.get_valid_directories(paths);
    int i = 1;
    for (const auto& dir : valid)
    {
        Json::Value cmds = paths[dir];
        if (!cmds.isArray()) continue;

        for (Json::ArrayIndex j = 0; j < cmds.size(); ++j)
        {
            // 获取命令原始字符串（用于显示）
            std::string cmdStr;
            if (cmds[j].isObject() && cmds[j].isMember("cmd"))
                cmdStr = cmds[j]["cmd"].asString();
            else if (cmds[j].isString())
                cmdStr = cmds[j].asString();  // 兼容旧格式
            else
                cmdStr = "???";

            // 替换换行符为空格，以便显示完整命令在一行
            std::string displayCmd = cmdStr;
            std::replace(displayCmd.begin(), displayCmd.end(), '\n', ' ');
            // 去掉末尾多余的空格
            while (!displayCmd.empty() && displayCmd.back() == ' ')
                displayCmd.pop_back();

            oss << i << "." << (j + 1) << "\t" << dir << "\t" << displayCmd << "\n";
        }
        i++;
    }
    return oss.str();
}

// 解析 "5.1" 这类索引，返回目录显示号和命令显示号
static bool parseSelection(const std::string& line, int& dirDisp, int& cmdDisp)
{
    // 期望格式：行首是 "5.1" 后跟 tab
    auto tabPos = line.find('\t');
    std::string indexPart =
        (tabPos != std::string::npos) ? line.substr(0, tabPos) : line;
    auto dotPos = indexPart.find('.');
    if (dotPos == std::string::npos)
        return false;
    try
    {
        dirDisp = std::stoi(indexPart.substr(0, dotPos));
        cmdDisp = std::stoi(indexPart.substr(dotPos + 1));
        return true;
    }
    catch (...)
    {
        return false;
    }
}

SearchResult Searcher::interactiveSearch(const Json::Value& config, File& file)
{
    // 确保 key order 已加载
    file.load_key_order();
    std::string list = generateList(config, file);

    // 尝试使用 fzf
    bool useFzf = (system("which fzf > /dev/null 2>&1") == 0);
    std::string selectedLine;

    if (useFzf)
    {
        // 写入临时文件
        char tmpname[] = "/tmp/pk_fzf_XXXXXX";
        int fd = mkstemp(tmpname);
        if (fd == -1)
        {
            std::cerr << Colors::RED
                      << QCoreApplication::translate("Searcher", "无法创建临时文件").toStdString()
                      << Colors::RESET << std::endl;
            return SearchResult();
        }
        write(fd, list.c_str(), list.size());
        close(fd);

        // 调用 fzf，结果通过 popen 捕获
        std::string cmd =
            std::string("fzf --delimiter='\t' --with-nth=2,3 < ") + tmpname;
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe)
        {
            unlink(tmpname);
            std::cerr << Colors::RED
                      << QCoreApplication::translate("Searcher", "无法运行 fzf").toStdString()
                      << Colors::RESET << std::endl;
            return SearchResult();
        }

        char buffer[1024];
        if (fgets(buffer, sizeof(buffer), pipe) != nullptr)
        {
            selectedLine = buffer;
            // 去掉末尾换行
            if (!selectedLine.empty() && selectedLine.back() == '\n')
                selectedLine.pop_back();
        }
        pclose(pipe);
        unlink(tmpname);
    }
    else
    {
        // 回退：显示列表并让用户输入索引
        std::cerr << Colors::CYAN
                  << QCoreApplication::translate("Searcher", "可用命令列表:\n").toStdString()
                  << Colors::RESET;
        std::cerr << list;
        std::cerr << Colors::CYAN
                  << QCoreApplication::translate("Searcher", "请输入要执行的编号 (例如 5.1): ").toStdString()
                  << Colors::RESET;
        std::getline(std::cin, selectedLine);
    }

    if (selectedLine.empty())
        return SearchResult();

    int dirDisp = 0, cmdDisp = 0;
    if (!parseSelection(selectedLine, dirDisp, cmdDisp))
    {
        std::cerr << Colors::RED
                  << QCoreApplication::translate("Searcher", "无效的选择格式").toStdString()
                  << Colors::RESET << std::endl;
        return SearchResult();
    }

    // 根据显示号定位目录和命令
    Json::Value paths = config["path"];
    std::vector<std::string> validDirs = file.get_valid_directories(paths);
    if (dirDisp < 1 || dirDisp > static_cast<int>(validDirs.size()))
    {
        std::cerr << Colors::RED
                  << QCoreApplication::translate("Searcher", "目录编号超出范围").toStdString()
                  << Colors::RESET << std::endl;
        return SearchResult();
    }
    std::string directory = validDirs[dirDisp - 1];
    int cmdIndex = cmdDisp - 1;

    // 使用 File 类的新接口获取实际执行的命令（支持别名）
    std::string command = file.getEffectiveCommand(directory, cmdIndex);
    if (command.empty())
    {
        std::cerr << Colors::RED
                  << QCoreApplication::translate("Searcher", "命令无效或不存在").toStdString()
                  << Colors::RESET << std::endl;
        return SearchResult();
    }

    return SearchResult(directory, cmdIndex, command);
}
