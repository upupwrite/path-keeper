#include "pk.h"

#include <QCoreApplication>
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include <fcntl.h>   // for open
#include <unistd.h>  // for dup, dup2, close
#include "colors.h"
#include "json/value.h"
#include "readline.h"

PathKeeper::PathKeeper()
{
    char buffer[1024];
    if (getcwd(buffer, sizeof(buffer)) != nullptr)
        cwd = buffer;
    else
        cwd = ".";

    file.load_key_order();
    logger = Logger(file.loadConfig());
}

void PathKeeper::outputCommand(const std::string& cmd)
{
    if (!cmd.empty())
        std::cout << cmd << std::endl;
    else
        std::cerr << Colors::YELLOW
                  << QCoreApplication::translate("PathKeeper",
                                                 "无法生成命令")
                         .toStdString()
                  << Colors::RESET << std::endl;
}

std::string PathKeeper::getInputIndex(const std::string& provided_index,
                                      const std::string& prompt)
{
    if (!provided_index.empty())
        return provided_index;

    std::cerr << Colors::CYAN << prompt << Colors::RESET;
    std::string index_str;
    std::getline(std::cin, index_str);
    return index_str;
}

bool PathKeeper::parseIndex(const std::string& index_str,
                            std::vector<std::string>& valid_dirs,
                            Json::Value& paths,
                            std::string& directory,
                            int& cmd_idx)
{
    if (index_str.find('.') != std::string::npos) {
        size_t dot = index_str.find('.');
        std::string part1 = index_str.substr(0, dot);
        std::string part2 = index_str.substr(dot + 1);
        try {
            int display_num = std::stoi(part1);
            cmd_idx = std::stoi(part2) - 1;
            if (display_num < 1 || display_num > (int)valid_dirs.size())
                return false;
            directory = valid_dirs[display_num - 1];
            Json::Value cmds = paths[directory]["cmds"];
            return (cmd_idx >= 0 && cmd_idx < (int)cmds.size());
        } catch (...) {
            return false;
        }
    } else {
        try {
            int display_num = std::stoi(index_str);
            if (display_num < 1 || display_num > (int)valid_dirs.size())
                return false;
            directory = valid_dirs[display_num - 1];
            Json::Value cmds = paths[directory]["cmds"];
            if (cmds.size() > 1) {
                std::cerr << Colors::GREEN
                          << QCoreApplication::translate("parseIndex", "目录:")
                                 .toStdString()
                          << Colors::RESET << " " << directory << std::endl;
                displayCommands(cmds);
                std::cerr << Colors::CYAN
                          << QCoreApplication::translate("parseIndex", "请选择命令编号: ")
                                 .toStdString()
                          << Colors::RESET;
                std::string cmd_idx_str;
                std::getline(std::cin, cmd_idx_str);
                try {
                    cmd_idx = std::stoi(cmd_idx_str) - 1;
                    return (cmd_idx >= 0 && cmd_idx < (int)cmds.size());
                } catch (...) {
                    return false;
                }
            } else {
                cmd_idx = 0;
                return true;
            }
        } catch (...) {
            return false;
        }
    }
}

void PathKeeper::saveRecentRecord(Json::Value& config,
                                  const std::string& directory,
                                  int cmd_idx)
{
    int orig_idx = -1;
    for (size_t i = 0; i < file.path_keys_order.size(); i++) {
        if (file.path_keys_order[i] == directory) {
            orig_idx = i;
            break;
        }
    }
    if (orig_idx != -1) {
        Json::Value recent(Json::arrayValue);
        recent.append(orig_idx);
        recent.append(cmd_idx);
        config["recent"] = recent;
        file.saveConfig(config);
    }
}

