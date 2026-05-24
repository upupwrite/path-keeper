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

    local exit_file
    exit_file=$(mktemp) || return 1
    trap 'rm -f "$exit_file"' RETURN

    cmd_output=$(_pk_binary "$@" 2>&2; echo $? > "$exit_file") && true
    exit_code=$(cat "$exit_file")

    if [ $exit_code -ne 0 ]; then
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

_is_interactive_cmd() {
    local cmd_str="$1"
    local interactive_progs="nvim|vim|vi|nano|emacs|less|more|top|htop"
    # 更宽松的匹配：命令前可能是行首、;、&&、||、管道、空格等
    echo "$cmd_str" | grep -qE "(^|[;&|()[:space:]])($interactive_progs)([[:space:]]|$)"
}

_extract_core_cmd() {
    local cmd_str="$1"
    cmd_str=$(echo "$cmd_str" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')
    if echo "$cmd_str" | grep -q "^("; then
        local inner
        inner=$(echo "$cmd_str" | sed -n 's/^(\(.*\))\s*$/\1/p')
        if [ -n "$inner" ]; then
            inner=$(echo "$inner" | sed 's/[[:space:]]*2>&1[[:space:]]*|.*$//')
            inner=$(echo "$inner" | sed 's/[[:space:]]*|[[:space:]]*tee.*$//')
            echo "$inner"
            return
        fi
    fi
    echo "$cmd_str"
}