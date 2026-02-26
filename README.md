# P(ath) K(eeper) Command Tool - 多功能命令行工具
## 项目概述

PathKeeper 是一个命令行工具，用于：

- 记录常用命令
- 快速切换和执行命令
- 支持配置终端类型

## 功能特性

- **命令记录**：可添加、查看、执行历史记录 
- **多级分类**：支持命令分类管理（如 1.2 表示第一类第二条命令）
- **配置管理**：可配置默认执行终端 


## 使用技巧

1. **快速执行**：使用 `-p` 参数可以快速执行最近使用的命令 
2. **分类管理**：添加命令时合理分类，便于后续查找

## 示例工作流

```bash
#执行默认命令
pk

#添加开发相关命令
pk -a

#查看所有记录
pk -s

#执行第一条命令
pk -e 1
```

## 注意事项

1. 命令记录保存在本地文件(`~ /.pk.json`)中，注意备份重要记录 
2. 使用 `-e` 参数时，请确保序号有效

## 安装说明

```bash
# 安装依赖
# Ubuntu/Debian:
sudo apt update
sudo apt install cmake pkg-config libjsoncpp-dev build-essential

# CentOS/RHEL
sudo yum install cmake pkgconfig jsoncpp-devel gcc-c++

# Fedora
sudo dnf install cmake pkgconfig jsoncpp-devel gcc-c++

# 编译项目
mkdir build && cd build
cmake ..
make

# 安装到系统（需要sudo权限）
sudo make install
```
## 卸载方法

```bash
# 在build下运行
sudo make uninstall
```

或者手动卸载：

```bash
# 手动删除安装的文件
sudo rm /usr/local/bin/pk
```


## 贡献指南

欢迎贡献代码和改进建议！请遵循开源社区规范，并提交 PR 或 Issue。

## 开源许可

本项目遵循开源协议，详细请查看项目根目录下的 `LICENSE` 文件。

如需进一步了解代码细节，可查看对应模块的源码。
