#include "pk.h"

#include <QCoreApplication>
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include "colors.h"
#include "json/value.h"
#include "readline.h"

PathKeeper::PathKeeper()
{
    // Get current working directory
    char buffer[1024];
    if (getcwd(buffer, sizeof(buffer)) != nullptr)
    {
        cwd = buffer;
    }
    else
    {
        cwd = ".";
    }

    file.load_key_order();
}

void PathKeeper::addRecord()
{
    Json::Value config = file.loadConfig();
    // 初始化 readline（只需调用一次，建议放在程序启动时）
    static bool initialized = false;
    if (!initialized)
    {
        ReadlineHelper::initialize();
        initialized = true;
    }

    std::string directory = ReadlineHelper::read_line(
        Colors::CYAN +
        QCoreApplication::translate("addRecord", "请输入记录目录:")
            .toStdString() +
        Colors::RESET);

    if (directory.empty())
    {
        std::cout << Colors::YELLOW
                  << QCoreApplication::translate("addRecord", "目录不能为空!")
                         .toStdString()
                  << Colors::RESET << std::endl;
        return;
    }

    if (directory == ".")
    {
        directory = cwd;
    }

    // 检查目录是否已存在
    Json::Value commands = Json::arrayValue;

    if (fs::exists(directory) && fs::is_directory(directory))
    {
        std::cout << Colors::GREEN << directory << Colors::RESET << std::endl;
        commands = config["path"][directory];
        displayCommands(commands);
    }

    // Use readline to read commands (with completion)
    std::string cmd = ReadlineHelper::read_line(
        Colors::CYAN +
        QCoreApplication::translate("addRecord", "请输入命令:").toStdString() +
        Colors::RESET);

    if (cmd.empty() && commands.empty())
    {
        std::string default_command = "ls -l";
        std::cout << QCoreApplication::translate("addRecord", "使用默认命令: ")
                         .toStdString()
                  << default_command << std::endl;
        commands.append(default_command);
    }
    else if (!cmd.empty())
    {
        commands.append(cmd);
    }

    config["path"][directory] = commands;
    if (!config.isMember("shell") || config["shell"].isNull())
    {
        config["shell"] = "sh";
    }

    file.saveConfig(config);
    std::cout
        << Colors::GREEN
        << QCoreApplication::translate("addRecord", "记录已保存!").toStdString()
        << Colors::RESET << std::endl;
}

void PathKeeper::runRecent()
{
    Json::Value config = file.loadConfig();

    if (config["recent"].isNull())
    {
        std::cout << Colors::YELLOW
                  << QCoreApplication::translate("runRecent", "没有最近记录!")
                         .toStdString()
                  << Colors::RESET << std::endl;
        return;
    }

    if (!config["recent"].isArray() || config["recent"].size() != 2)
    {
        std::cout << Colors::RED
                  << QCoreApplication::translate("runRecent",
                                                 "最近记录格式无效!")
                         .toStdString()
                  << Colors::RESET << std::endl;
        return;
    }

    try
    {
        int orig_idx1 = config["recent"][0].asInt();
        int idx2 = config["recent"][1].asInt();
        Json::Value paths = config["path"];

        // Check using the original index
        if (orig_idx1 < 0 ||
            orig_idx1 >= static_cast<int>(file.path_keys_order.size()))
        {
            std::cout << Colors::RED
                      << QCoreApplication::translate("runRecent",
                                                     "最近记录无效!")
                             .toStdString()
                      << Colors::RESET << std::endl;
            return;
        }

        std::string directory = file.path_keys_order[orig_idx1];
        if (!paths.isMember(directory))
        {
            std::cout << Colors::RED
                      << QCoreApplication::translate(
                             "runRecent", "最近记录对应的目录已不存在!")
                             .toStdString()
                      << Colors::RESET << std::endl;
            return;
        }

        Json::Value commands = paths[directory];

        if (idx2 < 0 || idx2 >= static_cast<int>(commands.size()))
        {
            std::cout << Colors::RED
                      << QCoreApplication::translate("runRecent",
                                                     "最近记录无效!")
                             .toStdString()
                      << Colors::RESET << std::endl;
            return;
        }

        std::string command = commands[idx2].asString();
        runCommand(directory, command);
    }
    catch (const std::exception &)
    {
        std::cout << Colors::RED
                  << QCoreApplication::translate("runRecent", "最近记录无效!")
                         .toStdString()
                  << Colors::RESET << std::endl;
    }
}