void PathKeeper::processIndexSelection(const std::string& index_str,
                                       Json::Value& paths,
                                       Json::Value& config,
                                       bool output_command,
                                       bool set_recent)
{
    std::vector<std::string> valid_dirs = file.get_valid_directories(paths);
    std::string directory;
    int cmd_idx;
    if (!parseIndex(index_str, valid_dirs, paths, directory, cmd_idx)) {
        std::cerr << Colors::RED
                  << QCoreApplication::translate("processIndexSelection", "无效编号!")
                         .toStdString()
                  << Colors::RESET << std::endl;
        return;
    }

    Json::Value cmds = paths[directory]["cmds"];
    Json::Value cmdEntry = cmds[cmd_idx];
    std::string command = File::getCommandString(cmdEntry);
    bool log_set = false, log_value = false;
    File::getCommandLogFlag(cmdEntry, log_set, log_value);

    if (set_recent)
        saveRecentRecord(config, directory, cmd_idx);

    if (output_command) {
        std::string final_cmd = CommandBuilder::build(directory, command, logger,
                                                      log_set, log_value);
        outputCommand(final_cmd);
    } else {
        std::cerr << QCoreApplication::translate("processIndexSelection", "配置完成").toStdString()
                  << std::endl;
    }
}

// -------- 公有方法 ---------




void PathKeeper::addRecord()
{
    Json::Value config = file.loadConfig();
    static bool readline_init = false;
    if (!readline_init) {
        ReadlineHelper::initialize();
        readline_init = true;
    }

    std::string directory = ReadlineHelper::read_line(
        Colors::CYAN +
        QCoreApplication::translate("addRecord", "请输入记录目录:").toStdString() +
        Colors::RESET);
    if (directory.empty()) {
        std::cerr << Colors::YELLOW
                  << QCoreApplication::translate("addRecord", "目录不能为空!").toStdString()
                  << Colors::RESET << std::endl;
        return;
    }
    if (directory == ".") directory = cwd;

    Json::Value& path_obj = config["path"];
    if (!path_obj.isMember(directory))
        path_obj[directory] = Json::Value(Json::objectValue);
    Json::Value& entry = path_obj[directory];
    if (!entry.isMember("cmds"))
        entry["cmds"] = Json::arrayValue;

    Json::Value& cmds = entry["cmds"];
    if (fs::exists(directory) && fs::is_directory(directory)) {
        std::cerr << Colors::GREEN << directory << Colors::RESET << std::endl;
        displayCommands(cmds);
    }

    std::string cmd = ReadlineHelper::read_line(
        Colors::CYAN +
        QCoreApplication::translate("addRecord", "请输入命令 (回车选择单行/编辑器):").toStdString() +
        Colors::RESET);

    if (cmd.empty()) {
        std::string choice = ReadlineHelper::read_line(
            Colors::CYAN +
            std::string("按 'e' 打开编辑器，其他键输入单行: ") +
            Colors::RESET);
        if (choice == "e" || choice == "E") {
            // ---------- 编辑器模式 ----------
            char tmpname[] = "/tmp/pk_cmd_XXXXXX";
            int fd = mkstemp(tmpname);
            if (fd == -1) {
                std::cerr << Colors::RED << "无法创建临时文件" << Colors::RESET << std::endl;
                return;
            }
            close(fd);

            Editor editor;
            std::string editor_cmd = editor.getEditor();
            if (editor_cmd.empty()) {
                std::cerr << Colors::YELLOW << "未设置编辑器，请先执行 pk config -editor" << Colors::RESET << std::endl;
                unlink(tmpname);
                return;
            }

            if (access(editor_cmd.c_str(), X_OK) != 0) {
                std::cerr << Colors::RED << "编辑器 '" << editor_cmd << "' 不存在或无权执行！" << Colors::RESET << std::endl;
                unlink(tmpname);
                return;
            }

            // ★★★ 1. 强制恢复终端规范模式
            rl_deprep_terminal();
            ReadlineHelper::reset_terminal();

            // ★★★ 2. 将 stdout 重定向到真实终端，解决 shell 管道捕获导致的“输出不是终端”问题
            int stdout_backup = dup(STDOUT_FILENO);      // 备份原 stdout（管道）
            int tty_fd = open("/dev/tty", O_WRONLY);
            if (tty_fd != -1) {
                dup2(tty_fd, STDOUT_FILENO);
                close(tty_fd);
            }

            std::cerr << Colors::GREEN << "正在启动编辑器: " << editor_cmd << Colors::RESET << std::endl;
            std::string sys_cmd = editor_cmd + " " + tmpname;
            int ret = system(sys_cmd.c_str());

            // ★★★ 3. 恢复原 stdout（管道），避免影响后续输出
            if (stdout_backup != -1) {
                dup2(stdout_backup, STDOUT_FILENO);
                close(stdout_backup);
            }

            if (ret == -1) {
                std::cerr << Colors::RED << "调用编辑器失败 (fork 失败)" << Colors::RESET << std::endl;
                unlink(tmpname);
                return;
            }
            if (ret != 0) {
                std::cerr << Colors::RED << "编辑器异常退出 (返回码: " << ret << ")" << Colors::RESET << std::endl;
                unlink(tmpname);
                return;
            }

            // 读取编辑器保存的命令
            std::ifstream ifs(tmpname);
            std::stringstream buffer;
            buffer << ifs.rdbuf();
            cmd = buffer.str();
            unlink(tmpname);

            if (cmd.empty()) {
                std::cerr << Colors::YELLOW << "编辑器未保存任何内容，命令未添加。" << Colors::RESET << std::endl;
                return;
            }

            // 编辑器模式直接保存为普通字符串（日志跟随全局设置）
            cmds.append(cmd);
            file.saveConfig(config);
            std::cerr << Colors::GREEN
                      << QCoreApplication::translate("addRecord", "记录已保存!").toStdString()
                      << Colors::RESET << std::endl;
            return;   // 直接返回，不进入后续的日志询问
        }
        else {
            // 单行模式，仍然用 readline 获取命令
            cmd = ReadlineHelper::read_line(
                Colors::CYAN +
                QCoreApplication::translate("addRecord", "请输入单行命令:").toStdString() +
                Colors::RESET);
        }
    }

    // 单行命令的默认值与日志询问（保持原有逻辑）
    if (cmd.empty() && cmds.empty()) {
        cmd = "ls -l";
        std::cerr << QCoreApplication::translate("addRecord", "使用默认命令: ").toStdString()
                  << cmd << std::endl;
    }
    if (!cmd.empty()) {
        std::string logChoice = ReadlineHelper::read_line(
            Colors::CYAN +
            std::string("为该命令单独设置日志记录? (y=强制记录 / n=强制不记录 / 回车=跟随全局): ") +
            Colors::RESET);
        if (logChoice == "y" || logChoice == "Y") {
            Json::Value obj;
            obj["cmd"] = cmd;
            obj["log"] = true;
            cmds.append(obj);
        } else if (logChoice == "n" || logChoice == "N") {
            Json::Value obj;
            obj["cmd"] = cmd;
            obj["log"] = false;
            cmds.append(obj);
        } else {
            cmds.append(cmd);
        }
    }

    file.saveConfig(config);
    std::cerr << Colors::GREEN
              << QCoreApplication::translate("addRecord", "记录已保存!").toStdString()
              << Colors::RESET << std::endl;
}

