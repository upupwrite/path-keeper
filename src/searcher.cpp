#include "searcher.h"

#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include "colors.h"
#include "commandbuilder.h"

static std::string generateList(const Json::Value& config, File& file)
{
    std::ostringstream oss;
    Json::Value paths = config["path"];
    std::vector<std::string> valid = file.get_valid_directories(paths);
    int i = 1;
    for (const auto& dir : valid)
    {
        Json::Value cmds = paths[dir]["cmds"];
        for (Json::ArrayIndex j = 0; j < cmds.size(); ++j)
        {
            std::string cmd = cmds[j].asString();
            std::string firstLine = cmd.substr(0, cmd.find('\n'));
            oss << i << "." << (j + 1) << "\t" << dir << "\t" << firstLine
                << "\n";
        }
        i++;
    }
    return oss.str();
}

// 简单解析 "5.1" 这类索引，返回目录显示号和命令显示号
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

std::string Searcher::interactiveSearch(const Json::Value& config, File& file)
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
            std::cerr << "无法创建临时文件" << std::endl;
            return "";
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
            std::cerr << "无法运行 fzf" << std::endl;
            return "";
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
        std::cerr << Colors::CYAN << "可用命令列表:\n" << Colors::RESET;
        std::cerr << list;
        std::cerr << Colors::CYAN
                  << "请输入要执行的编号 (例如 5.1): " << Colors::RESET;
        std::getline(std::cin, selectedLine);
    }

    if (selectedLine.empty())
        return "";

    int dirDisp = 0, cmdDisp = 0;
    if (!parseSelection(selectedLine, dirDisp, cmdDisp))
    {
        std::cerr << Colors::RED << "无效的选择格式" << Colors::RESET
                  << std::endl;
        return "";
    }

    // 根据显示号定位目录和命令
    Json::Value paths = config["path"];
    std::vector<std::string> validDirs = file.get_valid_directories(paths);
    if (dirDisp < 1 || dirDisp > static_cast<int>(validDirs.size()))
    {
        std::cerr << Colors::RED << "目录编号超出范围" << Colors::RESET
                  << std::endl;
        return "";
    }
    std::string directory = validDirs[dirDisp - 1];
    Json::Value cmds = paths[directory]["cmds"];
    if (cmdDisp < 1 || cmdDisp > static_cast<int>(cmds.size()))
    {
        std::cerr << Colors::RED << "命令编号超出范围" << Colors::RESET
                  << std::endl;
        return "";
    }
    std::string command = cmds[cmdDisp - 1].asString();

    // 生成最终可执行命令
    Logger logger(config);
    return CommandBuilder::build(directory, command, logger);
}
