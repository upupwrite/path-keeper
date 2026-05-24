#pragma once
#include <termios.h>
#include <pwd.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

class ReadlineHelper
{
public:
    static void initialize()
    {
        rl_readline_name = "PathKeeper";

        // ★ 关键修复：将 readline 的输出（提示符、控制序列）重定向到 stderr，
        // 避免被 shell 包装脚本捕获到 stdout 中。
        rl_outstream = stderr;

#ifdef RL_STATE_COLOR
        rl_colored_stats = 1;
        rl_colored_completion_prefix = 1;
#endif

        rl_bind_key('\t', rl_menu_complete);
        rl_attempted_completion_function = path_keeper_completion;
    }

    static std::string read_line(const std::string& prompt)
    {
        char* line = readline(prompt.c_str());
        if (!line) { return ""; }

        std::string result(line);
        free(line);
        return result;
    }

    static std::vector<std::string> get_system_commands()
    {
        std::vector<std::string> commands;

        static const char* common_commands[] = {
            "ls", "cd", "pwd", "mkdir", "rm", "cp",
            "mv", "cat", "less", "more", "grep", "find",
            "head", "tail", "vim", "nano", "echo", "printf",
            "chmod", "chown", "ps", "kill", "top", "htop",
            "git", "docker", "ssh", "scp", "tar", "gzip",
            "gunzip", "make", "cmake", "g++", "gcc", "python",
            "python3", "node", "npm", "yarn", "java", "javac",
            "systemctl", "journalctl", "apt", "yum", "dnf", "brew"
        };
        commands.insert(commands.end(), std::begin(common_commands), std::end(common_commands));

        char* path_env = getenv("PATH");
        if (path_env) {
            std::string path_str(path_env);
            std::stringstream ss(path_str);
            std::string dir;
            while (std::getline(ss, dir, ':')) {
                try {
                    if (fs::exists(dir) && fs::is_directory(dir)) {
                        for (const auto& entry : fs::directory_iterator(dir)) {
                            if (entry.is_regular_file() && access(entry.path().c_str(), X_OK) == 0)
                                commands.push_back(entry.path().filename().string());
                        }
                    }
                } catch (...) {}
            }
        }

        std::sort(commands.begin(), commands.end());
        commands.erase(std::unique(commands.begin(), commands.end()), commands.end());
        return commands;
    }

private:
    static char* command_generator(const char* text, int state)
    {
        static std::vector<std::string> commands;
        static size_t list_index = 0;
        static size_t text_len = 0;

        if (state == 0) {
            commands = get_system_commands();
            list_index = 0;
            text_len = strlen(text);
        }

        while (list_index < commands.size()) {
            const std::string& cmd = commands[list_index++];
            if (strncmp(cmd.c_str(), text, text_len) == 0)
                return strdup(cmd.c_str());
        }
        return nullptr;
    }

    static char* filename_generator(const char* text, int state)
    {
        static char** matches = nullptr;
        static int match_index = 0;

        if (state == 0) {
            if (matches) {
                for (char** p = matches; *p; p++) free(*p);
                free(matches);
            }
            matches = rl_completion_matches(text, rl_filename_completion_function);
            match_index = 0;
        }

        if (matches && matches[match_index])
            return strdup(matches[match_index++]);
        return nullptr;
    }

    static char** path_keeper_completion(const char* text, int start, int end)
    {
        (void)end;
        rl_attempted_completion_over = 1;

        if (start == 0) {
            if (strchr(text, '/') != nullptr)
                return rl_completion_matches(text, filename_generator);
            else
                return rl_completion_matches(text, command_generator);
        } else {
            return rl_completion_matches(text, filename_generator);
        }
    }




public :
    // 强制恢复终端为规范模式，用于启动外部全屏程序前
    static void reset_terminal()
    {
        struct termios term;
        if (tcgetattr(STDIN_FILENO, &term) != -1)
        {
            term.c_lflag |= (ICANON | ECHO);
            tcsetattr(STDIN_FILENO, TCSANOW, &term);
        }
        // 保险：系统级重置
        system("stty sane");
    }
};