void PathKeeper::showRecord()
{
    Json::Value config = file.loadConfig();
    Json::Value paths = config["path"];
    if (paths.empty()) {
        std::cerr << Colors::YELLOW
                  << QCoreApplication::translate("showRecord", "没有记录!").toStdString()
                  << Colors::RESET << std::endl;
        return;
    }

    int i = 1;
    std::vector<std::string> valid_dirs = file.get_valid_directories(paths);
    for (const auto& dir : valid_dirs) {
        std::cerr << Colors::BLUE << "[" << i << "]" << Colors::RESET
                  << " " << Colors::GREEN << dir << Colors::RESET << std::endl;
        Json::Value cmds = paths[dir]["cmds"];
        displayCommands(cmds, i);
        i++;
    }
    displayRecentMark(config, paths);
}

void PathKeeper::outputRecentCommand()
{
    Json::Value config = file.loadConfig();
    if (config["recent"].isNull() || !config["recent"].isArray() || config["recent"].size() != 2) {
        std::cerr << Colors::YELLOW
                  << QCoreApplication::translate("runRecent", "没有最近记录!").toStdString()
                  << Colors::RESET << std::endl;
        return;
    }

    int orig_idx1 = config["recent"][0].asInt();
    int idx2 = config["recent"][1].asInt();
    Json::Value paths = config["path"];
    if (orig_idx1 < 0 || orig_idx1 >= (int)file.path_keys_order.size()) {
        std::cerr << "Recent invalid" << std::endl;
        return;
    }
    std::string directory = file.path_keys_order[orig_idx1];
    if (!paths.isMember(directory)) {
        std::cerr << "Recent directory missing" << std::endl;
        return;
    }
    Json::Value cmds = paths[directory]["cmds"];
    if (idx2 < 0 || idx2 >= (int)cmds.size()) {
        std::cerr << "Recent command invalid" << std::endl;
        return;
    }
    Json::Value cmdEntry = cmds[idx2];
    std::string command = File::getCommandString(cmdEntry);
    bool log_set = false, log_value = false;
    File::getCommandLogFlag(cmdEntry, log_set, log_value);
    std::string final_cmd = CommandBuilder::build(directory, command, logger,
                                                  log_set, log_value);
    outputCommand(final_cmd);
}

