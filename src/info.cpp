#include "info.h"

#include <iostream>

#include "colors.h"

const std::string VERSION = "2.5.8";

void showVersion(bool verbose)
{
    std::cerr << "path-keeper v" << VERSION << "\n";
    std::cerr << "Copyright (C) 2023-2026 path-keeper Inc.\n";
    std::cerr << "License: GPL-3.0-or-later\n\n";

    if (verbose)
    {
        std::cerr << "Build Information:\n";
        std::cerr << "  Build date:   " << BUILD_DATE << "\n";
        std::cerr << "  Compiler:     " << COMPILER << "\n";
        std::cerr << "  Architecture: "
                  << (sizeof(void*) == 8 ? "64-bit" : "32-bit") << "\n\n";
    }

    std::cerr << "This program is free software: you can redistribute it "
                 "and/or modify\n";
    std::cerr << "it under the terms of the GNU General Public License as "
                 "published by\n";
    std::cerr << "the Free Software Foundation, either version 3 of the "
                 "License, or\n";
    std::cerr << "(at your option) any later version.\n\n";

    std::cerr << "Written by upupwrite\n";
    std::cerr << "Home page: <https://gitee.com/upupwrite/path-keeper>\n";
}

void showHelp()
{
    std::cerr << "path-keeper Help\n"
              << "Usage: pk <option|subcommand> [args]\n\n"
              << "Main options:\n"
              << "  -a, --add           "
                 "添加命令记录（支持多行：按回车后选择编辑器模式）\n"
              << "  -e, --execute [idx] 执行命令（可选直接指定编号，如 2.1）\n"
              << "  -p, --point [idx]   执行命令但不更新 recent 记录\n"
              << "  -s, --show          显示所有记录\n"
              << "  -c, --configure     设置 recent 记录\n"
              << "  -v, --version       显示版本信息\n"
              << "  -V, --version-verbose 显示详细版本与构建信息\n"
              << "  -h, --help          显示此帮助\n\n"
              << "Subcommands:\n"
              << "  config              打开编辑器设置\n"
              << "  config -editor [cmd] 设置默认编辑器（无参数时交互选择）\n"
              << "  alias add <name> <index>  添加别名\n"
              << "  alias remove <name>       删除别名\n"
              << "  alias list                列出所有别名\n"
              << "  alias install             生成 shell 别名文件 "
                 "(~/.pk_aliases.sh)\n"
              << "  search              使用 fzf 交互式搜索并执行命令\n"
              << "  verify              校验命令哈希值\n"
              << "  rehash              重新生成并保存哈希值\n"
              << "  log                 列出日志文件\n\n"
              << "Log control per command:\n"
              << "  "
                 "添加命令时可选择单独日志记录（y=强制记录/n=强制不记录/"
                 "回车=跟随全局）\n"
              << "  也可在配置文件中将命令改为 {\"cmd\":\"...\", "
                 "\"log\":true/false}\n"
              << "Global log settings are in ~/.pk.json under \"log\" key.\n"
              << std::endl;
}