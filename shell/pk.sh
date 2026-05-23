# pk.sh - Shell integration for path-keeper
# Place this file in /usr/local/share/path-keeper/pk.sh
# Source it in your shell like this: source /usr/local/share/path-keeper/pk.sh

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

    # 正确捕获 stdout 并保留退出码
    # 使用临时文件存储退出码，避免复杂的文件描述符操作
    local exit_file
    exit_file=$(mktemp) || return 1
    trap 'rm -f "$exit_file"' RETURN

    # 将 _pk_binary 的 stdout 保存至变量，stderr 直通终端
    # 退出码写入临时文件
    cmd_output=$(_pk_binary "$@" 2>&2; echo $? > "$exit_file") && true
    # 读取真实的退出码
    exit_code=$(cat "$exit_file")

    if [ $exit_code -ne 0 ]; then
        return $exit_code
    fi

    if [ -n "$cmd_output" ]; then
        # 如果命令是交互式程序（如编辑器），直接执行并接管终端
        if _is_interactive_cmd "$cmd_output"; then
            # 剥离可能的外层包裹和管道，只执行核心命令
            local core_cmd
            core_cmd=$(_extract_core_cmd "$cmd_output")
            echo "Executing interactive command: $core_cmd"
            eval "$core_cmd"
        else
            eval "$cmd_output"
        fi
    fi
}

# 检测命令是否包含全屏交互式程序（可自行扩充）
_is_interactive_cmd() {
    local cmd_str="$1"
    # 常见的全屏编辑器、翻页器
    local interactive_progs="nvim|vim|vi|nano|emacs|less|more|top|htop"
    # 如果命令中包含这些程序名，并且存在管道或重定向，视为需要直接运行
    if echo "$cmd_str" | grep -qE "(^|;|&&|\|\||\()\s*($interactive_progs)\s"; then
        return 0
    fi
    # 也可检测是否有管道符号，再进一步判断
    if echo "$cmd_str" | grep -q "|" && echo "$cmd_str" | grep -qE "\b($interactive_progs)\b"; then
        return 0
    fi
    return 1
}

# 从可能复杂的命令字符串中提取最可能的核心命令（简单启发式）
_extract_core_cmd() {
    local cmd_str="$1"
    # 去掉首尾空白
    cmd_str=$(echo "$cmd_str" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')
    # 如果以 '(' 开头且带有管道，尝试提取括号内的内容并去掉最后的管道部分
    if echo "$cmd_str" | grep -q "^("; then
        # 去掉最外层的括号
        local inner
        inner=$(echo "$cmd_str" | sed -n 's/^(\(.*\))\s*$/\1/p')
        if [ -n "$inner" ]; then
            # 移除管道 `| tee ...` 或 `2>&1 | tee ...` 等
            inner=$(echo "$inner" | sed 's/[[:space:]]*2>&1[[:space:]]*|.*$//')
            inner=$(echo "$inner" | sed 's/[[:space:]]*|[[:space:]]*tee.*$//')
            echo "$inner"
            return
        fi
    fi
    # 否则直接返回原字符串（作为退化处理）
    echo "$cmd_str"
}
