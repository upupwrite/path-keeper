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
#include <json/value.h>

#include <string>

#include "loadfile.h"

struct SearchResult {
  bool valid;
  std::string directory;
  int commandIndex;
  std::string effectiveCommand;

  SearchResult() : valid(false), commandIndex(-1) {}
  SearchResult(const std::string &dir, int idx, const std::string &cmd)
      : valid(true), directory(dir), commandIndex(idx), effectiveCommand(cmd) {}
};

class Searcher {
public:
  static SearchResult interactiveSearch(const Json::Value &config, File &file);
};
