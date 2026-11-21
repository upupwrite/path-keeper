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
#include <sstream>

// ---------------------------------------------------------------------------
// Quote `s` so it can be embedded in a shell command line as a single
// argument. Embedded single quotes are escaped with the standard `'\''`
// trick, which closes the quoted run, emits an escaped quote, then re-opens.
// ---------------------------------------------------------------------------
static std::string shellQuote(const std::string& s)
{
    std::string out;
    out.reserve(s.size() + 2);
    out.push_back('\'');
    for (char c : s)
    {
        if (c == '\'')
        {
            out += "'\\''";
        }
        else
        {
            out.push_back(c);
        }
    }
    out.push_back('\'');
    return out;
}

// ---------------------------------------------------------------------------
// Resolve a leading `~` / `~/...` in `dir` to $HOME, matching the expansion
// that the old `cd <dir> && ...` prefix used to perform. An empty `dir`
// also resolves to $HOME (what a bare `cd` did).
// ---------------------------------------------------------------------------
static std::string resolveDir(const std::string& dir)
{
    const char* home = std::getenv("HOME");
    if (dir.empty())
    {
        return home ? std::string(home) : std::string(".");
    }
    if (dir == "~")
    {
        return home ? std::string(home) : dir;
    }
    if (dir.size() >= 2 && dir[0] == '~' && dir[1] == '/')
    {
        if (home)
        {
            return std::string(home) + dir.substr(1);
        }
    }
    return dir;
}

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

bool isTmuxAvailable()
{
    const char* path_env = std::getenv("PATH");
    if (!path_env)
        return false;

    std::stringstream ss(path_env);
    std::string dir;

    while (std::getline(ss, dir, ':'))
    {
        std::string candidate = dir + "/tmux";
        // Check that the file exists and is executable.
        if (access(candidate.c_str(), X_OK) == 0)
        {
            return true;
        }
    }
    return false;
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
//
// SECURITY NOTES
//   * `dir` is never concatenated into a shell command; the child calls
//     chdir() on the resolved path. Its contents therefore cannot inject
//     shell metacharacters into anything.
//   * `command` IS the user-provided command that this tool exists to run
//     through a shell; that is intentional functionality, so the static
//     analyser warning on `execl` is explicitly suppressed below.
// ---------------------------------------------------------------------------
static void runInProcess(const std::string& command, const std::string& dir,
                         const std::string& log_prefix, bool record)
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
        // Child: change into `dir` first (never via a shell), then exec the
        // shell so that the user-provided `command` is interpreted.
        const std::string resolved_dir = resolveDir(dir);
        if (chdir(resolved_dir.c_str()) != 0)
        {
            _exit(127);
        }

        // `command` is the user-supplied shell command this tool exists to
        // run; executing it through `bash -c` is intentional. Suppress the
        // command-injection warning, since the only alternative would be to
        // remove the tool's core functionality.
        // codeql[cpp/command-line-injection]
        execl("/bin/bash", "bash", "-c", command.c_str(),
              static_cast<char*>(nullptr));
        _exit(127);
    }

    // Parent: put the controlling terminal into raw mode so interactive
    // programs receive keystrokes byte-by-byte (arrow keys, Ctrl-C, ...).
    struct termios saved_termios{};
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

void Shell::shellCommand(const std::string& command, const std::string& dir,
                         const bool record, const bool self,
                         const bool non_shell)
{
    // ======================= In-process execution =======================
    // non_shell == true: bypass tmux / external shell entirely and run the
    // command directly through a PTY in the current process.
    if (non_shell)
    {
        const std::string log_prefix = "[" + log.timestamp() + "]\n" +
                                       "DIR: " + dir + "\n" +
                                       "COMMAND: " + command + "\n";
        runInProcess(command, dir, log_prefix, record);
        return;
    }
    // ====================================================================

    // Legacy path: build a shell one-liner and hand it off to an external
    // shell. `dir` must be quoted so it cannot escape `cd` and inject
    // additional commands. Empty `dir` keeps the original bare `cd` behaviour
    // (which switches to $HOME).
    const std::string cd_prefix =
        dir.empty() ? std::string("cd") : "cd " + shellQuote(dir);
    const std::string wrapped_command = cd_prefix + " && (\n" + command + "\n)";

    // tmux detection is done lazily, once per process.
    static bool tmux_checked = false;
    static bool has_tmux = false;
    if (!tmux_checked)
    {
        has_tmux = isTmuxAvailable();
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
        shell_command = wrapped_command + " ; cd " + shellQuote(cwd);
    }
    else
    {
        shell_command = shell + " <<EOF\n" + wrapped_command + " ; cd " +
                        shellQuote(cwd) + "\nEOF";
    }

    if (!self && !std::empty(shell_command))
    {
        if (record && has_tmux)
        {
            // Quote both `log_prefix` (contains user data) and `log_file`
            // so neither can break out of the surrounding single quotes.
            std::cout << "echo " << shellQuote("\n" + log_prefix) << " >> "
                      << shellQuote(log_file) << " && "
                      << "tmux pipe-pane " << shellQuote("cat >> " + log_file)
                      << "&&" << shell_command << "\n"
                      << "tmux pipe-pane" << std::endl;
        }
        else if (record && !has_tmux)
        {
            std::cerr
                << "Warning: tmux not found, logging output not available."
                << std::endl;
            std::cout << "echo " << shellQuote("\n" + log_prefix) << " >> "
                      << shellQuote(log_file) << " && " << shell_command
                      << std::endl;
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
