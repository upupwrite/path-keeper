#pragma once
#include <json/value.h>
#include <string>
#include <vector>
#include "loadfile.h"

class AliasManager {
    File& file_;
public:
    explicit AliasManager(File& f) : file_(f) {}
    void addAlias(const std::string& name, const std::string& index);
    void removeAlias(const std::string& name);
    void listAliases() const;
    void generateShellFile() const;
};
