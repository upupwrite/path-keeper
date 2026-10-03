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
#   because stdout already contains the real command output.
#
# Commands that must NOT capture stdout:
#   * `-a` / `add` / `search`  -> readline prompt only
#   * `-r` / `--run`           -> launches the Rust editor (interactive
#                                 UI) and then executes the buffer
#                                 directly. Capturing stdout would
#                                 swallow both the readline prompt and
#                                 the editor UI, and eval would then
#                                 try to re-run the editor's output as
#                                 shell commands.
#
pk() {
    # ------------------------------------------------------------------
    # 1. Detect PTY mode. Last flag wins, mirroring the binary's logic.
    # ------------------------------------------------------------------
    local need_eval=1
    local a
    for a in "$@"; do
        case "$a" in
            --pty|-P)    need_eval=0 ;;
            --no-pty|-N) need_eval=1 ;;
        esac
    done

    # ------------------------------------------------------------------
    # 2. Interactive commands that must never be captured.
    #
    #    They need a real terminal on stdout for readline / fzf /
    #    the Rust editor. They also never emit a shell script that
    #    would need eval.
    # ------------------------------------------------------------------
    case "${1:-}" in
        -a|--add|add|search|-r|--run|run)
            _pk_binary "$@"
            return $?
            ;;
    esac

    # ------------------------------------------------------------------
    # 3. Bare `log` / `config` always rely on the external shell
    #    (less / the configured editor), even under --pty.
    # ------------------------------------------------------------------
    if [ "$need_eval" -eq 0 ]; then
        case "${1:-}" in
            log|config)
                [ "$#" -eq 1 ] && need_eval=1
                ;;
        esac
    fi

    # ------------------------------------------------------------------
    # 4. Legacy path: capture stdout, then eval it in the parent shell.
    # ------------------------------------------------------------------
    if [ "$need_eval" -eq 1 ]; then
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

    # ------------------------------------------------------------------
    # 5. PTY path: pass everything straight through.
    # ------------------------------------------------------------------
    _pk_binary "$@"
    return $?
}
