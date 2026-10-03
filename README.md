P(ath) K(eeper) Command-Line Tool - A Multi-Functional Command-Line Utility

Project Overview

Path-keeper is a powerful command-line tool designed for easily managing, organizing, and executing frequently used commands. It features multi-level categorization, execution logging, shell aliases, and rich customization options.

Key Improvements

This version includes important bug fixes and new features:

· Fixed missing method implementations
· Resolved shell script integration issues
· Improved error handling and argument passing
· Enhanced interactive command detection
· Stabilized editor mode and multi-line support

Features

· Command Recording: Add, view, and execute command history
· Multi-Level Categorization: Supports command categorization (e.g., 1.2 represents the second command under the first category)
· Configuration Management: Configurable default execution shell and editor
· Recent Command Execution: Quickly re-run recently executed commands
· Shell Integration: Seamless integration with bash/zsh, with automatic detection of interactive programs
· Execution Logging: Optionally record timestamped logs with output capture for each command
· Interactive Search: Fuzzy-find command selection using fzf
· Hash Verification: Verify command integrity and detect configuration tampering
· Shell Aliases: Create convenient shell aliases for frequently used command indices
· Multi-Line Commands: Support editor-based multi-line command input
· Flexible Log Control: Per-command log setting overrides, with global default support
· Dependencies: fzf and tmux (required for search and command logging features)

Usage Tips

1. Quick Execution: Use the -p flag to execute a command without updating the recent record
2. Category Management: Categorize commands sensibly when adding them for easier retrieval later
3. Multi-Line Commands: When adding a command, press Enter without typing anything to enter editor mode
4. Shell Integration: Source the pk.sh script in your shell to ensure proper command execution and terminal handling

Example Workflow

```bash
# View the current directory and run the most recent command (if set)
pk

# Add a new command record
pk -a

# View all records
pk -s

# Execute the command under the first category
pk -e 1

# Execute a specific command (e.g., the second command under the first category)
pk -e 1.2

# Execute without updating the recent record
pk -p 1

# Set the most recent command for quick access
pk -c
```

Command Reference

Main Options

```
-a, --add           Add a new command record (single-line or multi-line editor mode)
-e [index]          Execute a command by index and update the recent record
-p [index]          Execute a command by index without updating the recent record
-s, --show          Display all recorded commands by category
-c, --configure     Set the most recently executed command record
-v, --version       Show version information
-V, --version-verbose
                     Show detailed version and build information
-h, --help          Show detailed help information
```

Subcommands

```
Configuration Management:
  config             Open the configuration file in the default editor
  config -editor     Interactively set the editor (or specify directly)
  config -editor vim Set the editor to a specific command

Advanced Features:
  alias add <name> <index>   Add an alias for quick command execution
  alias remove <name>        Remove an alias
  alias list                 List all aliases
  alias install              Generate the shell alias file (~/.pk_aliases.sh)
  search                     Interactive search using fzf
  verify                     Verify command integrity
  rehash                     Regenerate and save command hash values
  log                        List and view log files
```

Per-Command Log Control

When adding a command, you can choose the logging behavior:

```
y (force log):        Always log command output
n (force no log):     Never log command output
empty/Enter:          Use the global log.enabled setting
```

Shell Integration

The pk.sh script provides the following features:

· Automatic stdout/stderr handling: Properly captures and executes command output
· Interactive program detection: Automatically detects vim, nano, emacs, less, more, htop, man
· Robust command extraction: Handles log-wrapped commands and removes debug output
· Terminal compatibility: Supports bash, zsh, dash, ksh, and fish

Enable shell integration by sourcing pk.sh in your shell configuration file:

```bash
source /usr/local/share/path-keeper/pk.sh
```

Installation Instructions

Dependencies

```bash
# Ubuntu/Debian:
sudo apt update
sudo apt install cmake pkg-config libjsoncpp-dev build-essential
sudo apt install qt5-qmake qt5-default

# CentOS/RHEL:
sudo yum install cmake pkgconfig jsoncpp-devel gcc-c++
sudo yum install qt5-devel

# Fedora:
sudo dnf install cmake pkgconfig jsoncpp-devel gcc-c++
sudo dnf install qt5-devel
```

[!TIP]
This program requires fzf and tmux. To record output, it should be executed within a tmux session.

Install fzf and tmux:

```bash
sudo apt install fzf tmux
```

Build and Install

Clone the repository and build:

```bash
git clone https://github.com/upupwrite/path-keeper.git
cd path-keeper
mkdir build && cd build
cmake ..
make
```

Install to the system (requires sudo privileges):

```bash
sudo make install
```

Enable shell integration by adding to ~/.bashrc or ~/.zshrc:

```bash
source /usr/local/share/path-keeper/pk.sh
```

Security Features

Hash Verification:

Verify that the command configuration has not been tampered with:

```bash
pk verify              # Check command integrity
pk rehash              # Regenerate hash values after manual editing
```

Log File Management:

· Timestamped log entries providing an audit trail
· Per-command execution tracking

Internationalization

Path-keeper supports multiple languages through Qt translations:

· English (en)
· Simplified Chinese (zh_CN)

The application automatically detects the system locale and loads the corresponding translation.

Testing

Build and run tests (if available):

```bash
cd build
cmake .. -DBUILD_TESTS=ON
make
ctest
```

Uninstallation

Run from the build directory:

```bash
sudo make uninstall
```

Or manually uninstall:

```bash
sudo rm /usr/local/bin/pk
sudo rm -rf /usr/local/share/path-keeper
```

Or manually remove installed files:

```bash
sudo rm /usr/local/bin/pk
```

Troubleshooting

Issue: "pk: binary not found"
Solution: Ensure /usr/local/bin is in your PATH

```bash
echo $PATH
which pk
```

Issue: "No editors found"
Solution: Install a common editor

```bash
sudo apt install vim    # or nano, emacs, etc.
```

Issue: "Failed to load translation"
Solution: Verify Qt5/Qt6 installation

```bash
pkg-config --modversion Qt5Core
```

Issue: Commands fail to execute
Solution: Check whether shell integration has been sourced

```bash
grep "source.*pk.sh" ~/.bashrc
# If not found, add it to ~/.bashrc:
echo "source /usr/local/share/path-keeper/pk.sh" >> ~/.bashrc
source ~/.bashrc
```

Issue: Log files are not being created
Solution: Ensure the log directory exists and is writable

```bash
mkdir -p ~/.pk_logs
chmod 700 ~/.pk_logs
```

Notes

1. Command records are stored in a local file (~/.pk.json). Remember to back up important records.
2. When using the -e flag, ensure the index is valid.
3. When adding commands, you can choose the logging behavior for each command.
4. Multi-line commands can be entered using editor mode.
5. Shell variables and aliases can be used in recorded commands.
6. Environment variables are inherited from the directory specified in the record.

Advanced Features

Search and Filter:

Use the search subcommand for fuzzy-find command selection:

```bash
pk search              # Interactive search using fzf (requires fzf)
```

View Logs:

List and view execution logs:

```bash
pk log                 # Display available log files
```

Contribution Guidelines

Contributions and suggestions for improvement are welcome! Please follow open-source community guidelines and submit a PR or Issue.

Open Source License

This project follows an open-source license. For details, please refer to the LICENSE file in the project root directory.

This project uses the GNU General Public License v3.0 (GPLv3).

For details, please refer to the LICENSE file in the project root directory.

More information about GPLv3: https://www.gnu.org/licenses/gpl-3.0.html

Support and More Information

For more details about the code, you can refer to the source code of the corresponding modules.

GitHub: https://github.com/upupwrite/path-keeper
Author: upupwrite
