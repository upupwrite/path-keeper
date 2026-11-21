# P(ath) K(eeper) 命令工具 - 多功能命令行工具

## 项目概述

Path-keeper 是一个功能强大的命令行工具，用于轻松管理、组织和执行常用命令。它具有多级分类、执行日志记录、shell别名以及丰富的自定义选项。

### 主要改进

此版本包含重要的错误修复和新功能：
- 修复了缺失的方法实现
- 解决了 shell 脚本集成问题
- 改进了错误处理和参数传递
- 增强了交互式命令检测
- 稳定了编辑器模式和多行支持

## 功能特性

- 命令记录：添加、查看和执行命令历史
- 多级分类：支持命令分类（例如，1.2 表示第一个分类下的第二条命令）
- 配置管理：可配置默认执行 shell 和编辑器
- 最近命令执行：快速重新运行最近执行过的命令
- Shell 集成：与 bash/zsh 无缝集成，自动检测交互式程序
- 执行日志记录：可选为每条命令记录带时间戳和输出捕获的日志
- 交互式搜索：使用 fzf 进行模糊查找命令选择
- 哈希验证：验证命令完整性，检测配置篡改
- Shell 别名：为常用命令索引创建便捷的 shell 别名
- 多行命令：支持基于编辑器的多行命令输入
- 灵活日志控制：每条命令可单独覆盖日志设置，同时支持全局默认设置
- 依赖 fzf 和 tmux（如需使用搜索和命令日志记录功能）

## 使用提示

1. 快速执行：使用 `-p` 参数执行命令时不更新最近记录
2. 分类管理：添加命令时合理分类，便于后续检索
3. 多行命令：添加命令时直接回车不输入内容，即进入编辑器模式
4. Shell 集成：在 shell 中 source pk.sh 脚本，确保命令正常执行和终端处理

## 示例工作流

```bash
# 查看当前目录并运行最近命令（如果已设置）
pk

# 添加新命令记录
pk -a

# 查看所有记录
pk -s

# 执行第一个分类下的命令
pk -e 1

# 执行特定命令（例如：第1个分类下的第2条命令）
pk -e 1.2

# 执行但不更新最近记录
pk -p 1

# 设置最近命令以便快速访问
pk -c
```

## 命令参考

### 主要选项

```
-a, --add           添加新命令记录（单行或多行编辑器模式）
-e [索引]           按索引执行命令并更新最近记录
-p [索引]           按索引执行命令但不更新最近记录
-s, --show          按分类显示所有已记录的命令
-c, --configure     设置最近执行的命令记录
-v, --version       显示版本信息
-V, --version-verbose
                     显示详细版本和构建信息
-h, --help          显示详细帮助信息
```

### 子命令

```
配置管理：
  config             在默认编辑器中打开配置文件
  config -editor     交互式设置编辑器（或直接指定）
  config -editor vim 将编辑器设置为特定命令

高级功能：
  alias add <名称> <索引>   添加别名以快速执行命令
  alias remove <名称>       删除别名
  alias list                列出所有别名
  alias install             生成 shell 别名文件（~/.pk_aliases.sh）
  search                    使用 fzf 交互式搜索
  verify                    验证命令完整性
  rehash                    重新生成并保存命令哈希值
  log                       列出并查看日志文件
```

## 每条命令的日志控制

添加命令时，可以选择日志行为：

```
y（强制记录）：       始终记录命令输出
n（强制不记录）：     从不记录命令输出
空/回车：            使用全局 log.enabled 设置
```

## Shell 集成

pk.sh 脚本提供以下功能：

- 自动处理 stdout/stderr：正确捕获并执行命令输出
- 交互式程序检测：自动检测 vim、nano、emacs、less、more、htop、man
- 健壮的命令提取：处理日志包装命令并移除调试输出
- 终端兼容性：支持 bash、zsh、dash、ksh 和 fish

通过在 shell 配置文件中 source pk.sh 来启用 shell 集成：

```bash
source /usr/local/share/path-keeper/pk.sh
```

## 安装说明

### 依赖要求

