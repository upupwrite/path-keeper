import json
import os
import shutil
import subprocess
import tempfile
from pathlib import Path

import pytest


def locate_pk():
    """自动寻找 pk 可执行文件"""
    # 1. 优先使用环境变量
    env_pk = os.environ.get("PK_BINARY")
    if env_pk and os.path.exists(env_pk):
        return env_pk

    # 2. 尝试在当前文件所在路径的父目录下寻找（适用于源码与构建目录并列的场景）
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

    # 3. 降级为 shutil.which("pk") 或默认 "pk"
    which_pk = shutil.which("pk")
    return which_pk if which_pk else "pk"


PK_BINARY = os.environ.get("PK_BINARY", locate_pk())


@pytest.fixture(autouse=True)
def setup_home_and_cleanup(monkeypatch, tmp_path):
    """
    为每个测试用例创建临时 HOME 目录，
    让 pk 的配置文件 .pk.json 和日志 .pk.log 存放在临时目录中。
    测试结束后自动清理。
    """
    home = tmp_path / "home"
    home.mkdir()
    monkeypatch.setenv("HOME", str(home))
    yield home


def run_pk(*args, input_text=None, cwd=None, env=None, timeout=10):
    """
    辅助函数：运行 pk 并返回 CompletedProcess。
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
        check=False,  # 显式指定，避免 Pyright 警告
    )
    return proc


def read_config(home_path):
    """读取 .pk.json 并返回解析后的 dict，若文件不存在则返回空字典"""
    config_file = home_path / ".pk.json"
    if not config_file.exists():
        return {}  # 不再返回 None
    with open(config_file, "r") as f:
        return json.load(f)


# ================================================================
# 测试用例
# ================================================================


def test_add_record(setup_home_and_cleanup):
    """
    测试添加一条记录：
    输入 '.' 作为目录（应替换为当前工作目录），
    输入 'ls -la' 作为命令，验证配置文件是否正确写入。
    """
    home = setup_home_and_cleanup
    with tempfile.TemporaryDirectory() as tmp_dir:
        input_str = ".\nls -la\n"
        result = run_pk("-a", input_text=input_str, cwd=tmp_dir)
        assert result.returncode == 0, f"stderr: {result.stderr}"

        config = read_config(home)
        paths = config.get("path", {})
        assert tmp_dir in paths
        cmds = paths[tmp_dir]
        assert isinstance(cmds, list)
        print(cmds)
        assert len(cmds) == 1
        assert cmds[0]["cmd"] == "ls -la"


def test_show_record(setup_home_and_cleanup):
    """
    测试显示记录：
    使用 pk -a 先添加一个目录和命令，再通过 pk -s 检查输出。
    """
    with tempfile.TemporaryDirectory() as tmp_dir:
        # 添加记录
        input_str = ".\necho hello\n"
        result = run_pk("-a", input_text=input_str, cwd=tmp_dir)
        assert result.returncode == 0

        # 显示记录
        result = run_pk("-s")
        assert result.returncode == 0
        stderr = result.stderr
        assert tmp_dir in stderr
        assert "echo hello" in stderr


def test_execute_recent(setup_home_and_cleanup):
    """
    测试执行最近记录：
    添加一个目录并包含两条命令，然后执行 -e 1.2，
    提供 Y 确认哈希验证，验证输出中包含命令脚本，并检查 recent 字段。
    """
    home = setup_home_and_cleanup
    with tempfile.TemporaryDirectory() as tmp_dir:
        # 添加两条命令到同一个目录
        run_pk("-a", input_text=".\nmake build\n", cwd=tmp_dir)
        run_pk("-a", input_text=".\nmake test\n", cwd=tmp_dir)

        result = run_pk("-e", "1.2")
        print(run_pk("-s").stderr)
        print(result.stderr)
        assert result.returncode == 0, f"stderr: {result.stderr}"
        stdout = result.stdout
        # 输出应包含 cd <tmp_dir> && make test
        assert "cd " + tmp_dir in stdout
        assert "make test" in stdout

        # 验证 recent 记录被更新
        new_config = read_config(home)
        assert "recent" in new_config  # 确保键存在
        recent = new_config["recent"]
        assert recent is not None
        # 只添加了一个目录，所以目录索引为0；第二个命令索引为1
        assert recent[0] == 0
        assert recent[1] == 1


def test_point_execution_no_recent(setup_home_and_cleanup):
    """
    测试点执行（-p）：执行命令但不更新 recent。
    需要提供 Y 确认哈希验证。
    """
    home = setup_home_and_cleanup
    with tempfile.TemporaryDirectory() as tmp_dir:
        # 添加一条命令
        run_pk("-a", input_text=".\nls -l\n", cwd=tmp_dir)

        # 先通过 -c 设置一个 recent 记录作为基准
        run_pk("-c", input_text="1.1\n")

        config_before = read_config(home)
        assert "recent" in config_before
        recent_before = config_before["recent"]
        assert recent_before is not None

        # 执行 -p 1.1，提供信任输入
        result = run_pk("-p", "1.1", input_text="Y\n")
        assert result.returncode == 0
        # 输出应包含 ls -l
        assert "ls -l" in result.stdout

        # recent 不应被修改
        config_after = read_config(home)
        assert "recent" in config_after
        assert config_after["recent"] == recent_before


def test_set_recent(setup_home_and_cleanup):
    """
    测试设置最近记录（-c）：
    添加两个不同目录的命令，通过 -c 选择第二个目录，
    然后无参数运行 pk 验证执行的是第二个目录的命令。
    """
    with tempfile.TemporaryDirectory() as dir1, tempfile.TemporaryDirectory() as dir2:
        # 添加两个不同目录的命令
        run_pk("-a", input_text=".\ncmdA\n", cwd=dir1)
        run_pk("-a", input_text=".\ncmdB\n", cwd=dir2)

        # 设置 recent 为 2.1 （即 dir2 的 cmdB）
        result = run_pk("-c", input_text="2.1\n")
        assert result.returncode == 0

        # 无参数运行，应执行 dir2 的 cmdB，需确认哈希
        result = run_pk(input_text="Y\n")
        assert result.returncode == 0, f"stderr: {result.stderr}"
        stdout = result.stdout
        # 输出应包含 dir2 和 cmdB
        assert "cd " + dir2 in stdout
        assert "cmdB" in stdout
        # 不应包含 dir1 的 cmdA
        assert "cmdA" not in stdout


def test_version(setup_home_and_cleanup):
    """测试版本输出"""
    result = run_pk("-v")
    assert result.returncode == 0
    assert "path-keeper" in result.stderr

    result_verbose = run_pk("--version-verbose")
    assert result_verbose.returncode == 0
    assert "Build date" in result_verbose.stderr


def test_help(setup_home_and_cleanup):
    """测试帮助输出"""
    result = run_pk("-h")
    assert result.returncode == 0
    stderr = result.stderr
    assert "--add" in stderr
    assert "--execute" in stderr


def test_no_args_runs_recent(setup_home_and_cleanup):
    """
    无参数调用时，应尝试执行最近记录。
    若没有最近记录，应输出提示信息。
    """
    result = run_pk()
    assert result.returncode == 0
    assert "没有最近记录" in result.stderr


def test_search_fallback(setup_home_and_cleanup):
    """
    测试搜索功能（在没有 fzf 的环境下会退化为列表选择）。
    提供选择和信任输入，验证输出命令并更新 recent。
    """
    home = setup_home_and_cleanup
    with tempfile.TemporaryDirectory() as dir1, tempfile.TemporaryDirectory() as dir2:
        run_pk("-a", input_text=".\ncmd1\n", cwd=dir1)
        run_pk("-a", input_text=".\ncmd2\n", cwd=dir2)

        # 设置 PATH 为空，使 fzf 不可用；pk 本身使用绝对路径执行
        env_override = {"PATH": ""}
        result = run_pk("search", input_text="2.1\n", env=env_override)
        assert result.returncode == 0, f"stderr: {result.stderr}"
        stdout = result.stdout
        # 输出应包含 cmd2 的执行脚本
        assert "cmd2" in stdout

        # 验证 recent 被更新（至少不是 None）
        config = read_config(home)
        assert "recent" in config
        assert config["recent"] is not None


def test_execute(setup_home_and_cleanup):
    """
    测试执行:
    添加一个目录包含一个命令,检验std::out是否符合预期
    """
    home = setup_home_and_cleanup
    with tempfile.TemporaryDirectory() as dir1:
        board = run_pk("-s")
        assert "没有记录" in board.stderr
        jsonclear = read_config(home)
        print(jsonclear)
        assert "shell" not in jsonclear
        run_pk("-a", input_text=f"{dir1}\nls\n")
        oneline = run_pk("-s")
        assert "ls" in oneline.stderr
        assert dir1 in oneline.stderr

        pk_command_return = run_pk("-e", input_text="1.1\n")
        print(pk_command_return.stderr)
        print(pk_command_return.stdout)
        jsonfile = read_config(home)
        print(jsonfile)


# TODO: 完成log功能
# def test_log(setup_home_and_cleanup):
#     """
#     测试log是否符合格式:
#     添加两个命令并执行需要log,验证pk_log文件
#     """
#     with tempfile.TemporaryDirectory() as dir1, tempfile.TemporaryDirectory() as dir2:
#         run_pk("-a", input_text=".\ncmd1\n", cwd=dir1)
#         run_pk("-a", input_text=".\ncmd2\n", cwd=dir2)
#         log_enable_result = subprocess.run(
#             ["pk", "log", "--enable", "1.1"],
#             capture_output=True,
#             text=True,
#             check=False,
#         )
#         assert "配置完成" in log_enable_result.stderr
#         log_enable_global = subprocess.run(
#             ["pk", "log", "--enable", "global"],
#             capture_output=True,
#             text=True,
#             check=False,
#         )
#         assert "Enabled" in log_enable_global.stderr
#         file_global_setting = read_config(setup_home_and_cleanup)
#         assert "True" in file_global_setting
#         assert "global" in file_global_setting
#         subprocess.run(["pk", "log", "--disable"], input="1.2\n", check=False)
#         file_disable_setting = read_config(setup_home_and_cleanup)
#         assert "false" in file_disable_setting["path"][dir2][1]["log"]


def test_config(setup_home_and_cleanup):
    config_result = subprocess.run(
        [PK_BINARY, "config", "-editor", "vim"],
        capture_output=True,
        text=True,
        check=False,
    )
    print(config_result.stderr)
    assert "Selected editor" in config_result.stderr
    result = run_pk("config")
    print(result.stdout)
    assert "vim" in result.stdout