void PathKeeper::setRecent()
{
    Json::Value config = file.loadConfig();
    Json::Value paths = config["path"];
    if (paths.empty()) {
        std::cerr << "No records" << std::endl;
        return;
    }
    showRecord();
    std::string index_str = getInputIndex(
        "", QCoreApplication::translate("setRecent", "请输入目标编号: ").toStdString());
    if (index_str.empty()) return;
    processIndexSelection(index_str, paths, config, false, true);
}

void PathKeeper::selectRun(const std::string& cmd_index, bool set_recent, bool show)
{
    Json::Value config = file.loadConfig();
    if (show) showRecord();
    Json::Value paths = config["path"];
    if (paths.empty()) {
        std::cerr << Colors::YELLOW
                  << QCoreApplication::translate("selectRun", "没有记录!").toStdString()
                  << Colors::RESET << std::endl;
        return;
    }
    std::string index_str = getInputIndex(
        cmd_index, QCoreApplication::translate("selectRun", "请输入要执行的编号: ").toStdString());
    if (index_str.empty()) {
        outputRecentCommand();
        return;
    }
    processIndexSelection(index_str, paths, config, true, set_recent);
}

void PathKeeper::runPoint(const std::string& cmd_index)
{
    selectRun(cmd_index, false, cmd_index.empty());
}

void PathKeeper::displayCommands(const Json::Value& commands, int parent_index)
{
    for (Json::ArrayIndex j = 0; j < commands.size(); j++) {
        std::string cmd_str = File::getCommandString(commands[j]);
        std::string display = cmd_str;
        if (cmd_str.find('\n') != std::string::npos)
            display = cmd_str.substr(0, cmd_str.find('\n')) + " [M]";
        if (parent_index > 0)
            std::cerr << "    " << Colors::CYAN << "[" << parent_index << "."
                      << (j + 1) << "]" << Colors::RESET << " " << display << std::endl;
        else
            std::cerr << "    " << Colors::CYAN << "[" << (j + 1) << "]"
                      << Colors::RESET << " " << display << std::endl;
    }
}

void PathKeeper::displayRecentMark(const Json::Value& config, const Json::Value& paths)
{
    if (!config["recent"].isNull() && config["recent"].isArray() && config["recent"].size() == 2) {
        int orig_idx1 = config["recent"][0].asInt();
        int idx2 = config["recent"][1].asInt();
        int display_num = file.get_display_number_by_directory_index(orig_idx1, paths);
        if (display_num != -1) {
            std::string recent_dir = file.path_keys_order[orig_idx1];
            Json::Value cmds = paths[recent_dir]["cmds"];
            if (idx2 >= 0 && idx2 < (int)cmds.size())
                std::cerr << Colors::YELLOW
                          << QCoreApplication::translate("displayRecentMark", "最近执行").toStdString()
                          << ": [" << display_num << "." << (idx2 + 1) << "] "
                          << recent_dir << Colors::RESET << std::endl;
        }
    }
}
