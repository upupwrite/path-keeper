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
    case "$1" in
        -e)
            local cmd_output
            cmd_output=$(_pk_binary "$@")
            if [ $? -eq 0 ] && [ -n "$cmd_output" ]; then
                eval "$cmd_output"
            else
                echo "$cmd_output"
            fi
            ;;
        *)
            _pk_binary "$@"
            ;;
    esac
}
