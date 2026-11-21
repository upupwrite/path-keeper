# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Path Keeper Contributors
# Bash completion for path-keeper (pk).

_pk_completion()
{
    local cur prev words cword
    if declare -F _init_completion >/dev/null 2>&1; then
        _init_completion || return
    else
        COMPREPLY=()
        cur="${COMP_WORDS[COMP_CWORD]}"
        prev="${COMP_WORDS[COMP_CWORD-1]}"
        words=("${COMP_WORDS[@]}")
        cword=$COMP_CWORD
    fi

    # Use the real binary, never the pk() shell wrapper (which eval's stdout).
    local pk_cmd=(command pk)

    case "$prev" in
        -editor)
            COMPREPLY=( $(compgen -W "$("${pk_cmd[@]}" completion editors "$cur")" -- "$cur") )
            return
            ;;
        completion|complete)
            COMPREPLY=( $(compgen -W "commands indices aliases directories editors" -- "$cur") )
            return
            ;;
        -e|--execute|-p|--point|-c|--configure|-r|--run|run)
            COMPREPLY=( $(compgen -W "$("${pk_cmd[@]}" completion indices "$cur")" -- "$cur") )
            return
            ;;
        --enable|--disable)
            COMPREPLY=( $(compgen -W "global $("${pk_cmd[@]}" completion indices "$cur")" -- "$cur") )
            return
            ;;
    esac

    if [[ $cword -eq 1 ]]; then
        COMPREPLY=( $(compgen -W "$("${pk_cmd[@]}" completion commands "$cur")" -- "$cur") )
        return
    fi

    case "${words[1]}" in
        alias)
            if [[ $cword -eq 2 ]]; then
                COMPREPLY=( $(compgen -W "add remove list install" -- "$cur") )
            elif [[ ${words[2]} == remove ]]; then
                COMPREPLY=( $(compgen -W "$("${pk_cmd[@]}" completion aliases "$cur")" -- "$cur") )
            elif [[ ${words[2]} == add && $cword -ge 4 ]]; then
                COMPREPLY=( $(compgen -W "$("${pk_cmd[@]}" completion indices "$cur")" -- "$cur") )
            fi
            return
            ;;
        completion|complete)
            COMPREPLY=( $(compgen -W "commands indices aliases directories editors" -- "$cur") )
            return
            ;;
        log)
            COMPREPLY=( $(compgen -W "--enable --disable global $("${pk_cmd[@]}" completion indices "$cur")" -- "$cur") )
            return
            ;;
    esac
}

complete -F _pk_completion pk
