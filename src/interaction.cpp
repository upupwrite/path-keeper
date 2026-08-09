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

#include "interaction.h"

#include <QCoreApplication>
#include <iostream>
#include <string>

#include "colors.h"
#include "info.h"
#include "loadfile.h"

Interaction::Interaction() {}

void Interaction::dir()
{
    std::cerr << QCoreApplication::translate("Interaction", "当前所在的目录:")
                     .toStdString()
              << Colors::BLUE << pk.cwd << Colors::RESET << std::endl;
}

void Interaction::main(int argc, char **argv)
{
    try
    {
        if (argc == 1)
        {
            dir();
            pk.runRecent();
            return;
        }

        std::string option = argv[1];

        if (option == "-a" || option == "--add")
        {
            dir();
            pk.addRecord();
        }
        else if (option == "-s" || option == "--show")
        {
            dir();
            pk.showRecord();
        }
        else if (option == "-p" || option == "--point")
        {
            dir();
            if (argc > 2)
            {
                pk.runPoint(argv[2]);
            }
            else
            {
                pk.runPoint();
            }
        }
        else if (option == "-c" || option == "--configure")
        {
            dir();
            pk.setRecent();
        }
        else if (option == "-e" || option == "--execute")
        {
            dir();
            if (argc > 2)
            {
                pk.selectRun(argv[2], true, false);
            }
            else
            {
                pk.selectRun();
            }
        }
        else if (option == "-v" || option == "--version")
        {
            showVersion();
            return;
        }
        else if (option == "--version-verbose" || option == "-V")
        {
            showVersion(true);
            return;
        }
        else if (option == "-h" || option == "--help")
        {
            showHelp();
            return;
        }
        else if (option == "search")
        {
            pk.search();
        }
        else if (option == "log")
        {
            if (argc > 2)
            {
                File file;

                // 处理 --enable 选项
                if (std::strcmp(argv[2], "--enable") == 0)
                {
                    if (argc >= 4 && std::strcmp(argv[3], "global") == 0)
                    {
                        // 启用全局日志
                        file.setGlobalLogEnabled(true);
                        std::cerr << "Global logging is enabled" << std::endl;
                    }
                    else if (argc >= 4)
                    {
                        // 启用特定命令的日志
                        auto config = pk.showRecord(false);
                        std::string target = argv[3];
                        std::string directory;
                        int cmd_idx;
                        auto valid_dirs = file.get_valid_directories(config["path"]);

                        if (!pk.parseIndex(target, valid_dirs, config["path"], directory, cmd_idx))
                        {
                            std::cerr << "Invalid number" << std::endl;
                            return;
                        }

                        file.setCommandLogEnabled(directory, cmd_idx, true);
                        std::cerr << target << " logging is enabled" << std::endl;
                    }
                    else
                    {
                        // 交互式输入
                        auto config = pk.showRecord();
                        std::cerr << QCoreApplication::translate("Interaction", "Input index: ").toStdString();
                        std::string target;
                        std::getline(std::cin, target);
                        std::string directory;
                        int cmd_idx;
                        auto valid_dirs = file.get_valid_directories(config["path"]);

                        if (!pk.parseIndex(target, valid_dirs, config["path"], directory, cmd_idx))
                        {
                            std::cerr << "Invalid number" << std::endl;
                            return;
                        }

                        file.setCommandLogEnabled(directory, cmd_idx, true);
                        std::cerr << target << " logging is enabled" << std::endl;
                    }
                }
                // 处理 --disable 选项
                else if (std::strcmp(argv[2], "--disable") == 0)
                {
                    if (argc >= 4 && std::strcmp(argv[3], "global") == 0)
                    {
                        // 禁用全局日志
                        file.setGlobalLogEnabled(false);
                        std::cerr << "Global logging is disabled" << std::endl;
                    }
                    else if (argc >= 4)
                    {
                        // 禁用特定命令的日志
                        auto config = pk.showRecord(false);
                        std::string target = argv[3];
                        std::string directory;
                        int cmd_idx;
                        auto valid_dirs = file.get_valid_directories(config["path"]);

                        if (!pk.parseIndex(target, valid_dirs, config["path"], directory, cmd_idx))
                        {
                            std::cerr << "Invalid number" << std::endl;
                            return;
                        }

                        file.setCommandLogEnabled(directory, cmd_idx, false);
                        std::cerr << target << " logging is disabled" << std::endl;
                    }
                    else
                    {
                        // 交互式输入
                        auto config = pk.showRecord();
                        std::cerr << QCoreApplication::translate("Interaction", "Input index: ").toStdString();
                        std::string target;
                        std::getline(std::cin, target);
                        std::string directory;
                        int cmd_idx;
                        auto valid_dirs = file.get_valid_directories(config["path"]);

                        if (!pk.parseIndex(target, valid_dirs, config["path"], directory, cmd_idx))
                        {
                            std::cerr << "Invalid number" << std::endl;
                            return;
                        }

                        file.setCommandLogEnabled(directory, cmd_idx, false);
                        std::cerr << target << " logging is disabled" << std::endl;
                    }
                }
                else
                {
                    // 无效选项
                    std::cerr << "Invalid option. Use --enable or --disable" << std::endl;
                }
            }
            else
            {
                // 参数不足时显示日志文件
                shell.shellCommand("less .pk.log", "~", false, true);
            }
        }

        else if (option == "config")
        {
            Editor editor;
            if (argc > 2)
            {
                if (std::strcmp(argv[2], "-editor") == 0)
                {
                    if (argv[3] == nullptr || argv[3][0] == '\0')
                    {
                    SETEDITOR:
                        std::string currentEditor = editor.getEditor();
                        if (!currentEditor.empty())
                        {
                            std::cerr
                                << "Current editor setting: " << currentEditor
                                << "\n";
                        }
                        auto editors = editor.getAvailableEditors();

                        if (editors.empty())
                        {
                            std::cerr
                                << "No common editors found on your system.\n";
                            std::cerr << "Please install one (e.g., vim, nano) "
                                         "and rerun this script.\n";
                            return;
                        }

                        editor.printMenu(editors);
                        std::string selectedEditor =
                            editor.getUserChoice(editors);

                        std::cerr << "\nSelected editor: " << selectedEditor
                                  << "\n";
                        editor.setEditor(selectedEditor);
                    }
                    else
                    {
                        std::string selectedEditor = argv[3];
                        std::cerr << "Selected editor: " << selectedEditor
                                  << "\n";
                        editor.setEditor(selectedEditor);
                    }
                }
            }
            else
            {
                if (!editor.getEditor().empty())
                {
                    std::string configCommand =
                        editor.getEditor() + " " + Achieve::CONFIG_FILE;
                    shell.shellCommand(configCommand, "~", false, true);
                    return;
                }
                else
                {
                    std::cerr << "No current editor setting" << std::endl;
                    goto SETEDITOR;
                }
            }
        }
        else
        {
            std::cerr << Colors::RED
                      << QCoreApplication::translate("Interaction",
                                                     "未知选项: ")
                             .toStdString()
                      << option << Colors::RESET << std::endl;
            std::cerr << QCoreApplication::translate("Interaction", "用法")
                             .toStdString()
                      << ": pk [-a | -s | -p "
                       "| -c | -e "
                       "[index]]" << std::endl
                    << "      pk search" << std::endl
                    << "      pk log [--enable|--disable] [global|index]" << std::endl
                    << "      pk config [-editor [editor]]" << std::endl
                    << "      pk -h | --help" << std::endl
                    << "      pk -v | --version" << std::endl;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << Colors::RED
                  << QCoreApplication::translate("Interaction", "错误: ")
                         .toStdString()
                  << e.what() << Colors::RESET << std::endl;
    }
}
