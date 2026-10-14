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
#if __has_include(<jsoncpp/json/json.h>)
#  include <jsoncpp/json/json.h>   // Debian / Ubuntu
#elif __has_include(<json/json.h>)
#  include <json/json.h>            // Arch Linux
#else
#  error "jsoncpp headers not found"
#endif

#include "loadfile.h"
#include "log.h"

class Shell
{
private:
    File file;
    Log log;

public:
    Shell();
    std::string cwd;
    void shellCommand(const std::string &command, const std::string &dir,
                      const bool record = false, const bool self = false,
                      const bool non_shell = false);
};
