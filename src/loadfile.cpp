#include "loadfile.h"

#include <sys/stat.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>

#include "colors.h"
#include "pk.h"

std::vector<std::string> File::path_keys_order;

namespace
{
const Json::Value DEFAULT_CONFIG = []()
{
    Json::Value config;
    config["path"] = Json::objectValue;
    config["recent"] = Json::nullValue;
    config["shell"] = "sh";
    config["editor"] = "";
    return config;
}();

struct JsonEscape
{
    static std::string escapeChar(char c)
    {
        switch (c)
        {
        case '"': return "\\\"";
        case '\\': return "\\\\";
        case '\b': return "\\b";
        case '\f': return "\\f";
        case '\n': return "\\n";
        case '\r': return "\\r";
        case '\t': return "\\t";
        default:
            if (static_cast<unsigned char>(c) < 0x20 || c == 0x7f)
            {
                char buf[7];
                snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
                return buf;
            }
            return std::string(1, c);
        }
    }
};
}

void JsonFormatter::writeIndent()
{
    for (int i = 0; i < indent_; ++i) os_ << ' ';
}

void JsonFormatter::writeString(const std::string &str)
{
    os_ << '"';
    for (char c : str) os_ << JsonEscape::escapeChar(c);
    os_ << '"';
}

void JsonFormatter::writeValue(const Json::Value &value, int extraIndent)
{
    switch (value.type())
    {
    case Json::nullValue: os_ << "null"; break;
    case Json::intValue: os_ << value.asInt(); break;
    case Json::uintValue: os_ << value.asUInt(); break;
    case Json::realValue: os_ << value.asDouble(); break;
    case Json::stringValue: writeString(value.asString()); break;
    case Json::booleanValue: os_ << (value.asBool() ? "true" : "false"); break;
    case Json::arrayValue: writeArray(value, extraIndent); break;
    case Json::objectValue: writeObject(value, extraIndent); break;
    }
}

void JsonFormatter::writeArray(const Json::Value &array, int extraIndent)
{
    if (array.empty()) { os_ << "[]"; return; }
    os_ << "[";
    int newIndent = indent_ + extraIndent;
    for (Json::ArrayIndex i = 0; i < array.size(); ++i)
    {
        if (i > 0) os_ << ",";
        os_ << "\n";
        for (int j = 0; j < newIndent; ++j) os_ << ' ';
        JsonFormatter formatter(os_, newIndent);
        formatter.writeValue(array[i], extraIndent);
    }
    os_ << "\n";
    for (int j = 0; j < indent_; ++j) os_ << ' ';
    os_ << "]";
}

void JsonFormatter::writeObject(const Json::Value &obj, int extraIndent)
{
    if (obj.empty()) { os_ << "{}"; return; }
    os_ << "{";
    int newIndent = indent_ + extraIndent;
    bool first = true;
    Json::Value::Members members = obj.getMemberNames();
    std::sort(members.begin(), members.end());
    for (size_t i = 0; i < members.size(); ++i)
    {
        if (!first) os_ << ",";
        os_ << "\n";
        for (int j = 0; j < newIndent; ++j) os_ << ' ';
        writeString(members[i]);
        os_ << ": ";
        JsonFormatter formatter(os_, newIndent);
        formatter.writeValue(obj[members[i]], extraIndent);
        first = false;
    }
    os_ << "\n";
    for (int j = 0; j < indent_; ++j) os_ << ' ';
    os_ << "}";
}

void JsonFormatter::writeObject(const Json::Value &obj, int extraIndent,
                                const std::vector<std::string> &keyOrder)
{
    if (obj.empty()) { os_ << "{}"; return; }
    os_ << "{";
    int newIndent = indent_ + extraIndent;
    bool first = true;
    for (const auto &key : keyOrder)
    {
        if (!obj.isMember(key)) continue;
        if (!first) os_ << ",";
        os_ << "\n";
        for (int j = 0; j < newIndent; ++j) os_ << ' ';
        writeString(key);
        os_ << ": ";
        JsonFormatter formatter(os_, newIndent);
        formatter.writeValue(obj[key], extraIndent);
        first = false;
    }
    os_ << "\n";
    for (int j = 0; j < indent_; ++j) os_ << ' ';
    os_ << "}";
}

File::File() { CONFIG_FILE = Achieve::CONFIG_FILE; }





void File::load_key_order()
{
    path_keys_order.clear();
    std::ifstream file(CONFIG_FILE);
    if (!file.is_open()) return;

    std::string line;
    bool in_path_section = false;
    int brace_count = 0;

    while (std::getline(file, line))
    {
        std::string::size_type firstNonSpace = line.find_first_not_of(" \t");
        if (firstNonSpace == std::string::npos) continue;

        if (line.find("\"path\":") != std::string::npos)
        {
            in_path_section = true;
            size_t brace_pos = line.find('{', firstNonSpace);
            if (brace_pos != std::string::npos) brace_count = 1;
            continue;
        }

        if (in_path_section)
        {
            for (char c : line)
            {
                if (c == '{') brace_count++;
                else if (c == '}') brace_count--;
            }
            if (brace_count == 0) break;

            if (line.find("//") != std::string::npos) continue;

            std::string::size_type quoteStart = line.find('"', firstNonSpace);
            if (quoteStart == std::string::npos) continue;
            std::string::size_type quoteEnd = line.find('"', quoteStart + 1);
            if (quoteEnd == std::string::npos) continue;
            if (line.find(':', quoteEnd) == std::string::npos) continue;

            std::string key = line.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
            if (!key.empty()) path_keys_order.push_back(key);
        }
    }
}

