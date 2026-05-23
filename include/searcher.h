#pragma once
#include <string>
#include <json/value.h>
#include "loadfile.h"
#include "logger.h"

class Searcher {
public:
    static std::string interactiveSearch(const Json::Value& config, File& file);
};
