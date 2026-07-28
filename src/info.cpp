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

#include "info.h"

#include <iostream>
#include <iterator>

#include "colors.h"

const std::string VERSION = "2.4.9";

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
    std::cerr << "path-keeper Help" << std::endl;
    std::cerr << "Options:\n";

    std::cerr << Colors::BOLD << "  " << "--add" << Colors::RESET << ":\n"
              << "    " << "  ";
    std::cerr << "Add a command" << std::endl;

    std::cerr << Colors::BOLD << "  " << "--execute" << Colors::RESET << ":\n"
              << "    " << "  ";
    std::cerr << "Execute a command" << std::endl;

    std::cerr << Colors::BOLD << "  " << "--point" << Colors::RESET << ":\n"
              << "    " << "  ";
    std::cerr << "Execute a command without record into recent" << std::endl;

    std::cerr << Colors::BOLD << "  " << "--configure" << Colors::RESET << ":\n"
              << "    " << "  ";
    std::cerr << "Configure recent command" << std::endl;

    std::cerr << Colors::BOLD << "  " << "--version" << Colors::RESET << ":\n"
              << "    " << "  ";
    std::cerr << "Check version" << std::endl;

    std::cerr << Colors::BOLD << "  " << "--version-verbose" << Colors::RESET
              << ":\n"
              << "    " << "  ";
    std::cerr << "Check version verbose" << std::endl;

    std::cerr << Colors::BOLD << "  " << "--help" << Colors::RESET << ":\n"
              << "    " << "  ";
    std::cerr << "Get this help" << std::endl;
}
