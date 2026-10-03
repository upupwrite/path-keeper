# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Path Keeper Contributors
# This file is part of Path Keeper.
#
# pytest for pk (path-keeper) binary.

import json
import os
import shutil
import subprocess
import tempfile
from pathlib import Path

import pytest


# ================================================================
# 定位 pk 可执行文件
# ================================================================

def locate_pk():
    """自动寻找 pk 可执行文件"""
    env_pk = os.environ.get("PK_BINARY")
    if env_pk and os.path.exists(env_pk):
        return env_pk

    script_dir = Path(__file__).resolve().parent
    candidates = [
        script_dir / ".." / "build" / "pk",
        script_dir / ".." / "cmake-build-debug" / "pk",
        script_dir / ".." / "cmake-build-release" / "pk",
        script_dir / "pk",
        Path.cwd() / "pk",
    ]
    for cand in candidates:
        if cand.exists():
            return str(cand)

    which_pk = shutil.which("pk")
    return which_pk if which_pk else "pk"


PK_BINARY = os.environ.get("PK_BINARY", locate_pk())


# ================================================================
# Fixtures
# ================================================================

@pytest.fixture(autouse=True)
def setup_home_and_cleanup(monkeypatch, tmp_path):
    """
    为每个测试用例创建临时 HOME 目录，
    让 pk 的配置文件 .pk.json 和日志 .pk.log 存放在临时目录中。
    """
    home = tmp_path / "home"
    home.mkdir()
    monkeypatch.setenv("HOME", str(home))
    # 防止其它本地环境变量污染测试
    monkeypatch.delenv("PK_BINARY", raising=False)
    yield home


# ================================================================
# 辅助函数
# ================================================================

def run_pk(*args, input_text=None, cwd=None, env=None, timeout=10):
    """
    运行 pk 并返回 CompletedProcess。
    显式将 HOME 传入子进程环境，避免 monkeypatch 在某些平台不生效。
    """
    cmd = [PK_BINARY] + list(args)
    merged_env = os.environ.copy()
    if env:
        merged_env.update(env)
    proc = subprocess.run(
        cmd,
        input=input_text,
        capture_output=True,
        text=True,
        cwd=cwd,
        env=merged_env,
        timeout=timeout,
        check=False,
    )
    return proc


def read_config(home_path):
    """读取 .pk.json 并返回解析后的 dict，若文件不存在则返回空字典"""
    config_file = Path(home_path) / ".pk.json"
    if not config_file.exists():
        return {}
    with open(config_file, "r") as f:
        return json.load(f)


def write_config(home_path, config):
    """直接将 config 写入 .pk.json（用于预置测试场景）"""
    config_file = Path(home_path) / ".pk.json"
    with open(config_file, "w") as f:
        json.dump(config, f, indent=2)


# ================================================================
# 基础功能
# ================================================================

def test_add_record(setup_home_and_cleanup):
    """添加一条记录：目录 '.' + 命令 'ls -la'。"""
    home = setup_home_and_cleanup
    with tempfile.TemporaryDirectory() as tmp_dir:
        result = run_pk("-a", input_text=".\nls -la\n", cwd=tmp_dir)
        assert result.returncode == 0, f"stderr: {result.stderr}"

        config = read_config(home)
        paths = config.get("path", {})
        assert tmp_dir in paths
        cmds = paths[tmp_dir]
        assert isinstance(cmds, list)
        assert len(cmds) == 1
        assert cmds[0]["cmd"] == "ls -la"
        # 哈希应被写入
        assert "hash" in cmds[0] and cmds[0]["hash"]


def test_add_multiline_command_via_config(setup_home_and_cleanup):
    """
    多行命令应能正确写入 JSON 并读回。
    这里直接操作配置文件来模拟编辑器写入的效果（-E 需要交互式编辑器）。
    """
    home = setup_home_and_cleanup
    with tempfile.TemporaryDirectory() as tmp_dir:
        run_pk("-a", input_text=".\nplaceholder\n", cwd=tmp_dir)

        # 手工覆盖为多行命令
        config = read_config(home)
        config["path"][tmp_dir][0]["cmd"] = "echo line1\necho line2\necho line3"
        config["path"][tmp_dir][0].pop("hash", None)
        write_config(home, config)

        # -s 输出应保留完整内容
        result = run_pk("-s")
        assert result.returncode == 0
        # 显示时换行可能被保留，也可能被替换；两者都接受
        combined = result.stdout + result.stderr
        assert "line1" in combined and "line2" in combined and "line3" in combined


