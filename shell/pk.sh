# pk.sh - Shell integration for path-keeper
# Place this file in /usr/local/share/path-keeper/pk.sh
# Source it in your shell: source /usr/local/share/path-keeper/pk.sh

_pk_binary() {
    if command -v pk >/dev/null 2>&1; then
        command pk "$@"
    else
        echo "pk: binary not found" >&2
        return 127
    fi
}

pk() {
    local cmd_output
    local exit_code
    local exit_file

    exit_file=$(mktemp) || return 1

    # 捕获二进制输出（仅 stdout），stderr 直通终端
    cmd_output=$(_pk_binary "$@" 2>&2; echo $? > "$exit_file") && true
    exit_code=$(cat "$exit_file")
    rm -f "$exit_file"      # 显式清理，不再依赖 trap

    if [ "$exit_code" -ne 0 ]; then
        return $exit_code
    fi

    if [ -n "$cmd_output" ]; then
        if _is_interactive_cmd "$cmd_output"; then
            local core_cmd
            core_cmd=$(_extract_core_cmd "$cmd_output")
            echo "Executing interactive command: $core_cmd"
            eval "$core_cmd"
        else
            eval "$cmd_output"
        fi
    fi
}

# 检测命令是否包含全屏交互式程序（改进的正则）
_is_interactive_cmd() {
    local cmd_str="$1"
    local interactive_progs="nvim|vim|vi|nano|emacs|less|more|top|htop"
    # 匹配命令名前后为行首、管道、分号、逻辑操作符或空白
    echo "$cmd_str" | grep -qE "(^|[;&|()[:space:]])($interactive_progs)([[:space:]]|$)"
}

# 从复合命令中提取核心命令（更鲁棒的实现）
_extract_core_cmd() {
    local cmd_str="$1"
    # 去除首尾空白
    cmd_str=$(echo "$cmd_str" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')
    # 如果以 '(' 开头，尝试剥离外层括号和管道部分
    if echo "$cmd_str" | grep -q "^("; then
        local inner
        inner=$(echo "$cmd_str" | sed -n 's/^(\(.*\))\s*$/\1/p')
        if [ -n "$inner" ]; then
            # 移除尾部可能存在的 `2>&1 | tee ...` 等
            inner=$(echo "$inner" | sed 's/[[:space:]]*2>&1[[:space:]]*|.*$//')
            inner=$(echo "$inner" | sed 's/[[:space:]]*|[[:space:]]*tee.*$//')
            echo "$inner"
            return
        fi
    fi
    echo "$cmd_str"
}