#pragma once
#include <json/value.h>

#include <string>

#include "loadfile.h"
// #include "logger.h"

class Searcher
{
public:
    static std::string interactiveSearch(const Json::Value& config, File& file);
};