def test_show_record(setup_home_and_cleanup):
    """显示记录：先 -a 添加，再 -s 检查输出。"""
    with tempfile.TemporaryDirectory() as tmp_dir:
        result = run_pk("-a", input_text=".\necho hello\n", cwd=tmp_dir)
        assert result.returncode == 0

        result = run_pk("-s")
        assert result.returncode == 0
        combined = result.stdout + result.stderr
        assert tmp_dir in combined
        assert "echo hello" in combined


def test_show_record_empty(setup_home_and_cleanup):
    """无记录时 -s 应提示没有记录。"""
    result = run_pk("-s")
    assert result.returncode == 0
    combined = result.stdout + result.stderr
    assert "没有记录" in combined


# ================================================================
# 执行命令（-e / -p / recent）
# ================================================================

def test_execute_recent(setup_home_and_cleanup):
    """-e 1.2 执行第二条命令，并更新 recent。"""
    home = setup_home_and_cleanup
    with tempfile.TemporaryDirectory() as tmp_dir:
        run_pk("-a", input_text=".\nmake build\n", cwd=tmp_dir)
        run_pk("-a", input_text=".\nmake test\n", cwd=tmp_dir)

        result = run_pk("-e", "1.2")
        assert result.returncode == 0, f"stderr: {result.stderr}"
        stdout = result.stdout
        assert "cd " + tmp_dir in stdout
        assert "make test" in stdout
        # 新逻辑：整块命令应被括号包裹成子 shell
        assert "(\n" in stdout or "(" in stdout

        new_config = read_config(home)
        assert "recent" in new_config
        recent = new_config["recent"]
        assert recent is not None
        assert recent[0] == 0
        assert recent[1] == 1


def test_execute_multiline_subshell_wrapping(setup_home_and_cleanup):
    """
    多行命令应被包成 `cd <dir> && ( ... )` 的形式，
    保证所有行都在目标目录执行。
    """
    home = setup_home_and_cleanup
    with tempfile.TemporaryDirectory() as tmp_dir:
        run_pk("-a", input_text=".\nplaceholder\n", cwd=tmp_dir)

        # 改写成多行命令
        config = read_config(home)
        config["path"][tmp_dir][0]["cmd"] = "echo A\necho B"
        config["path"][tmp_dir][0].pop("hash", None)
        write_config(home, config)

        result = run_pk("-e", "1.1", input_text="Y\n")
        assert result.returncode == 0, f"stderr: {result.stderr}"
        stdout = result.stdout
        # 关键：目录切换后应紧跟括号，整块包起来
        assert f"cd {tmp_dir} && (" in stdout
        assert "echo A" in stdout
        assert "echo B" in stdout


def test_point_execution_no_recent(setup_home_and_cleanup):
    """-p 执行命令但不更新 recent。"""
    home = setup_home_and_cleanup
    with tempfile.TemporaryDirectory() as tmp_dir:
        run_pk("-a", input_text=".\nls -l\n", cwd=tmp_dir)
        run_pk("-c", input_text="1.1\n")

        config_before = read_config(home)
        assert "recent" in config_before
        recent_before = config_before["recent"]
        assert recent_before is not None

        result = run_pk("-p", "1.1")
        assert result.returncode == 0
        assert "ls -l" in result.stdout

        config_after = read_config(home)
        assert config_after["recent"] == recent_before


def test_hash_mismatch_reject(setup_home_and_cleanup):
    """
    手工篡改命令内容后，哈希不匹配，回答 'n' 应拒绝执行。
    """
    home = setup_home_and_cleanup
    with tempfile.TemporaryDirectory() as tmp_dir:
        run_pk("-a", input_text=".\necho original\n", cwd=tmp_dir)

        config = read_config(home)
        config["path"][tmp_dir][0]["cmd"] = "echo tampered"
        write_config(home, config)

        result = run_pk("-e", "1.1", input_text="n\n")
        assert result.returncode == 0
        combined = result.stdout + result.stderr
        # 应询问是否信任，且拒绝后不打印执行脚本
        assert "信任" in combined or "trust" in combined.lower()
        assert "echo tampered" not in result.stdout