```bash
# Ubuntu/Debian:
sudo apt update
sudo apt install cmake pkg-config libjsoncpp-dev build-essential
sudo apt install qt5-qmake qt5-default

# CentOS/RHEL:
sudo yum install cmake pkgconfig jsoncpp-devel gcc-c++
sudo yum install qt5-devel

# Fedora:
sudo dnf install cmake pkgconfig jsoncpp-devel gcc-c++
sudo dnf install qt5-devel
```
> [!TIP]
> 本程序需要 fzf 和 tmux。为了记录输出，应在 tmux 会话中执行。

安装 fzf 和 tmux：
```bash
sudo apt install fzf tmux
```

### 构建与安装

克隆仓库并构建：

```bash
git clone https://github.com/upupwrite/path-keeper.git
cd path-keeper
mkdir build && cd build
cmake ..
make
```

安装到系统（需要 sudo 权限）：

```bash
sudo make install
```

启用 shell 集成，添加到 ~/.bashrc 或 ~/.zshrc：

```bash
source /usr/local/share/path-keeper/pk.sh
```

## 安全特性

哈希验证：

验证命令配置未被篡改：

```bash
pk verify              # 检查命令完整性
pk rehash              # 手动编辑后重新生成哈希值
```

日志文件管理：

- 带时间戳的日志条目，提供审计追踪
- 每条命令的执行跟踪

## 国际化

Path-keeper 通过 Qt 翻译支持多种语言：

- 英语（en）
- 简体中文（zh_CN）

应用程序会自动检测系统区域设置并加载相应的翻译。

## 测试

构建并运行测试（如果可用）：

```bash
cd build
cmake .. -DBUILD_TESTS=ON
make
ctest
```

## 卸载方法

在构建目录中运行：

```bash
sudo make uninstall
```

或者手动卸载：

```bash
sudo rm /usr/local/bin/pk
sudo rm -rf /usr/local/share/path-keeper
```

或者手动删除已安装文件：

```bash
sudo rm /usr/local/bin/pk
```

## 故障排除

问题："pk: binary not found"
解决方法：确保 /usr/local/bin 在 PATH 中

```bash
echo $PATH
which pk
```

问题："No editors found"
解决方法：安装一个常见编辑器

```bash
sudo apt install vim    # 或 nano、emacs 等
```

问题："Failed to load translation"
解决方法：验证 Qt5/Qt6 安装

```bash
pkg-config --modversion Qt5Core
```

问题：命令无法执行
解决方法：检查 shell 集成是否已 source

```bash
grep "source.*pk.sh" ~/.bashrc
# 如果未找到，则添加到 ~/.bashrc：
echo "source /usr/local/share/path-keeper/pk.sh" >> ~/.bashrc
source ~/.bashrc
```

问题：日志文件未创建
解决方法：确保日志目录存在且可写

```bash
mkdir -p ~/.pk_logs
chmod 700 ~/.pk_logs
```

## 注意事项

1. 命令记录保存在本地文件（~/.pk.json）中。请记得备份重要记录。
2. 使用 -e 参数时，请确保索引有效。
3. 添加命令时，可以为每条命令选择日志行为。
4. 多行命令可以使用编辑器模式输入。
5. 记录的命令中可以使用 shell 变量和别名。
6. 环境变量从记录中指定的目录继承。

## 高级功能

搜索与过滤：

使用 search 子命令进行模糊查找命令选择：

```bash
pk search              # 使用 fzf 交互式搜索（需要 fzf）
```

查看日志：

列出并查看执行日志：

```bash
pk log                 # 显示可用的日志文件
```

## 贡献指南

欢迎贡献和改进建议！请遵守开源社区规范，提交 PR 或 Issue。

## 开源许可证

本项目遵循开源许可证。详情请参阅项目根目录下的 LICENSE 文件。

本项目采用 GNU 通用公共许可证 v3.0（GPLv3）

详情请参阅项目根目录下的 LICENSE 文件。

更多关于 GPLv3 的信息：https://www.gnu.org/licenses/gpl-3.0.html

## 支持与更多信息

关于代码的更多细节，您可以查阅相应模块的源代码。

GitHub：https://github.com/upupwrite/path-keeper
作者：upupwrite