Json::Value File::loadConfig()
{
    std::ifstream file(CONFIG_FILE);
    if (!file.is_open()) {
        Json::Value defaultConfig;
        defaultConfig["path"] = Json::objectValue;
        defaultConfig["recent"] = Json::nullValue;
        defaultConfig["shell"] = "sh";
        defaultConfig["editor"] = "";
        defaultConfig["log"] = Json::objectValue; // 新增日志字段
        defaultConfig["log"]["enabled"] = false;
        defaultConfig["log"]["dir"] = "~/.pk_logs";
        defaultConfig["log"]["max_age_days"] = 30;
        defaultConfig["aliases"] = Json::objectValue;
        saveConfig(defaultConfig);
        return defaultConfig;
    }

    Json::Value config;
    try {
        file >> config;
        // 确保基本字段存在
        if (!config.isMember("path")) config["path"] = Json::objectValue;
        if (!config.isMember("recent")) config["recent"] = Json::nullValue;
        if (!config.isMember("shell")) config["shell"] = "sh";
        if (!config.isMember("editor")) config["editor"] = "";
        if (!config.isMember("log")) config["log"] = Json::objectValue;
        if (!config["log"].isMember("enabled")) config["log"]["enabled"] = false;
        if (!config["log"].isMember("dir")) config["log"]["dir"] = "~/.pk_logs";
        if (!config["log"].isMember("max_age_days")) config["log"]["max_age_days"] = 30;
        if (!config.isMember("aliases")) config["aliases"] = Json::objectValue;

        // 转换旧格式 path 条目
        Json::Value& pathObj = config["path"];
        Json::Value::Members dirs = pathObj.getMemberNames();
        for (const auto& dir : dirs) {
            if (pathObj[dir].isArray()) {
                // 旧格式：数组变为 { "cmds": array }
                Json::Value newEntry;
                newEntry["cmds"] = pathObj[dir];
                pathObj[dir] = newEntry;
            } else if (pathObj[dir].isObject()) {
                if (!pathObj[dir].isMember("cmds"))
                    pathObj[dir]["cmds"] = Json::arrayValue;
                if (!pathObj[dir].isMember("hashes"))
                    pathObj[dir]["hashes"] = Json::arrayValue;
            }
        }
    } catch (const Json::Exception& e) {
        std::cerr << Colors::RED << "解析配置文件错误: " << e.what() << Colors::RESET << std::endl;
    }
    return config;
}

void File::saveConfig(const Json::Value& config)
{
    std::ofstream file(CONFIG_FILE);
    if (!file.is_open()) return;

    // 保持键顺序
    Json::Value paths = config["path"];
    Json::Value::Members memberNames = paths.getMemberNames();
    std::vector<std::string> orderedKeys = path_keys_order;
    for (const auto& it : memberNames) {
        if (std::find(orderedKeys.begin(), orderedKeys.end(), it) == orderedKeys.end())
            orderedKeys.push_back(it);
    }

    file << "{\n";
    file << "  \"path\": ";
    {
        JsonFormatter formatter(file, 2);
        formatter.writeObject(paths, 2, orderedKeys);
    }

    if (config.isMember("recent") && !config["recent"].isNull()) {
        file << ",\n  \"recent\": ";
        JsonFormatter(file, 2).writeValue(config["recent"], 2);
    }
    if (config.isMember("shell")) {
        file << ",\n  \"shell\": ";
        JsonFormatter(file, 2).writeValue(config["shell"], 2);
    }
    if (config.isMember("editor")) {
        file << ",\n  \"editor\": ";
        JsonFormatter(file, 2).writeValue(config["editor"], 2);
    }
    if (config.isMember("log")) {
        file << ",\n  \"log\": ";
        JsonFormatter(file, 2).writeValue(config["log"], 2);
    }
    if (config.isMember("aliases")) {
        file << ",\n  \"aliases\": ";
        JsonFormatter(file, 2).writeValue(config["aliases"], 2);
    }
    file << "\n}";

    path_keys_order = orderedKeys;
}

std::vector<std::string> File::get_valid_directories(const Json::Value& paths)
{
    std::vector<std::string> valid;
    for (const auto& key : path_keys_order) {
        if (paths.isMember(key) && paths[key].isObject())
            valid.push_back(key);
    }
    return valid;
}