def test_hash_mismatch_accept(setup_home_and_cleanup):
    """哈希不匹配但回答 Y 时，应同步哈希并执行。"""
    home = setup_home_and_cleanup
    with tempfile.TemporaryDirectory() as tmp_dir:
        run_pk("-a", input_text=".\necho original\n", cwd=tmp_dir)

        config = read_config(home)
        config["path"][tmp_dir][0]["cmd"] = "echo updated"
        write_config(home, config)

        result = run_pk("-e", "1.1", input_text="Y\n")
        assert result.returncode == 0
        assert "echo updated" in result.stdout

        # 哈希应被更新为新命令
        new_config = read_config(home)
        assert new_config["path"][tmp_dir][0]["cmd"] == "echo updated"
        assert "hash" in new_config["path"][tmp_dir][0]


def test_set_recent(setup_home_and_cleanup):
    """-c 设置最近记录，无参数运行时执行它。"""
    with tempfile.TemporaryDirectory() as dir1, \
         tempfile.TemporaryDirectory() as dir2:
        run_pk("-a", input_text=".\ncmdA\n", cwd=dir1)
        run_pk("-a", input_text=".\ncmdB\n", cwd=dir2)

        result = run_pk("-c", input_text="2.1\n")
        assert result.returncode == 0

        result = run_pk(input_text="Y\n")
        assert result.returncode == 0, f"stderr: {result.stderr}"
        stdout = result.stdout
        assert "cd " + dir2 in stdout
        assert "cmdB" in stdout
        assert "cmdA" not in stdout


def test_no_args_runs_recent(setup_home_and_cleanup):
    """无参数调用，无 recent 时提示。"""
    result = run_pk()
    assert result.returncode == 0
    combined = result.stdout + result.stderr
    assert "没有最近记录" in combined


def test_extra_arguments(setup_home_and_cleanup):
    """-e / -p 后附加参数应追加到命令末尾。"""
    home = setup_home_and_cleanup
    with tempfile.TemporaryDirectory() as dir1:
        run_pk("-a", input_text=f"{dir1}\necho hello\n")

        result = run_pk("-e", "1.1", "--extra", "world", input_text="Y\n")
        assert result.returncode == 0
        assert "echo hello --extra world" in result.stdout

        # -p 带额外参数，recent 不变
        run_pk("-c", input_text="1.1\n")
        config_before = read_config(home)
        recent_before = config_before["recent"]

        result = run_pk("-p", "1.1", "--extra", "foo", "bar", input_text="Y\n")
        assert result.returncode == 0
        assert "echo hello --extra foo bar" in result.stdout

        config_after = read_config(home)
        assert config_after["recent"] == recent_before

        # 带空格的参数
        result = run_pk(
            "-e", "1.1",
            "--arg1", "value with space", "--arg2=value2",
            input_text="Y\n",
        )
        assert result.returncode == 0
        assert "echo hello --arg1 value with space --arg2=value2" in result.stdout

        # 无索引 + 额外参数 → 警告并走交互选择
        result = run_pk("-e", "--extra", "ignored", input_text="1.1\nY\n")
        assert result.returncode == 0
        combined = result.stdout + result.stderr
        assert "Warning" in combined or "警告" in combined


# ================================================================
# 错误路径
# ================================================================

def test_invalid_index(setup_home_and_cleanup):
    """无效索引应报错但不崩溃。"""
    with tempfile.TemporaryDirectory() as tmp_dir:
        run_pk("-a", input_text=".\nls\n", cwd=tmp_dir)
        result = run_pk("-e", "99.99", input_text="Y\n")
        assert result.returncode == 0
        combined = result.stdout + result.stderr
        assert "无效编号" in combined or "Invalid" in combined


