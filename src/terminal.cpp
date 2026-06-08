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

// #include "pk.h"

// void PathKeeper::shellCommand(const std::string& command,
//                               const std::string& cwd)
//{
//     //    std::cout<<myshell<<" -c cd "<<cwd<<" "<<command;
//     std::cout << "cd " << cwd << " && " << command << " && cd " << this->cwd
//               << std::endl;
// }

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

void Shell::shellCommand(const std::string& command, const std::string& dir,
                         const bool record)
{
    std::string log_prefix = "[" + log.timestamp() + "]\n" + "DIR: " + dir +
                             "\n" + "COMMAND: " + command + "\n";
    Json::Value config = file.loadConfig();
    std::string shell = config["shell"].asString();
    std::string log_file = Achieve::LOG_FILE;
    if (record)
    {
        std::cout << "echo " << "'" << log_prefix << "' >> " << log_file
                  << " && "
                  << "tmux pipe-pane 'cat >> " << log_file << "'&&" << shell
                  << " <<EOF\n"
                  << "cd " << dir << " && " << command << "\nEOF"
                  << "\n"
                  << "tmux pipe-pane" << std::endl;
    }
    else
    {
        std::cout << "echo '" << log_prefix << "' >> " << log_file << " && "
                  << shell << " <<EOF\n"
                  << "cd " << dir << " && " << command << "\nEOF" << std::endl;
    }
}
