# P(ath) K(eeper) Command Tool - Multifunctional Command Line Tool

## Project Overview

Path-keeper is a powerful command-line tool for managing, organizing, and executing frequently-used commands with ease. It features multi-level categorization, execution logging, shell aliases and [...]

### New / Notable Features

This README was updated to document recently added functionality found in the codebase:

- Alias management: add, remove, list and install shell aliases for command indices (subcommands: `alias add`, `alias remove`, `alias list`, `alias install`).
- Editor integration: a lightweight Rust TUI editor is invoked to edit multi-line commands. The editor returns the full buffer as the command content.
- Run-from-editor (runFile): invoke the editor to produce a command and run it in a specified directory (supports PTY when needed).
- Improved path normalization: paths entered by the user are expanded (~), resolved to absolute form and canonicalized when possible.
- Recent-command handling: improved checks and fallback for recent records, with robust error messages.
- Translation loading: the binary loads Qt `.qm` translation files from the installation prefix (share/path-keeper/translations).

These features are implemented in the src/ code (notably src/pk.cpp and src/main.cpp) and in the rust/editor component.

## Features

- Command Recording: Add, view, and execute command history
- Multi-level Categorization: Supports command categorization (e.g., 1.2 represents the second command under the first category)
- Alias Management: Define named aliases that map to saved command indices and install them into a shell file (`~/.pk_aliases.sh`).
- Editor-based Multi-line Commands: Press enter without input (or use `--edit`) to open the Rust TUI editor and save the entire buffer as one command.
- Run-from-editor: Use the `runFile` flow (binary `pk` exposes a `runFile` action) to open the editor, then execute resulting command in a chosen directory using PTY support.
- Execution Logging: Optional per-command logging with timestamp and output capture
- Interactive Search: Use fzf for fuzzy-find command selection
- Hash Verification: Verify command integrity to detect configuration tampering
- Shell Integration: Seamless bash/zsh integration with automatic interactive program detection

## Usage Tips (high level)

- Quick Execution: Use `-p` to execute without updating the recent record
- Alias usage: Create an alias for a frequently used command and then source `~/.pk_aliases.sh` to use `alias_name` directly in your shell to run `pk -e X.Y`.
- Editor mode: Use `pk -a --edit` (or the corresponding option in your environment) to open the embedded Rust editor. The entire buffer is treated as the command.
- Run-from-editor: Use the `runFile` or UI flow which will ask for a directory and open the editor; after you save and exit the editor, the resulting content will be executed in the specified dire[...]

## Installation & Building

(Existing build instructions remain valid: CMake + Qt5/Qt6; see previous sections in the original README for full commands.)

## Internationalization

- The application loads Qt `.qm` translation files from `share/path-keeper/translations/path-keeper_<locale>.qm` relative to install prefix.
- Existing translations in the repository: English (`path-keeper_en.ts`) and Simplified Chinese (`path-keeper_zh_CN.ts`).
- Japanese: there is currently no `path-keeper_ja.ts` or `path-keeper_ja.qm` in the translations directory. To add Japanese translations:
  1. Create `translations/path-keeper_ja.ts` (use `lupdate` / Qt Linguist or copy and edit an existing `.ts`).
  2. Translate strings using Qt Linguist and save.
  3. Run `lrelease translations/path-keeper_ja.ts -qm build/translations/path-keeper_ja.qm` as part of your packaging step or CMake configuration so the binary can load it at runtime.

## Notes for packagers and maintainers

- The Rust-based editor API: `editor_run_and_get_json()` is used to launch the editor and return a JSON-escaped string of the buffer. The code wraps that JSON and parses it with JsonCpp.
- Alias installation writes `~/.pk_aliases.sh` containing shell `alias name='pk -e X.Y'` lines — inform users to source that file in their shell config (e.g. `echo 'source ~/.pk_aliases.sh' >> ~[...]

## Files changed in this documentation update

- README.md (this file): updated to document alias/editor/runFile and i18n notes
- README.cn.md: updated to include the same new features in Chinese

---

For other usage examples and full command reference, the original README sections remain valid. Please report omissions or inaccuracies as issues or PRs. 