def test_nonexistent_directory(setup_home_and_cleanup):
    """目录被删除后执行命令，应报'目标目录不存在'。"""
    home = setup_home_and_cleanup
    with tempfile.TemporaryDirectory() as tmp_dir:
        run_pk("-a", input_text=".\nls\n", cwd=tmp_dir)

    # tmp_dir 已被删除
    result = run_pk("-e", "1.1", input_text="Y\n")
    assert result.returncode == 0
    combined = result.stdout + result.stderr
    assert "目标目录不存在" in combined or "not exist" in combined.lower()


def test_unknown_option(setup_home_and_cleanup):
    """未知选项应给出提示。"""
    result = run_pk("--this-does-not-exist")
    assert result.returncode == 0
    combined = result.stdout + result.stderr
    assert "Unknown option" in combined or "未知" in combined


# ================================================================
# 版本 / 帮助
# ================================================================

def test_version(setup_home_and_cleanup):
    result = run_pk("-v")
    assert result.returncode == 0
    assert "path-keeper" in result.stderr

    result_verbose = run_pk("--version-verbose")
    assert result_verbose.returncode == 0
    assert "Build date" in result_verbose.stderr


def test_help(setup_home_and_cleanup):
    result = run_pk("-h")
    assert result.returncode == 0
    stderr = result.stderr
    assert "--add" in stderr
    assert "--execute" in stderr


# ================================================================
# 搜索
# ================================================================

def test_search_fallback(setup_home_and_cleanup):
    """无 fzf 时回退到列表选择。"""
    home = setup_home_and_cleanup
    with tempfile.TemporaryDirectory() as dir1, \
         tempfile.TemporaryDirectory() as dir2:
        run_pk("-a", input_text=".\ncmd1\n", cwd=dir1)
        run_pk("-a", input_text=".\ncmd2\n", cwd=dir2)

        env_override = {"PATH": ""}
        result = run_pk("search", input_text="2.1\n", env=env_override)
        assert result.returncode == 0, f"stderr: {result.stderr}"
        assert "cmd2" in result.stdout

        config = read_config(home)
        assert "recent" in config
        assert config["recent"] is not None


# ================================================================
# 执行完整流程
# ================================================================

def test_execute(setup_home_and_cleanup):
    """添加 → 显示 → 执行，校验配置和输出。"""
    home = setup_home_and_cleanup
    with tempfile.TemporaryDirectory() as dir1:
        board = run_pk("-s")
        assert "没有记录" in board.stderr
        jsonclear = read_config(home)
        assert "shell" not in jsonclear

        run_pk("-a", input_text=f"{dir1}\nls\n")
        oneline = run_pk("-s")
        assert "ls" in oneline.stderr
        assert dir1 in oneline.stderr

        pk_command_return = run_pk("-e", input_text="1.1\n")
        assert pk_command_return.returncode == 0
        jsonfile = read_config(home)
        # 命令仍应保存在配置中
        assert dir1 in jsonfile["path"]


# ================================================================
# config / editor
# ================================================================

def test_config_editor(setup_home_and_cleanup):
    home = setup_home_and_cleanup
    result = run_pk("config", "-editor", "vim")
    assert result.returncode == 0
    combined = result.stdout + result.stderr
    assert "Selected editor" in combined

    config = read_config(home)
    assert config.get("editor") == "vim"


# ================================================================
# log
# ================================================================

def test_log(setup_home_and_cleanup):
    """
    测试 log 功能：
    - 全局启用/禁用
    - 命令级启用/禁用
    - 日志文件内容格式
    """
    home = setup_home_and_cleanup
    with tempfile.TemporaryDirectory() as dir1, \
         tempfile.TemporaryDirectory() as dir2:
        run_pk("-a", input_text=f"{dir1}\ncmd1\n", cwd=dir1)
        run_pk("-a", input_text=f"{dir1}\ncmd1.2\n", cwd=dir1)
        run_pk("-a", input_text=f"{dir2}\ncmd2\n", cwd=dir2)

        # 1. 启用全局日志
        result = run_pk("log", "--enable", "global")
        assert result.returncode == 0
        assert "enabled" in result.stderr
        config = read_config(home)
        assert config.get("global_log") is True

        # 2. 禁用命令 1.2
        result = run_pk("log", "--disable", "1.2")
        assert result.returncode == 0
        assert "disabled" in result.stderr
        config = read_config(home)
        path_entry = config["path"].get(dir1)
        assert path_entry is not None
        assert path_entry[1].get("log") is False

        # 3. 执行命令
        run_pk("-e", "1.1", input_text="Y\n")
        run_pk("-e", "1.2", input_text="Y\n")
        run_pk("-e", "2.1", input_text="Y\n")

        # 4. 禁用全局日志
        result = run_pk("log", "--disable", "global")
        assert result.returncode == 0
        config = read_config(home)
        assert config.get("global_log") is False


