#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Path Keeper Contributors
# This file is part of Path Keeper.
# Path Keeper is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# Path Keeper is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
# You should have received a copy of the GNU General Public License
# along with Path Keeper. If not, see <https://www.gnu.org/licenses/>.

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

# pk() is a shell wrapper around the `pk` binary.
#
# Two execution modes exist in the binary:
#
#   Legacy mode (default, use_pty = false):
#     For commands that emit a shell script on stdout (`pk -e`, `pk -p`,
#     bare `pk`, `pk -c`), this wrapper captures stdout and eval's it in
#     the parent shell. This preserves the `cd <dir>`, tmux pipe-pane and
#     logging semantics -- the script *must* run in the parent shell, not
#     in a subshell, for `cd` and `tmux pipe-pane` to have any effect.
#
#   PTY mode (--pty / -P):
#     The binary executes the command itself inside an in-process PTY and
#     streams output directly. stdout is the real command output, so eval
#     must NOT be used here.
#
# Pure-interactive commands (`-a`, `--add`, `add`, `search`) never emit a
# script; they only prompt the user and write status to stderr. They are
# forwarded directly so no extra subshell is spawned. Note that `-e`, `-p`
# and `-c` are interactive too, but their stdout DOES carry the shell
# script, so they must go through the eval path.
#
pk() {
    # ------------------------------------------------------------------
    # 1. Detect PTY mode. Last flag wins, mirroring the binary's logic.
    # ------------------------------------------------------------------
    local use_pty=0
    local a
    for a in "$@"; do
        case "$a" in
            --pty|-P)    use_pty=1 ;;
            --no-pty|-N) use_pty=0 ;;
        esac
    done

    # PTY mode: the binary streams real output. Just forward.
    if [ "$use_pty" -eq 1 ]; then
        _pk_binary "$@"
        return $?
    fi

    # ------------------------------------------------------------------
    # 2. Pure-interactive commands: forward directly, no eval.
    #
    #    Only these qualify -- they never print a shell script to stdout.
    # ------------------------------------------------------------------
    case "${1:-}" in
        -a|--add|add|search)
            _pk_binary "$@"
            return $?
            ;;
    esac

    # ------------------------------------------------------------------
    # 3. Legacy path: capture stdout, then eval it in the parent shell.
    #
    #    Reached for `pk -e`, `pk -p`, `pk -c`, bare `pk`, `pk log`,
    #    `pk config`, `pk -s`, `pk alias ...`, etc.
    # ------------------------------------------------------------------
    local cmd_output ret
    cmd_output=$(_pk_binary "$@")
    ret=$?
    if [ "$ret" -eq 0 ] && [ -n "$cmd_output" ]; then
        eval "$cmd_output"
        ret=$?
    elif [ -n "$cmd_output" ]; then
        printf '%s\n' "$cmd_output"
    fi
    return "$ret"
}
