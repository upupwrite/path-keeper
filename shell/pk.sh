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
# Default behavior (use_pty = false in the binary):
#   The binary prints a shell command on stdout, which this wrapper
#   eval's in the parent shell. This preserves the legacy tmux / logging
#   / cd semantics.
#
# Opt-in PTY behavior:
#   With --pty / -P the binary runs the command itself inside an
#   in-process PTY and streams output directly -- eval is NOT used,
#   because stdout already contains the real command output (or, for
#   bare `pk log` / `pk config`, the wrapper still eval's the printed
#   command so less / the editor runs on the user's terminal).
#
pk() {
    # Decide whether stdout should be eval'd.
    #
    #   need_eval = 1 (default)  -> legacy: capture stdout, eval it
    #   need_eval = 0            -> PTY: pass output straight through
    local need_eval=1

    # Scan for PTY toggle flags. If both appear, the last one wins,
    # mirroring how the binary resolves them.
    local a
    for a in "$@"; do
        case "$a" in
            --pty|-P)    need_eval=0 ;;
            --no-pty|-N) need_eval=1 ;;
        esac
    done

    # Bare `log` / `config` (no further arguments) always rely on the
    # external shell (less / the configured editor), so they need eval
    # even when the user passed --pty.
    if [ "$need_eval" -eq 0 ]; then
        case "${1:-}" in
            log|config)
                [ "$#" -eq 1 ] && need_eval=1
                ;;
        esac
    fi

    if [ "$need_eval" -eq 1 ]; then
        # Legacy path: capture stdout, then eval it in the parent shell.
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
    fi

    # PTY path: the binary runs the command in an in-process PTY and
    # streams output directly. Just forward everything.
    _pk_binary "$@"
    return $?
}
