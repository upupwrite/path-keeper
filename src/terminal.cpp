// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Path Keeper Contributors
// This file is part of Path Keeper.
// Path Keeper is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// Path Keeper is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
// You should have received a copy of the GNU General Public License
// along with Path Keeper. If not, see <https://www.gnu.org/licenses/>.

#include "terminal.h"

#include <fcntl.h>
#include <poll.h>
#include <pty.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#include <cstdlib>
#include <fstream>
#include <iostream>

Shell::Shell()
{
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

// ---------------------------------------------------------------------------
// Execute a command directly inside the current process via a pseudo-terminal.
//
// Using a PTY (instead of popen/pipe) is required so that:
//   * isatty() returns true inside the child (keeps color output alive)
//   * True-color escape sequences are emitted by modern CLI tools
//   * Interactive programs (less, vim, ...) can receive keystrokes
//
// Output is streamed in real time to stdout and, when `record` is true, also
// appended to Achieve::LOG_FILE.
// ---------------------------------------------------------------------------
static void runInProcess(const std::string &exec_command,
                         const std::string &log_prefix, bool record)
{
    // Tell children they are running on a true-color capable terminal.
    setenv("COLORTERM", "truecolor", 1);
    setenv("TERM", "xterm-256color", 1);
    setenv("CLICOLOR_FORCE", "1", 1);
    setenv("FORCE_COLOR", "1", 1);

    // Open the log file for appending when recording is enabled.
    std::ofstream log_stream;
    if (record)
    {
        log_stream.open(Achieve::LOG_FILE, std::ios::app);
        if (log_stream.is_open())
        {
            log_stream << log_prefix << std::flush;
        }
    }

    // Inherit the parent terminal's window size for the child PTY.
    struct winsize ws = {};
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);

    int master_fd = -1;
    const pid_t pid = forkpty(&master_fd, nullptr, nullptr, &ws);
    if (pid < 0)
    {
        std::cerr << "Error: forkpty() failed." << std::endl;
        return;
    }

    if (pid == 0)
    {
        // Child: replace the image with /bin/sh -c "<command>".
        execl("/bin/bash", "bash", "-c", exec_command.c_str(),
              static_cast<char *>(nullptr));
        _exit(127);
    }

    // Parent: put the controlling terminal into raw mode so interactive
    // programs receive keystrokes byte-by-byte (arrow keys, Ctrl-C, ...).
    struct termios saved_termios
    {
    };
    bool termios_saved = false;
    const bool stdin_is_tty = isatty(STDIN_FILENO);
    if (stdin_is_tty && tcgetattr(STDIN_FILENO, &saved_termios) == 0)
    {
        struct termios raw_termios = saved_termios;
        cfmakeraw(&raw_termios);
        if (tcsetattr(STDIN_FILENO, TCSANOW, &raw_termios) == 0)
        {
            termios_saved = true;
        }
    }

    // Multiplex stdin (parent -> child) and PTY output (child -> parent).
    struct pollfd fds[2] = {
        {STDIN_FILENO, POLLIN, 0},
        {master_fd, POLLIN, 0},
    };
    bool stdin_open = stdin_is_tty;
    char buffer[4096];

    while (true)
    {
        if (!stdin_open)
        {
            // Disable polling on stdin once EOF is reached.
            fds[0].fd = -1;
        }

        const int ret = poll(fds, 2, -1);
        if (ret < 0)
        {
            break;
        }

        // Forward user input to the child PTY.
        if (fds[0].revents & POLLIN)
        {
            const ssize_t n = read(STDIN_FILENO, buffer, sizeof(buffer));
            if (n > 0)
            {
                if (write(master_fd, buffer, static_cast<size_t>(n)) < 0)
                {
                    // The child may have already exited; ignore.
                }
            }
            else
            {
                stdin_open = false;
                fds[0].fd = -1;
            }
        }

        // Forward PTY output to stdout, and mirror it to the log file.
        if (fds[1].revents & POLLIN)
        {
            const ssize_t n = read(master_fd, buffer, sizeof(buffer));
            if (n > 0)
            {
                std::cout.write(buffer, n);
                std::cout.flush();
                if (log_stream.is_open())
                {
                    log_stream.write(buffer, n);
                    log_stream.flush();
                }
            }
            else
            {
                break;
            }
        }

        if (fds[1].revents & (POLLHUP | POLLERR | POLLNVAL))
        {
            break;
        }
    }

    // Restore original terminal settings before returning control.
    if (termios_saved)
    {
        tcsetattr(STDIN_FILENO, TCSANOW, &saved_termios);
    }

    close(master_fd);

    int status = 0;
    waitpid(pid, &status, 0);

    if (log_stream.is_open())
    {
        log_stream.close();
    }
}

void Shell::shellCommand(const std::string &command, const std::string &dir,
                         const bool record, const bool self,
                         const bool non_shell)
{
    // 用子 shell 包裹命令，保证：
    //   * 编辑器返回的多行命令整体都在 dir 中执行；
    //   * cd 失败时整块命令不再执行，避免在错误目录下运行后续行。
    const std::string wrapped_command =
        "cd " + dir + " && (\n" + command + "\n)";

    // ======================= In-process execution =======================
    // non_shell == true: bypass tmux / external shell entirely and run the
    // command directly through a PTY in the current process. No tmux check
    // is performed and output is streamed in real time with true color.
    if (non_shell)
    {
        const std::string exec_command = wrapped_command;
        const std::string log_prefix = "[" + log.timestamp() + "]\n" +
                                       "DIR: " + dir + "\n" +
                                       "COMMAND: " + command + "\n";
        runInProcess(exec_command, log_prefix, record);
        return;
    }
    // ====================================================================

    // Legacy path: hand the command off to an external shell, optionally
    // using tmux for capture. tmux detection is done lazily, once per process.
    static bool tmux_checked = false;
    static bool has_tmux = false;
    if (!tmux_checked)
    {
        has_tmux = (system("which tmux > /dev/null 2>&1") == 0);
        tmux_checked = true;
    }

    std::string log_prefix = "[" + log.timestamp() + "]\n" + "DIR: " + dir +
                             "\n" + "COMMAND: " + command + "\n";
    Json::Value config = file.loadConfig();
    std::string shell = config["shell"].asString();
    std::string log_file = Achieve::LOG_FILE;
    std::string shell_command;

    if (std::empty(shell))
    {
        shell_command = wrapped_command + " ; cd " + cwd;
    }
    else
    {
        shell_command = shell + " <<EOF\n" + wrapped_command + " ; cd " +
                        cwd + "\nEOF";
    }

    if (!self && !std::empty(shell_command))
    {
        if (record && has_tmux)
        {
            std::cout << "echo '\n"
                      << log_prefix << "' >> " << log_file << " && "
                      << "tmux pipe-pane 'cat >> " << log_file << "'&&"
                      << shell_command << "\n"
                      << "tmux pipe-pane" << std::endl;
        }
        else if (record && !has_tmux)
        {
            std::cerr
                << "Warning: tmux not found, logging output not available."
                << std::endl;
            std::cout << "echo '\n"
                      << log_prefix << "' >> " << log_file << " && "
                      << shell_command << std::endl;
        }
        else
        {
            std::cout << shell_command << std::endl;
        }
    }
    else
    {
        std::cout << shell_command << std::endl;
    }
}