Json::Value PathKeeper::showRecord(const bool show)
{
    Json::Value config = file.loadConfig();
    Json::Value paths = config["path"];
    if (paths.empty())
    {
        std::cout << Colors::YELLOW
                  << QCoreApplication::translate("showRecord", "没有记录!")
                         .toStdString()
                  << Colors::RESET << std::endl;
        return Json::Value();
    }
    if (show)
    {
        int i = 1;
        std::vector<std::string> valid_dirs = file.get_valid_directories(paths);

        for (const auto &directory : valid_dirs)
        {
            std::cout << Colors::BLUE << "[" << i << "]" << Colors::RESET << " "
                      << Colors::GREEN << directory << Colors::RESET
                      << std::endl;

            Json::Value commands = paths[directory];

            displayCommands(commands, i);

            i++;
        }
    }

    displayRecentMark(config, paths);
    return config;
}

void PathKeeper::runCommand(const std::string &directory,
                            const std::string &command)
{
    std::cout
        << Colors::GREEN
        << QCoreApplication::translate("runCommand", "目录:").toStdString()
        << Colors::RESET << " " << directory << std::endl;
    std::cout
        << Colors::GREEN
        << QCoreApplication::translate("runCommand", "命令:").toStdString()
        << Colors::RESET << " " << Colors::CYAN << command << Colors::RESET
        << std::endl;
    std::cout
        << Colors::YELLOW
        << QCoreApplication::translate("runCommand", "执行命令: ").toStdString()
        << command << Colors::RESET << std::endl;

    int ret = shellCommand(command, directory);

    std::cout << Colors::BOLD << "return: " << ret << Colors::RESET
              << std::endl;
}

void PathKeeper::setRecent(const std::string &cmd_index)
{
    Json::Value config = showRecord();
    if (config.isNull())
        return;

    Json::Value paths = config["path"];
    std::string index_str = getInputIndex(
        cmd_index, QCoreApplication::translate("setRecent", "请输入目标编号: ")
                       .toStdString());
    if (index_str.empty())
        return;

    processIndexSelection(index_str, paths, config, false);
}

void PathKeeper::selectRun(const std::string &cmd_index, const bool set_recent,
                           const bool show)
{
    Json::Value config = showRecord(show);
    if (config.isNull())
        return;

    Json::Value paths = config["path"];
    if (paths.empty())
    {
        std::cout << Colors::YELLOW
                  << QCoreApplication::translate("selectRun", "没有记录!")
                         .toStdString()
                  << Colors::RESET << std::endl;
        return;
    }

    std::string index_str = getInputIndex(
        cmd_index,
        QCoreApplication::translate("selectRun", "请输入要执行的编号: ")
            .toStdString());
    if (index_str.empty())
    {
        runRecent();
        return;
    }

    processIndexSelection(index_str, paths, config, true, set_recent);
}

void PathKeeper::runPoint(const std::string &cmd_index)
{
    if (cmd_index.empty())
    {
        selectRun(cmd_index, false, true);
    }
    else
    {
        selectRun(cmd_index, false, false);
    }
}

void PathKeeper::displayCommands(const Json::Value &commands, int parent_index)
{
    for (Json::ArrayIndex j = 0; j < commands.size(); j++)
    {
        if (parent_index > 0)
        {
            std::cout << "    " << Colors::CYAN << "[" << parent_index << "."
                      << (j + 1) << "]" << Colors::RESET << " "
                      << commands[j].asString() << std::endl;
        }
        else
        {
            std::cout << "    " << Colors::CYAN << "[" << (j + 1) << "]"
                      << Colors::RESET << " " << commands[j].asString()
                      << std::endl;
        }
    }
}

void PathKeeper::displayRecentMark(const Json::Value &config,
                                   const Json::Value &paths)
{
    if (!config["recent"].isNull() && config["recent"].isArray() &&
        config["recent"].size() == 2)
    {
        int orig_idx1 = config["recent"][0].asInt();
        int idx2 = config["recent"][1].asInt();

        int display_num =
            file.get_display_number_by_directory_index(orig_idx1, paths);
        if (display_num != -1)
        {
            std::string recent_dir = file.path_keys_order[orig_idx1];
            Json::Value commands = paths[recent_dir];
            if (idx2 >= 0 && idx2 < static_cast<int>(commands.size()))
            {
                std::cout << Colors::YELLOW
                          << QCoreApplication::translate("displayRecentMark",
                                                         "最近执行")
                                 .toStdString()
                          << ": [" << display_num << "." << (idx2 + 1) << "] "
                          << recent_dir << Colors::RESET << std::endl;
            }
        }
    }
}

