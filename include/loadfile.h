#pragma once
#include <json/writer.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

#include "colors.h"
#include "json/value.h"

namespace Achieve
{
const std::string CONFIG_FILE = []() -> std::string
{
    const char* home = std::getenv("HOME");
    return home ? std::string(home) + "/.pk.json" : "./.pk.json";
}();
}  // namespace Achieve

class PathKeeper;

class JsonFormatter
{
    std::ostream& os_;
    int indent_;

public:
    JsonFormatter(std::ostream& os, int indent = 0) : os_(os), indent_(indent)
    {
    }
    void writeIndent();
    void writeString(const std::string& str);
    void writeValue(const Json::Value& value, int extraIndent);
    void writeArray(const Json::Value& array, int extraIndent);
    void writeObject(const Json::Value& obj, int extraIndent);
    void writeObject(const Json::Value& obj, int extraIndent,
                     const std::vector<std::string>& keyOrder);
};

class File
{
public:
    std::string CONFIG_FILE;
    static std::vector<std::string> path_keys_order;
    File();
    Json::Value loadConfig();
    void saveConfig(const Json::Value& config);
    void load_key_order();
    std::vector<std::string> get_valid_directories(const Json::Value& paths);
    int get_directory_index_by_display_number(int display_num,
                                              const Json::Value& paths);
    int get_display_number_by_directory_index(int orig_index,
                                              const Json::Value& paths);

    // 新增辅助函数：从命令条目（可能是字符串或对象）中提取命令文本和日志标志
    static std::string getCommandString(const Json::Value& cmdEntry);
    static void getCommandLogFlag(const Json::Value& cmdEntry, bool& log_set,
                                  bool& log_value);
};

class Editor
{
public:
    Editor();
    File file;
    const std::vector<std::string> COMMON_EDITORS;
    Json::Value config;
    void printMenu(
        const std::vector<std::pair<std::string, std::string>>& editors);
    std::string getUserChoice(
        const std::vector<std::pair<std::string, std::string>>& editors);
    std::string findInPath(const std::string& prog);
    std::vector<std::pair<std::string, std::string>> getAvailableEditors();
    std::string getEditor();
    void setEditor(const std::string& newEditor);
};