#include "info.h"

#include <iostream>
#include <iterator>

#include "colors.h"

const std::string VERSION = "2.4.3";

void showVersion(bool verbose)
{
    std::cout << "path-keeper v" << VERSION << "\n";
    std::cout << "Copyright (C) 2023-2026 path-keeper Inc.\n";
    std::cout << "License: GPL-3.0-or-later\n\n";

    if (verbose)
    {
        std::cout << "Build Information:\n";
        std::cout << "  Build date:   " << BUILD_DATE << "\n";
        std::cout << "  Compiler:     " << COMPILER << "\n";
        std::cout << "  Architecture: "
                  << (sizeof(void*) == 8 ? "64-bit" : "32-bit") << "\n\n";
    }

    std::cout << "This program is free software: you can redistribute it "
                 "and/or modify\n";
    std::cout << "it under the terms of the GNU General Public License as "
                 "published by\n";
    std::cout << "the Free Software Foundation, either version 3 of the "
                 "License, or\n";
    std::cout << "(at your option) any later version.\n\n";

    std::cout << "Written by upupwrite\n";
    std::cout << "Home page: <https://gitee.com/upupwrite/path-keeper>\n";
}

void showHelp()
{
    std::cout << "path-keeper Help" << std::endl;
    std::cout << "Options:\n";

    std::cout << Colors::BOLD << "  " << "--add" << Colors::RESET << ":\n"
              << "    " << "  ";
    std::cout << "Add a command" << std::endl;

    std::cout << Colors::BOLD << "  " << "--execute" << Colors::RESET << ":\n"
              << "    " << "  ";
    std::cout << "Execute a command" << std::endl;

    std::cout << Colors::BOLD << "  " << "--point" << Colors::RESET << ":\n"
              << "    " << "  ";
    std::cout << "Execute a command without record into recent" << std::endl;

    std::cout << Colors::BOLD << "  " << "--configure" << Colors::RESET << ":\n"
              << "    " << "  ";
    std::cout << "Configure recent command" << std::endl;

    std::cout << Colors::BOLD << "  " << "--version" << Colors::RESET << ":\n"
              << "    " << "  ";
    std::cout << "Check version" << std::endl;

    std::cout << Colors::BOLD << "  " << "--version-verbose" << Colors::RESET
              << ":\n"
              << "    " << "  ";
    std::cout << "Check version verbose" << std::endl;

    std::cout << Colors::BOLD << "  " << "--help" << Colors::RESET << ":\n"
              << "    " << "  ";
    std::cout << "Get this help" << std::endl;
}
