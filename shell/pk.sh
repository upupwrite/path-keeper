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

pk() {
    case "${1:-}" in
        -e|""|search|log|config|-p)
            local cmd_output
            local ret
            cmd_output=$(_pk_binary "$@")
            ret=$?
            if [ $ret -eq 0 ] && [ -n "$cmd_output" ]; then
                # printf '%s\n' "$cmd_output" >&2
                eval "$cmd_output"
                ret=$?
            else
                if [ -n "$cmd_output" ]; then
                    printf '%s\n' "$cmd_output"
                fi
            fi
            return $ret
            ;;
        *)
            _pk_binary "$@"
            ;;
    esac
}