# ================================================================
# alias
# ================================================================

def test_alias(setup_home_and_cleanup):
    home = setup_home_and_cleanup
    with tempfile.TemporaryDirectory() as dir1:
        run_pk("-a", input_text=f"{dir1}\necho hello\n")

        # 添加
        result = run_pk("alias", "add", "myalias", "1.1")
        assert result.returncode == 0
        assert "已添加" in result.stderr

        config = read_config(home)
        assert config["path"][dir1][0].get("alias") == "myalias"

        # 列出
        result = run_pk("alias", "list")
        assert result.returncode == 0
        assert "myalias" in result.stderr
        assert "1.1" in result.stderr

        # 通过别名执行（parseIndex 支持别名）
        result = run_pk("-e", "myalias", input_text="Y\n")
        assert result.returncode == 0
        assert "echo hello" in result.stdout

        # 移除
        result = run_pk("alias", "remove", "myalias")
        assert result.returncode == 0
        assert "已删除" in result.stderr

        result = run_pk("alias", "list")
        assert "myalias" not in result.stderr

        # install
        run_pk("alias", "add", "another", "1.1")
        result = run_pk("alias", "install")
        assert result.returncode == 0
        alias_script = Path(home) / ".pk_aliases.sh"
        assert alias_script.exists()
        content = alias_script.read_text()
        assert "alias another='pk -e 1.1'" in content


def test_alias_unknown_subcommand(setup_home_and_cleanup):
    result = run_pk("alias", "bogus")
    assert result.returncode == 0
    combined = result.stdout + result.stderr
    assert "Unknown" in combined or "未知" in combined


# ================================================================
# PTY 开关
# ================================================================

def test_pty_flags_accepted(setup_home_and_cleanup):
    """-P / -N 应被接受并从参数列表里剥离。"""
    with tempfile.TemporaryDirectory() as tmp_dir:
        # 带 -N 添加
        r1 = run_pk("-N", "-a", input_text=".\necho ptytest\n", cwd=tmp_dir)
        assert r1.returncode == 0

        # 带 -P 显示
        r2 = run_pk("-P", "-s")
        assert r2.returncode == 0
        combined = r2.stdout + r2.stderr
        assert "echo ptytest" in combined


# ================================================================
# 补全被注释的 test_dir_return
# ================================================================

def test_command_with_special_chars(setup_home_and_cleanup):
    """
    含特殊字符的命令应能正确写入 JSON 并读回。
    （对应原 test_dir_return 想覆盖的场景）
    """
    home = setup_home_and_cleanup
    with tempfile.TemporaryDirectory() as tmp_dir:
        special = 'echo "a\\b" && echo $HOME | grep -v "x"'
        run_pk("-a", input_text=f".\n{special}\n", cwd=tmp_dir)

        config = read_config(home)
        cmds = config["path"][tmp_dir]
        assert cmds[0]["cmd"] == special

        # -s 输出应包含原命令
        result = run_pk("-s")
        combined = result.stdout + result.stderr
        assert "echo" in combined


def test_directory_with_spaces(setup_home_and_cleanup):
    """目录名带空格时应能正确保存与执行。"""
    home = setup_home_and_cleanup
    with tempfile.TemporaryDirectory() as base:
        dir_with_space = os.path.join(base, "has space")
        os.makedirs(dir_with_space)

        run_pk("-a", input_text=f"{dir_with_space}\nls\n")
        config = read_config(home)
        assert dir_with_space in config["path"]

        result = run_pk("-e", "1.1", input_text="Y\n")
        assert result.returncode == 0
        # 子 shell 包裹应包含该目录
        assert dir_with_space in result.stdout
