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
#include <ctime>
#include <iostream>
#include <string>

const std::string BUILD_DATE = __DATE__ " " __TIME__;
const std::string COMPILER =
#ifdef __clang__
    "Clang " __clang_version__;
#elif __GNUC__
    "GCC " __VERSION__;
#elif _MSC_VER
    "MSVC " + std::to_string(_MSC_VER);
#else
    "Unknown Compiler";
#endif

void showVersion(bool verbose = false);
void showHelp();
