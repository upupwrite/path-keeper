#pragma once
#include <jsoncpp/json/json.h>

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
    void shellCommand(const std::string& command, const std::string& dir,
                      const bool record = false);
};
