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

#include "terminal.h"

#include <unistd.h>

#include <cstdlib>
#include <iostream>

Shell::Shell()
{
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

void Shell::shellCommand(const std::string &command, const std::string &dir,
                         const bool record, const bool self)
{
    static bool tmux_checked = false;
    static bool has_tmux = false;
    if (!tmux_checked)
    {
        has_tmux = (system("which tmux > /dev/null 2>&1") == 0);
        tmux_checked = true;
    }

    std::string log_prefix = "[" + log.timestamp() + "]\n" + "DIR: " + dir +
                             "\n" + "COMMAND: " + command + "\n";
    Json::Value config = file.loadConfig();
    std::string shell = config["shell"].asString();
    std::string log_file = Achieve::LOG_FILE;
    std::string shell_command;

    if (std::empty(shell))
    {
        shell_command = "cd " + dir + " && " + command + " ; cd " + cwd;
    }
    else
    {
        shell_command = shell + " <<EOF\n" + "cd " + dir + " && " + command +
                        " ; cd " + cwd + "\nEOF";
    }

    if (!self && !std::empty(shell_command))
    {
        if (record && has_tmux)
        {
            std::cout << "echo '\n"
                      << log_prefix << "' >> " << log_file << " && "
                      << "tmux pipe-pane 'cat >> " << log_file << "'&&"
                      << shell_command << "\n"
                      << "tmux pipe-pane" << std::endl;
        }
        else if (record && !has_tmux)
        {
            std::cerr
                << "Warning: tmux not found, logging output not available."
                << std::endl;
            std::cout << "echo '\n"
                      << log_prefix << "' >> " << log_file << " && "
                      << shell_command << std::endl;
        }
        else
        {
            std::cout << shell_command << std::endl;
        }
    }
    else
    {
        std::cout << shell_command << std::endl;
    }
}
