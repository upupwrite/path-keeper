# P(ath) K(eeper) Command Tool - Multifunctional Command Line Tool

## Project Overview

Path-keeper is a powerful command-line tool for managing, organizing, and executing frequently-used commands with ease. It features multi-level categorization, execution logging, shell aliases, and extensive customization options.

### Key Improvements

This version includes significant bug fixes and new features:
- Fixed missing method implementations
- Resolved shell script integration issues  
- Improved error handling and parameter passing
- Enhanced interactive command detection
- Stabilized editor mode and multi-line support

## Features

- Command Recording: Add, view, and execute command history
- Multi-level Categorization: Supports command categorization (e.g., 1.2 represents the second command under the first category)
- Configuration Management: Configurable default execution shell and editor
- Recent Command Execution: Quickly re-run your most recently executed command
- Shell Integration: Seamless bash/zsh integration with automatic interactive program detection
- Execution Logging: Optional per-command logging with timestamp and output capture
- Interactive Search: Use fzf for fuzzy-find command selection
- Hash Verification: Verify command integrity to detect configuration tampering
- Shell Aliases: Create convenient shell aliases for frequently used command indices
- Multi-line Commands: Support for editor-based multi-line command entry
- Flexible Log Control: Per-command logging override with global fallback settings
- Fzf and tmux needing if you want to use searching and command logging functions

## Usage Tips

1. Quick Execution: Use the `-p` parameter to execute a command without updating the recent record
2. Categorization Management: Properly categorize commands when adding them for easier retrieval
3. Multi-line Commands: Press enter without input to open editor mode when adding commands
4. Shell Integration: Source the pk.sh script in your shell for proper command execution and terminal handling

## Example Workflow

```bash
# View current directory and run recent command (if set)
pk

# Add a new command record
pk -a

# View all records
pk -s

# Execute the first category command
pk -e 1

# Execute a specific command (e.g., 2nd command in 1st category)
pk -e 1.2

# Execute without updating recent record
pk -p 1

# Set the recent command for quick access
pk -c
```

## Command Reference

### Main Options

```
-a, --add           Add new command record (single-line or multi-line editor mode)
-e [index]          Execute command by index and update recent record
-p [index]          Execute command by index without updating recent record  
-s, --show          Display all recorded commands organized by category
-c, --configure     Set the recent command record
-v, --version       Display version information
-V, --version-verbose
                    Display detailed version and build information
-h, --help          Show detailed help message
```

### Subcommands

```
Configuration Management:
  config              Open config file in default editor
  config -editor      Set editor interactively (or specify directly)
  config -editor vim  Set editor to specific command

Advanced Features:
  alias add <name> <index>  Add alias for quick command execution
  alias remove <name>       Remove alias
  alias list                List all aliases
  alias install             Generate shell alias file (~/.pk_aliases.sh)
  search                    Interactive search with fzf
  verify                    Verify command integrity
  rehash                    Regenerate and save command hashes
  log                       List and view log files
```

## Per-Command Logging Control

When adding commands, you can choose logging behavior:

```
y (Force Log):        Command output always logged
n (Force No Log):     Command output never logged
Empty/Enter:          Uses global log.enabled setting
```

## Shell Integration

The pk.sh script provides the following features:

- Automatic Stdout/Stderr Handling: Command output properly captured and executed
- Interactive Program Detection: Automatically detects vim, nano, emacs, less, more, htop, man
- Robust Command Extraction: Handles logging wrapper commands and removes debug output
- Terminal Compatibility: Works with bash, zsh, dash, ksh, and fish

Shell integration is enabled by sourcing pk.sh in your shell configuration:

```bash
source /usr/local/share/path-keeper/pk.sh
```

## Installation Instructions

### Prerequisites

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
> [!TIP]
> This programe needs fzf and tmux.It should be executed in a tmux session to record outputs.

Install fzf and tmux:  
```bash
sudo apt install fzf tmux
```


### Build and Install

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
You can add pk initiate script with this command:  
```bash
echo 'source /usr/local/share/path-keeper/pk.sh' >> ~/.bashrc
```
## Security Features

Hash Verification:

Verify that your command configuration has not been tampered with:

```bash
pk verify              # Check command integrity
pk rehash              # Regenerate hashes after manual edits
```

Log File Management:

- Timestamped log entries for audit trail
- Per-command execution tracking

## Internationalization

Path-keeper supports multiple languages through Qt translations:

- English (en)
- Simplified Chinese (zh_CN)

The application automatically detects system locale and loads appropriate translations.

## Testing

To build and run tests (if available):

```bash
cd build
cmake .. -DBUILD_TESTS=ON
make
ctest
```

## Uninstallation Method

Run in the build directory:

```bash
sudo make uninstall
```

Alternatively, uninstall manually:

```bash
sudo rm /usr/local/bin/pk
sudo rm -rf /usr/local/share/path-keeper
```

Or manually delete installed files:

```bash
sudo rm /usr/local/bin/pk
```

## Troubleshooting

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

Issue: Commands not executing
Solution: Check shell integration is sourced

```bash
grep "source.*pk.sh" ~/.bashrc
# If not found, add to ~/.bashrc:
echo "source /usr/local/share/path-keeper/pk.sh" >> ~/.bashrc
source ~/.bashrc
```

Issue: Log files not created
Solution: Ensure log directory exists and is writable

```bash
mkdir -p ~/.pk_logs
chmod 700 ~/.pk_logs
```

## Notes

1. Command records are saved in a local file (~/.pk.json). Remember to back up important records.
2. When using the -e parameter, ensure the index is valid.
3. When adding commands, you can choose per-command logging behavior.
4. Multi-line commands can be entered using the editor mode.
5. Shell variables and aliases work in recorded commands.
6. Environment is preserved from the directory specified in the record.

## Advanced Features

Search and Filter:

Use the search subcommand for fuzzy-find command selection:

```bash
pk search              # Interactive search with fzf (requires fzf)
```

View Logs:

List and view execution logs:

```bash
pk log                 # Show available log files
```

## Contribution Guidelines

Contributions and improvement suggestions are welcome! Please follow open-source community norms and submit PRs or Issues.

## Open Source License

This project adheres to an open-source license. For details, please refer to the LICENSE file in the project root directory.

This project is licensed under the GNU General Public License v3.0 (GPLv3)

See the LICENSE file in the project root directory for full details.

For more information about GPLv3: https://www.gnu.org/licenses/gpl-3.0.html

## Support and Further Information

For further details about the code, you can review the source code of the corresponding modules.

GitHub: https://github.com/upupwrite/path-keeper
Author: upupwrite