int File::get_directory_index_by_display_number(int display_num, const Json::Value &paths)
{
    std::vector<std::string> validDirs = get_valid_directories(paths);
    if (display_num < 1 || display_num > static_cast<int>(validDirs.size())) return -1;
    std::string selectedDir = validDirs[display_num - 1];
    for (size_t i = 0; i < path_keys_order.size(); ++i)
    {
        if (path_keys_order[i] == selectedDir && paths.isMember(selectedDir))
            return static_cast<int>(i);
    }
    return -1;
}

int File::get_display_number_by_directory_index(int orig_index, const Json::Value &paths)
{
    if (orig_index < 0 || orig_index >= static_cast<int>(path_keys_order.size())) return -1;
    std::string directory = path_keys_order[orig_index];
    if (!paths.isMember(directory)) return -1;
    std::vector<std::string> validDirs = get_valid_directories(paths);
    std::vector<std::string>::iterator it =
        std::find(validDirs.begin(), validDirs.end(), directory);
    if (it != validDirs.end())
        return static_cast<int>(std::distance(validDirs.begin(), it) + 1);
    return -1;
}

Editor::Editor()
    : COMMON_EDITORS({"vim", "nvim", "nano", "emacs", "micro", "helix", "code",
                      "codium", "kate", "gedit", "mousepad", "ne", "jed", "joe"}) {}

std::string Editor::findInPath(const std::string &prog)
{
    std::string cmd = "which " + prog + " 2>/dev/null";
    FILE *pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "";
    char buffer[128];
    std::string result;
    if (fgets(buffer, sizeof(buffer), pipe) != nullptr)
    {
        result = buffer;
        if (!result.empty() && result.back() == '\n') result.pop_back();
    }
    pclose(pipe);
    if (!result.empty() && access(result.c_str(), X_OK) == 0) return result;
    return "";
}

std::vector<std::pair<std::string, std::string>> Editor::getAvailableEditors()
{
    std::vector<std::pair<std::string, std::string>> available;
    for (const auto &name : COMMON_EDITORS)
    {
        std::string path = findInPath(name);
        if (!path.empty()) available.emplace_back(name, path);
    }
    return available;
}

void Editor::printMenu(const std::vector<std::pair<std::string, std::string>> &editors)
{
    std::cerr << "\nAvailable editors on your system:\n";
    for (size_t i = 0; i < editors.size(); ++i)
    {
        std::cerr << "  " << (i + 1) << ". " << editors[i].first << " ("
                  << editors[i].second << ")\n";
    }
    std::cerr << "  0. Enter a custom editor command\n";
}

std::string Editor::getUserChoice(const std::vector<std::pair<std::string, std::string>> &editors)
{
    while (true)
    {
        std::cerr << "\nEnter the number of your choice (or 0 for custom): ";
        std::string input;
        std::getline(std::cin, input);
        input.erase(0, input.find_first_not_of(" \t\n\r"));
        input.erase(input.find_last_not_of(" \t\n\r") + 1);
        if (input.empty()) continue;

        char *end;
        long choice = strtol(input.c_str(), &end, 10);
        if (*end != '\0')
        {
            std::cerr << "Please enter a valid number.\n";
            continue;
        }

        if (choice == 0)
        {
            std::cerr << "Enter the editor command (e.g., 'hx' or '/usr/bin/nvim'): ";
            std::string custom;
            std::getline(std::cin, custom);
            custom.erase(0, custom.find_first_not_of(" \t\n\r"));
            custom.erase(custom.find_last_not_of(" \t\n\r") + 1);
            if (custom.empty())
            {
                std::cerr << "No custom command entered. Please choose again.\n";
                continue;
            }
            std::string resolved = findInPath(custom);
            if (!resolved.empty()) return resolved;
            else if (custom.find('/') != std::string::npos && access(custom.c_str(), X_OK) == 0)
                return custom;
            else
            {
                std::cerr << Colors::YELLOW << "Warning: '" << Colors::RESET << custom
                          << "' does not seem to be executable. Using it anyway.\n";
                return custom;
            }
        }
        else if (choice >= 1 && static_cast<size_t>(choice) <= editors.size())
        {
            return editors[choice - 1].second;
        }
        else
        {
            std::cerr << "Invalid number. Please try again.\n";
        }
    }
}

std::string Editor::getEditor() { return config["editor"].asString(); }

void Editor::setEditor(const std::string &newEditor)
{
    config["editor"] = newEditor;
    file.saveConfig(config);
}


// ========== 新增静态辅助函数 ==========
std::string File::getCommandString(const Json::Value& cmdEntry) {
    if (cmdEntry.isString()) {
        return cmdEntry.asString();
    } else if (cmdEntry.isObject() && cmdEntry.isMember("cmd")) {
        return cmdEntry["cmd"].asString();
    }
    return "";
}

void File::getCommandLogFlag(const Json::Value& cmdEntry, bool& log_set, bool& log_value) {
    log_set = false;
    log_value = false;
    if (cmdEntry.isObject() && cmdEntry.isMember("log")) {
        log_set = true;
        log_value = cmdEntry["log"].asBool();
    }
}