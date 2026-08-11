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
    std::cerr << QCoreApplication::translate("Interaction", "Current directory:")
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
            std::string index;
            std::string extra;
            if (argc > 2) {
                if (argv[2][0] == '-') {
                    for (int i = 2; i < argc; ++i) {
                        if (!extra.empty()) extra += " ";
                        extra += argv[i];
                    }
                } else {
                    index = argv[2];
                    for (int i = 3; i < argc; ++i) {
                        if (!extra.empty()) extra += " ";
                        extra += argv[i];
                    }
                }
            }
            if (index.empty() && !extra.empty()) {
                std::cerr << Colors::YELLOW
                          << QCoreApplication::translate("Interaction", "Warning: extra arguments will be ignored when no index is provided")
                                 .toStdString()
                          << Colors::RESET << std::endl;
                return;
            }
            pk.runPoint(index, extra);
        }
        else if (option == "-c" || option == "--configure")
        {
            dir();
            pk.setRecent();
        }
        else if (option == "-e" || option == "--execute")
        {
            dir();
            std::string index;
            std::string extra;
            if (argc > 2) {
                if (argv[2][0] == '-') {
                    for (int i = 2; i < argc; ++i) {
                        if (!extra.empty()) extra += " ";
                        extra += argv[i];
                    }
                } else {
                    index = argv[2];
                    for (int i = 3; i < argc; ++i) {
                        if (!extra.empty()) extra += " ";
                        extra += argv[i];
                    }
                }
            }
            if (index.empty() && !extra.empty()) {
                std::cerr << Colors::YELLOW
                          << QCoreApplication::translate("Interaction", "Warning: extra arguments will be ignored when no index is provided")
                                 .toStdString()
                          << Colors::RESET << std::endl;
                return;
            }
            if (!index.empty()) {
                pk.selectRun(index, true, false, extra, true);
            } else {
                // No index and no extra arguments: enter interactive selection
                pk.selectRun("", true, true, "", true);
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

                // Handle --enable option
                if (std::strcmp(argv[2], "--enable") == 0)
                {
                    if (argc >= 4 && std::strcmp(argv[3], "global") == 0)
                    {
                        // Enable global logging
                        file.setGlobalLogEnabled(true);
                        std::cerr << QCoreApplication::translate("Interaction", "Global logging is enabled")
                                         .toStdString() << std::endl;
                    }
                    else if (argc >= 4)
                    {
                        // Enable logging for a specific command
                        auto config = pk.showRecord(false);
                        std::string target = argv[3];
                        std::string directory;
                        int cmd_idx;
                        auto valid_dirs =
                            file.get_valid_directories(config["path"]);

                        if (!pk.parseIndex(target, valid_dirs, config["path"],
                                           directory, cmd_idx))
                        {
                            std::cerr << QCoreApplication::translate("Interaction", "Invalid number")
                                             .toStdString() << std::endl;
                            return;
                        }

                        file.setCommandLogEnabled(directory, cmd_idx, true);
                        std::cerr << target
                                  << QCoreApplication::translate("Interaction", " logging is enabled")
                                         .toStdString() << std::endl;
                    }
                    else
                    {
                        // Interactive input
                        auto config = pk.showRecord();
                        std::cerr << QCoreApplication::translate(
                                         "Interaction", "Input index: ")
                                         .toStdString();
                        std::string target;
                        std::getline(std::cin, target);
                        std::string directory;
                        int cmd_idx;
                        auto valid_dirs =
                            file.get_valid_directories(config["path"]);

                        if (!pk.parseIndex(target, valid_dirs, config["path"],
                                           directory, cmd_idx))
                        {
                            std::cerr << QCoreApplication::translate("Interaction", "Invalid number")
                                             .toStdString() << std::endl;
                            return;
                        }

                        file.setCommandLogEnabled(directory, cmd_idx, true);
                        std::cerr << target
                                  << QCoreApplication::translate("Interaction", " logging is enabled")
                                         .toStdString() << std::endl;
                    }
                }
                // Handle --disable option
                else if (std::strcmp(argv[2], "--disable") == 0)
                {
                    if (argc >= 4 && std::strcmp(argv[3], "global") == 0)
                    {
                        // Disable global logging
                        file.setGlobalLogEnabled(false);
                        std::cerr << QCoreApplication::translate("Interaction", "Global logging is disabled")
                                         .toStdString() << std::endl;
                    }
                    else if (argc >= 4)
                    {
                        // Disable logging for a specific command
                        auto config = pk.showRecord(false);
                        std::string target = argv[3];
                        std::string directory;
                        int cmd_idx;
                        auto valid_dirs =
                            file.get_valid_directories(config["path"]);

                        if (!pk.parseIndex(target, valid_dirs, config["path"],
                                           directory, cmd_idx))
                        {
                            std::cerr << QCoreApplication::translate("Interaction", "Invalid number")
                                             .toStdString() << std::endl;
                            return;
                        }

                        file.setCommandLogEnabled(directory, cmd_idx, false);
                        std::cerr << target
                                  << QCoreApplication::translate("Interaction", " logging is disabled")
                                         .toStdString() << std::endl;
                    }
                    else
                    {
                        // Interactive input
                        auto config = pk.showRecord();
                        std::cerr << QCoreApplication::translate(
                                         "Interaction", "Input index: ")
                                         .toStdString();
                        std::string target;
                        std::getline(std::cin, target);
                        std::string directory;
                        int cmd_idx;
                        auto valid_dirs =
                            file.get_valid_directories(config["path"]);

                        if (!pk.parseIndex(target, valid_dirs, config["path"],
                                           directory, cmd_idx))
                        {
                            std::cerr << QCoreApplication::translate("Interaction", "Invalid number")
                                             .toStdString() << std::endl;
                            return;
                        }

                        file.setCommandLogEnabled(directory, cmd_idx, false);
                        std::cerr << target
                                  << QCoreApplication::translate("Interaction", " logging is disabled")
                                         .toStdString() << std::endl;
                    }
                }
                else
                {
                    // Invalid option
                    std::cerr << QCoreApplication::translate("Interaction", "Invalid option. Use --enable or --disable")
                                     .toStdString() << std::endl;
                }
            }
            else
            {
                // Not enough arguments – show log file
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
                                << QCoreApplication::translate("Interaction", "Current editor setting: ")
                                       .toStdString()
                                << currentEditor << "\n";
                        }
                        auto editors = editor.getAvailableEditors();

                        if (editors.empty())
                        {
                            std::cerr
                                << QCoreApplication::translate("Interaction", "No common editors found on your system.")
                                       .toStdString() << "\n";
                            std::cerr << QCoreApplication::translate("Interaction", "Please install one (e.g., vim, nano) and rerun this script.")
                                             .toStdString() << "\n";
                            return;
                        }

                        editor.printMenu(editors);
                        std::string selectedEditor =
                            editor.getUserChoice(editors);

                        std::cerr << "\n"
                                  << QCoreApplication::translate("Interaction", "Selected editor: ")
                                         .toStdString()
                                  << selectedEditor << "\n";
                        editor.setEditor(selectedEditor);
                    }
                    else
                    {
                        std::string selectedEditor = argv[3];
                        std::cerr << QCoreApplication::translate("Interaction", "Selected editor: ")
                                         .toStdString()
                                  << selectedEditor << "\n";
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
                    std::cerr << QCoreApplication::translate("Interaction", "No current editor setting")
                                     .toStdString() << std::endl;
                    goto SETEDITOR;
                }
            }
        }
        else if (option == "alias")
        {
            if (argc < 3) {
                std::cerr << QCoreApplication::translate("Interaction", "Usage: pk alias add <name> <index> | remove <name> | list | install")
                                 .toStdString() << "\n";
                return;
            }
            std::string subcmd = argv[2];
            if (subcmd == "add") {
                if (argc < 5) {
                    std::cerr << QCoreApplication::translate("Interaction", "Usage: pk alias add <name> <index>")
                                     .toStdString() << "\n";
                    return;
                }
                pk.addAlias(argv[3], argv[4]);
            } else if (subcmd == "remove") {
                if (argc < 4) {
                    std::cerr << QCoreApplication::translate("Interaction", "Usage: pk alias remove <name>")
                                     .toStdString() << "\n";
                    return;
                }
                pk.removeAlias(argv[3]);
            } else if (subcmd == "list") {
                pk.listAliases();
            } else if (subcmd == "install") {
                pk.installAliases();
            } else {
                std::cerr << QCoreApplication::translate("Interaction", "Unknown alias subcommand: ")
                                 .toStdString()
                          << subcmd << "\n";
                std::cerr << QCoreApplication::translate("Interaction", "Usage: pk alias add <name> <index> | remove <name> | list | install")
                                 .toStdString() << "\n";
            }
        }
        else
        {
            std::cerr << Colors::RED
                      << QCoreApplication::translate("Interaction", "Unknown option: ")
                             .toStdString()
                      << option << Colors::RESET << std::endl;
            std::cerr << QCoreApplication::translate("Interaction", "Usage")
                             .toStdString()
                      << ": pk [-a | -s | -p "
                         "| -c | -e "
                         "[index]]"
                      << std::endl
                      << "      pk search" << std::endl
                      << "      pk log [--enable|--disable] [global|index]"
                      << std::endl
                      << "      pk config [-editor [editor]]" << std::endl
                      << "      pk alias add <name> <index> | remove <name> | list | install" << std::endl
                      << "      pk -h | --help" << std::endl
                      << "      pk -v | --version" << std::endl;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << Colors::RED
                  << QCoreApplication::translate("Interaction", "Error: ")
                         .toStdString()
                  << e.what() << Colors::RESET << std::endl;
    }
}