std::string PathKeeper::getInputIndex(const std::string &provided_index,
                                      const std::string &prompt)
{
    if (!provided_index.empty())
        return provided_index;

    std::cout << Colors::CYAN << prompt << Colors::RESET;
    std::string index_str;
    std::getline(std::cin, index_str);
    return index_str;
}

bool PathKeeper::parseIndex(const std::string &index_str,
                            std::vector<std::string> &valid_dirs,
                            Json::Value &paths, std::string &directory,
                            int &cmd_idx)
{
    if (index_str.find('.') != std::string::npos)
    {
        size_t dot_pos = index_str.find('.');
        std::string part1 = index_str.substr(0, dot_pos);
        std::string part2 = index_str.substr(dot_pos + 1);

        try
        {
            int display_num = std::stoi(part1);
            cmd_idx = std::stoi(part2) - 1;

            if (display_num < 1 ||
                display_num > static_cast<int>(valid_dirs.size()))
                return false;

            directory = valid_dirs[display_num - 1];
            Json::Value commands = paths[directory];

            return (cmd_idx >= 0 &&
                    cmd_idx < static_cast<int>(commands.size()));
        }
        catch (const std::exception &)
        {
            return false;
        }
    }
    else
    {
        try
        {
            int display_num = std::stoi(index_str);

            if (display_num < 1 ||
                display_num > static_cast<int>(valid_dirs.size()))
                return false;

            directory = valid_dirs[display_num - 1];
            Json::Value commands = paths[directory];

            if (commands.size() > 1)
            {
                std::cout << Colors::GREEN
                          << QCoreApplication::translate("parseIndex", "目录:")
                                 .toStdString()
                          << Colors::RESET << " " << directory << std::endl;
                displayCommands(commands);

                std::cout << Colors::CYAN
                          << QCoreApplication::translate("parseIndex",
                                                         "请选择命令编号: ")
                                 .toStdString()
                          << Colors::RESET;
                std::string cmd_idx_str;
                std::getline(std::cin, cmd_idx_str);

                try
                {
                    cmd_idx = std::stoi(cmd_idx_str) - 1;
                    return (cmd_idx >= 0 &&
                            cmd_idx < static_cast<int>(commands.size()));
                }
                catch (const std::exception &)
                {
                    return false;
                }
            }
            else
            {
                cmd_idx = 0;
                return true;
            }
        }
        catch (const std::exception &)
        {
            return false;
        }
    }
}

void PathKeeper::saveRecentRecord(Json::Value &config,
                                  const std::string &directory, int cmd_idx)
{
    int orig_idx = -1;
    for (size_t i = 0; i < file.path_keys_order.size(); i++)
    {
        if (file.path_keys_order[i] == directory)
        {
            orig_idx = i;
            break;
        }
    }

    if (orig_idx != -1)
    {
        Json::Value recent(Json::arrayValue);
        recent.append(orig_idx);
        recent.append(cmd_idx);
        config["recent"] = recent;
        file.saveConfig(config);
    }
}

void PathKeeper::processIndexSelection(const std::string &index_str,
                                       Json::Value &paths, Json::Value &config,
                                       bool execute_command, bool set_recent)
{
    std::vector<std::string> valid_dirs = file.get_valid_directories(paths);
    std::string directory;
    int cmd_idx;

    if (!parseIndex(index_str, valid_dirs, paths, directory, cmd_idx))
    {
        std::cout << Colors::RED
                  << QCoreApplication::translate("processIndexSelection",
                                                 "无效编号!")
                         .toStdString()
                  << Colors::RESET << std::endl;
        return;
    }

    Json::Value commands = paths[directory];
    std::string command = commands[cmd_idx].asString();

    if (execute_command)
    {
        if (set_recent)
        {
            saveRecentRecord(config, directory, cmd_idx);
        }

        runCommand(directory, command);
    }
    else
    {
        if (set_recent)
        {
            saveRecentRecord(config, directory, cmd_idx);
        }

        int orig_idx = -1;
        for (size_t i = 0; i < file.path_keys_order.size(); i++)
        {
            if (file.path_keys_order[i] == directory)
            {
                orig_idx = i;
                break;
            }
        }

        if (orig_idx != -1)
        {
            std::cout << QCoreApplication::translate("processIndexSelection",
                                                     "配置完成")
                             .toStdString()
                      << std::endl;
        }
    }
}
