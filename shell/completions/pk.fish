# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Path Keeper Contributors
# Fish completion for path-keeper (pk).

# The pk() wrapper captures stdout for normal commands, so use `command pk`.
complete -c pk -f -n '__fish_use_subcommand' \
    -a '(command pk completion commands 2>/dev/null)'

complete -c pk -f -n '__fish_seen_subcommand_from alias' \
    -a 'add remove list install'
complete -c pk -f -n '__fish_seen_subcommand_from remove' \
    -a '(command pk completion aliases 2>/dev/null)'
complete -c pk -f -n '__fish_seen_subcommand_from add' \
    -a '(command pk completion indices 2>/dev/null)'

complete -c pk -f -n '__fish_seen_subcommand_from completion complete' \
    -a 'commands indices aliases directories editors'

complete -c pk -f -n '__fish_seen_subcommand_from config' -a '-editor'
complete -c pk -f -n '__fish_seen_subcommand_from -editor' \
    -a '(command pk completion editors 2>/dev/null)'

complete -c pk -f -n '__fish_seen_subcommand_from -e --execute -p --point -c --configure -r --run run log' \
    -a '(command pk completion indices 2>/dev/null)'
complete -c pk -f -n '__fish_seen_subcommand_from log' \
    -a '--enable --disable global'
