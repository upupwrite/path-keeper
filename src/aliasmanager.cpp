#include "aliasmanager.h"
#include <fstream>
#include <iostream>
#include <cstdlib>

void AliasManager::addAlias(const std::string& name, const std::string& index) {
    Json::Value config = file_.loadConfig();
    config["aliases"][name] = index;
    file_.saveConfig(config);
    generateShellFile();
}

void AliasManager::removeAlias(const std::string& name)
{
    Json::Value config = file_.loadConfig();
    if (config["aliases"].isMember(name))
    {
        config["aliases"].removeMember(name);
        file_.saveConfig(config);
        generateShellFile();
    }
}
void AliasManager::listAliases() const {
    Json::Value config = file_.loadConfig();
    if (config["aliases"].empty()) {
        std::cerr << "No aliases defined.\n";
        return;
    }
    for (const auto& key : config["aliases"].getMemberNames())
        std::cerr << key << " -> " << config["aliases"][key].asString() << std::endl;
}

void AliasManager::generateShellFile() const {
    const char* home = getenv("HOME");
    if (!home) return;
    std::string path = std::string(home) + "/.pk_aliases.sh";
    std::ofstream file(path);
    if (!file) return;

    Json::Value config = file_.loadConfig();
    for (const auto& name : config["aliases"].getMemberNames()) {
        std::string idx = config["aliases"][name].asString();
        file << "pk_" << name << "() { pk execute -- \"" << idx << "\" \"$@\"; }\n";
        file << "alias " << name << "='pk_" << name << "'\n";
    }
}
