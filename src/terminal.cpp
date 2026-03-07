#include <pwd.h>
#include <signal.h>

#include <atomic>

#include "pk.h"

// 使用原子变量替代全局变量，避免竞态条件
static std::atomic<pid_t> g_child_pid(-1);

// 信号处理函数
static void handle_sigint(int sig)
{
    (void)sig;
    pid_t child_pid = g_child_pid.load();
    if (child_pid > 0)
    {
        kill(child_pid, SIGINT);
    }
}

int PathKeeper::shellCommand(const std::string& command, const std::string& cwd)
{
    Json::Value config = file.loadConfig();
    std::string myshell = config["shell"].asString();

    // 创建伪终端
    int master_fd = -1, slave_fd = -1;
    if (openpty(&master_fd, &slave_fd, nullptr, nullptr, nullptr) == -1)
    {
        perror("openpty failed");
        return -1;
    }

    // 使用RAII方式确保文件描述符被关闭
    auto close_fds = [&]()
    {
        if (master_fd != -1)
            close(master_fd);
        if (slave_fd != -1)
            close(slave_fd);
    };

    // 获取当前终端大小并设置到伪终端
    struct winsize ws;
    if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) == 0)
    {
        ioctl(master_fd, TIOCSWINSZ, &ws);
    }

    // 保存原始终端设置和信号处理
    struct termios old_settings;
    if (tcgetattr(STDIN_FILENO, &old_settings) == -1)
    {
        perror("tcgetattr failed");
        close_fds();
        return -1;
    }

    struct sigaction old_sigint_action;
    struct sigaction sa;

    pid_t pid = fork();
    if (pid == 0)
    {  // 子进程
        close(master_fd);
        master_fd = -1;

        // 设置新的会话和进程组
        if (setsid() == -1)
        {
            perror("setsid failed");
            exit(EXIT_FAILURE);
        }

        // 设置控制终端为伪终端
        if (ioctl(slave_fd, TIOCSCTTY, 0) == -1)
        {
            perror("ioctl TIOCSCTTY failed");
        }

        // 设置标准输入、输出、错误到伪终端
        dup2(slave_fd, STDIN_FILENO);
        dup2(slave_fd, STDOUT_FILENO);
        dup2(slave_fd, STDERR_FILENO);
        close(slave_fd);
        slave_fd = -1;
        if (!cwd.empty())
        {
            std::string resolved_cwd = cwd;
            // 处理以 ~ 开头的路径（支持 ~ 和 ~/...）
            if (!resolved_cwd.empty() && resolved_cwd[0] == '~')
            {
                const char* home = getenv("HOME");
                if (home == nullptr)
                {
                    struct passwd* pw = getpwuid(getuid());
                    if (pw && pw->pw_dir)
                    {
                        home = pw->pw_dir;
                    }
                    else
                    {
                        perror("Cannot determine home directory");
                        exit(EXIT_FAILURE);
                    }
                }

                if (resolved_cwd == "~")
                {
                    resolved_cwd = home;
                }
                else if (resolved_cwd.size() > 1 && resolved_cwd[1] == '/')
                {
                    // 将 ~/ 替换为 home 目录
                    resolved_cwd = std::string(home) + resolved_cwd.substr(1);
                }
                // 其他形如 ~username 的情况未作处理，保持原样（可根据需要扩展）
            }

            if (chdir(resolved_cwd.c_str()) == -1)
            {
                perror("chdir failed");
                exit(EXIT_FAILURE);
            }
        }
        // 恢复默认的信号处理
        signal(SIGINT, SIG_DFL);

        // 执行命令
        execlp(myshell.c_str(), myshell.c_str(), "-c", command.c_str(),
               nullptr);
        perror("execlp failed");
        exit(EXIT_FAILURE);
    }
    else if (pid > 0)
    {                            // 父进程
        g_child_pid.store(pid);  // 设置原子变量

        close(slave_fd);
        slave_fd = -1;

        // 设置信号处理程序
        sa.sa_handler = handle_sigint;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        if (sigaction(SIGINT, &sa, &old_sigint_action) == -1)
        {
            perror("sigaction failed");
        }

        // 设置非阻塞模式
        int flags = fcntl(master_fd, F_GETFL, 0);
        if (flags != -1)
        {
            fcntl(master_fd, F_SETFL, flags | O_NONBLOCK);
        }

        // 设置原始模式
        struct termios raw_settings = old_settings;
        cfmakeraw(&raw_settings);
        tcsetattr(STDIN_FILENO, TCSANOW, &raw_settings);

        // 主循环处理用户输入和子进程输出
        char buffer[1024];
        bool process_running = true;

        while (process_running)
        {
            fd_set read_fds;
            FD_ZERO(&read_fds);
            FD_SET(STDIN_FILENO, &read_fds);
            FD_SET(master_fd, &read_fds);

            struct timeval timeout;
            timeout.tv_sec = 0;
            timeout.tv_usec = 100000;  // 100ms

            int max_fd = std::max(STDIN_FILENO, master_fd) + 1;
            int ready = select(max_fd, &read_fds, nullptr, nullptr, &timeout);

            if (ready == -1)
            {
                if (errno == EINTR)
                    continue;
                break;
            }

            // 处理子进程输出
            if (FD_ISSET(master_fd, &read_fds))
            {
                ssize_t n = read(master_fd, buffer, sizeof(buffer));
                if (n > 0)
                {
                    write(STDOUT_FILENO, buffer, n);
                }
                else if (n == 0)
                {
                    // EOF - 子进程可能已退出
                    break;
                }
                else if (errno != EAGAIN && errno != EWOULDBLOCK)
                {
                    break;
                }
            }

            // 处理用户输入
            if (FD_ISSET(STDIN_FILENO, &read_fds))
            {
                ssize_t n = read(STDIN_FILENO, buffer, sizeof(buffer));
                if (n > 0)
                {
                    // 检查是否是Ctrl+C (ASCII 3)
                    if (n == 1 && buffer[0] == 0x03)
                    {
                        kill(pid, SIGINT);
                    }
                    else
                    {
                        write(master_fd, buffer, n);
                    }
                }
                else if (n == 0)
                {
                    break;  // EOF
                }
            }

            // 检查进程是否结束
            int status;
            pid_t result = waitpid(pid, &status, WNOHANG);
            if (result == pid)
            {
                process_running = false;
            }
            else if (result == -1)
            {
                process_running = false;
            }
        }

        // 恢复终端设置
        tcsetattr(STDIN_FILENO, TCSANOW, &old_settings);

        // 恢复原来的信号处理
        sigaction(SIGINT, &old_sigint_action, nullptr);
        g_child_pid.store(-1);

        // 获取子进程退出状态
        int status;
        waitpid(pid, &status, 0);  // 确保回收子进程
        close_fds();

        return WIFEXITED(status)     ? WEXITSTATUS(status)
               : WIFSIGNALED(status) ? 128 + WTERMSIG(status)
                                     : -1;
    }
    else
    {
        perror("fork failed");
        close_fds();
        return -1;
    }
}
