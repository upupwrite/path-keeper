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
        -e|""|search|log)
            local cmd_output
            local ret
            # 捕获标准输出和退出码（标准错误直接透传）
            cmd_output=$(_pk_binary "$@")
            ret=$?
            if [ $ret -eq 0 ] && [ -n "$cmd_output" ]; then
                # 若希望先显示原命令，取消下行注释
                # printf '%s\n' "$cmd_output" >&2   # 输出到 stderr 避免干扰管道
                eval "$cmd_output"
                ret=$?   # 取 eval 的退出码
            else
                # 若出错但有输出（如错误信息来自 stdout，但通常错误走 stderr），则打印
                if [ -n "$cmd_output" ]; then
                    printf '%s\n' "$cmd_output"
                fi
                # ret 保留 _pk_binary 的退出码
            fi
            return $ret
            ;;
        *)
            _pk_binary "$@"
            ;;
    esac
}
