//! A tiny terminal editor, plus a C ABI surface that lets C++ drive it
//! and retrieve the buffer as a JSON-escaped string.
//!
//! The exported functions live at the bottom of this file; everything above
//! them is the standalone editor implementation.

use crossterm::{
    cursor::{Hide, MoveTo, Show},
    event::{
        self, DisableBracketedPaste, DisableMouseCapture, EnableBracketedPaste,
        EnableMouseCapture, Event, KeyCode, KeyEventKind, KeyModifiers, MouseButton,
        MouseEvent, MouseEventKind,
    },
    execute, queue,
    style::{Color, Print, ResetColor, SetBackgroundColor, SetForegroundColor},
    terminal::{
        self, EnterAlternateScreen, LeaveAlternateScreen, disable_raw_mode, enable_raw_mode,
    },
};
use std::ffi::{CStr, CString};
use std::io::{self, Write};
use std::os::raw::c_char;

/// Directions the cursor can be moved in.
enum Move {
    Left,
    Right,
    Up,
    Down,
}

// ----------------------------------------------------------------------
// Double-buffered screen
// ----------------------------------------------------------------------

/// One character cell in the screen buffer: glyph plus its colours.
#[derive(Clone, Copy, PartialEq)]
struct Cell {
    ch: char,
    fg: Color,
    bg: Color,
}

impl Default for Cell {
    fn default() -> Self {
        Cell {
            ch: ' ',
            fg: Color::Reset,
            bg: Color::Reset,
        }
    }
}

/// The last frame that was actually written to the terminal.
///
/// The renderer builds the *next* frame in memory and compares it against this
/// buffer; only cells that differ are pushed to the terminal. This removes the
/// full-screen clear that used to cause flicker.
struct Screen {
    width: u16,
    height: u16,
    buffer: Vec<Cell>,
}

impl Screen {
    fn new() -> Self {
        Self {
            width: 0,
            height: 0,
            buffer: Vec::new(),
        }
    }

    /// Reallocate the buffer for a new terminal size.
    ///
    /// Every cell is marked invalid (`'\0'`) so the next render repaints the
    /// whole screen instead of leaving stale glyphs behind.
    fn resize(&mut self, w: u16, h: u16) {
        if self.width == w && self.height == h {
            return;
        }
        self.width = w;
        self.height = h;
        self.buffer = vec![
            Cell {
                ch: '\0',
                fg: Color::Reset,
                bg: Color::Reset,
            };
            (w as usize) * (h as usize)
        ];
    }
}

// ----------------------------------------------------------------------
// Editor
// ----------------------------------------------------------------------

struct Editor {
    /// One `String` per line. Always contains at least one line.
    content: Vec<String>,
    /// Cursor row (index into `content`).
    row: u16,
    /// Cursor column, counted in characters (not bytes).
    column: u16,
    /// Terminal width in columns.
    width: u16,
    /// Terminal height in rows.
    height: u16,
    /// First buffer row currently displayed.
    row_offset: u16,
    /// First buffer column currently displayed.
    column_offset: u16,
    /// Set when the screen needs to be repainted.
    dirty: bool,
    /// Last frame written to the terminal.
    screen: Screen,
}

impl Editor {
    fn new() -> Self {
        Self {
            content: vec![String::new()],
            row: 0,
            column: 0,
            height: 0,
            width: 0,
            row_offset: 0,
            column_offset: 0,
            dirty: true,
            screen: Screen::new(),
        }
    }

    // ------------------------------------------------------------------
    // Layout helpers
    // ------------------------------------------------------------------

    /// Width of the line-number gutter: digits of the largest line number
    /// plus one separating space.
    fn gutter_width(&self) -> u16 {
        let digits = self.content.len().to_string().len() as u16;
        digits + 1
    }

    /// Rows available for text (the last terminal row is the status bar).
    fn text_height(&self) -> u16 {
        self.height.saturating_sub(1)
    }

    /// Columns available for text (everything right of the gutter).
    fn text_width(&self) -> u16 {
        self.width.saturating_sub(self.gutter_width())
    }

    /// Number of characters in the line the cursor is on.
    fn current_line_len(&self) -> u16 {
        self.content
            .get(self.row as usize)
            .map(|s| s.chars().count() as u16)
            .unwrap_or(0)
    }

    /// Byte offset of the `n`-th character in a line (`line.len()` if past end).
    fn byte_at(line: &str, n: usize) -> usize {
        line.char_indices()
            .nth(n)
            .map(|(i, _)| i)
            .unwrap_or(line.len())
    }

    // ------------------------------------------------------------------
    // Editing
    // ------------------------------------------------------------------

    /// Insert a character at the cursor and advance one column.
    fn insert_char(&mut self, c: char) {
        let row = self.row as usize;
        if row >= self.content.len() {
            return;
        }
        let col = (self.column as usize).min(self.content[row].chars().count());
        let byte_idx = Self::byte_at(&self.content[row], col);
        self.content[row].insert(byte_idx, c);
        self.column += 1;
        self.dirty = true;
        self.ensure_visible();
    }

    /// Split the current line at the cursor (Enter key).
    fn enter(&mut self) {
        let row = self.row as usize;
        if row >= self.content.len() {
            return;
        }
        let col = (self.column as usize).min(self.content[row].chars().count());
        let byte_idx = Self::byte_at(&self.content[row], col);
        let rest = self.content[row].split_off(byte_idx);
        self.content.insert(row + 1, rest);
        self.row += 1;
        self.column = 0;
        self.dirty = true;
        self.ensure_visible();
    }

    /// Delete the character *before* the cursor (Backspace).
    ///
    /// At the start of a line the current line is appended to the previous
    /// one, which is the behaviour every editor has.
    fn backspace(&mut self) {
        let row = self.row as usize;
        if row >= self.content.len() {
            return;
        }
        if self.column > 0 {
            let col = self.column as usize;
            let len = self.content[row].chars().count();
            if col <= len {
                let start = Self::byte_at(&self.content[row], col - 1);
                let end = Self::byte_at(&self.content[row], col);
                self.content[row].replace_range(start..end, "");
                self.column -= 1;
            }
        } else if row > 0 {
            // Join with the previous line.
            let current = self.content.remove(row);
            let prev_len = self.content[row - 1].chars().count() as u16;
            self.content[row - 1].push_str(&current);
            self.row -= 1;
            self.column = prev_len;
        }
        self.dirty = true;
        self.ensure_visible();
    }

    /// Delete the character *under* the cursor (Delete).
    ///
    /// At the end of a line the following line is appended to the current one.
    fn delete(&mut self) {
        let row = self.row as usize;
        if row >= self.content.len() {
            return;
        }
        let col = self.column as usize;
        let len = self.content[row].chars().count();
        if col < len {
            let start = Self::byte_at(&self.content[row], col);
            let end = Self::byte_at(&self.content[row], col + 1);
            self.content[row].replace_range(start..end, "");
        } else if row + 1 < self.content.len() {
            let next = self.content.remove(row + 1);
            self.content[row].push_str(&next);
        }
        self.dirty = true;
        self.ensure_visible();
    }

    /// Paste a (possibly multi-line) string at the cursor.
    ///
    /// Newlines split the buffer; carriage returns are stripped so that both
    /// Unix (`\n`) and Windows (`\r\n`) line endings work.
    fn paste(&mut self, text: &str) {
        let mut first = true;
        for line in text.split('\n') {
            if !first {
                self.enter();
            }
            let line = line.trim_end_matches('\r');
            for c in line.chars() {
                self.insert_char(c);
            }
            first = false;
        }
        self.ensure_visible();
    }

    /// Move the cursor, clamping it into the buffer.
    fn move_cursor(&mut self, kind: Move, step: u16) {
        match kind {
            Move::Left => {
                self.column = self.column.saturating_sub(step);
            }
            Move::Right => {
                let max = self.current_line_len();
                self.column = self.column.saturating_add(step).min(max);
            }
            Move::Up => {
                self.row = self.row.saturating_sub(step);
            }
            Move::Down => {
                let max = self.content.len().saturating_sub(1) as u16;
                self.row = self.row.saturating_add(step).min(max);
            }
        }
        // A shorter line may leave the cursor past its end.
        let line_len = self.current_line_len();
        self.column = self.column.min(line_len);

        self.dirty = true;
        self.ensure_visible();
    }

    fn move_to_line_start(&mut self) {
        self.column = 0;
        self.dirty = true;
        self.ensure_visible();
    }

    fn move_to_line_end(&mut self) {
        self.column = self.current_line_len();
        self.dirty = true;
        self.ensure_visible();
    }

    // ------------------------------------------------------------------
    // Scrolling
    // ------------------------------------------------------------------

    /// Scroll the viewport so that the cursor is visible.
    ///
    /// This is deliberately *not* called from `render`: mouse-wheel scrolling
    /// must be able to move the viewport without snapping back to the cursor.
    fn ensure_visible(&mut self) {
        // Vertical scrolling.
        let height = self.text_height().max(1) as u32;
        let row = self.row as u32;
        let max_offset = (self.content.len() as u32).saturating_sub(height);
        let mut offset = self.row_offset as u32;

        if row < offset {
            offset = row;
        } else if row >= offset + height {
            offset = row - height + 1;
        }
        offset = offset.min(max_offset);
        self.row_offset = offset as u16;

        // Horizontal scrolling.
        let width = self.text_width().max(1) as u32;
        let col = self.column as u32;
        let mut col_off = self.column_offset as u32;

        if col < col_off {
            col_off = col;
        } else if col >= col_off + width {
            col_off = col - width + 1;
        }
        let line_len = self.current_line_len() as u32;
        let max_col_off = line_len.saturating_sub(1);
        col_off = col_off.min(max_col_off);
        self.column_offset = col_off as u16;
    }

    // ------------------------------------------------------------------
    // Mouse
    // ------------------------------------------------------------------

    /// Handle a mouse event. Selection is intentionally *not* supported:
    /// dragging does nothing.
    fn handle_mouse(&mut self, m: MouseEvent) {
        match m.kind {
            MouseEventKind::ScrollUp => {
                // Pure viewport scroll; the cursor stays where it is.
                self.row_offset = self.row_offset.saturating_sub(3);
                self.dirty = true;
            }
            MouseEventKind::ScrollDown => {
                let max = (self.content.len() as u16)
                    .saturating_sub(self.text_height().max(1));
                self.row_offset = (self.row_offset + 3).min(max);
                self.dirty = true;
            }
            MouseEventKind::Down(MouseButton::Left) => {
                let status_row = self.height.saturating_sub(1);
                if m.row >= status_row {
                    return; // Click on the status bar: ignore.
                }

                let buf_row = (m.row + self.row_offset) as usize;
                if buf_row >= self.content.len() {
                    return;
                }
                self.row = buf_row as u16;

                let gutter = self.gutter_width();
                if m.column >= gutter {
                    let buf_col = (m.column - gutter) as usize
                        + self.column_offset as usize;
                    let len = self.content[buf_row].chars().count();
                    self.column = buf_col.min(len) as u16;
                } else {
                    self.column = 0;
                }

                self.dirty = true;
                self.ensure_visible();
            }
            // Everything else (drags, right clicks, ...) is ignored.
            _ => {}
        }
    }

    // ------------------------------------------------------------------
    // Drawing
    // ------------------------------------------------------------------

    fn render(&mut self, stdout: &mut impl Write) -> io::Result<()> {
        let width = self.width;
        let height = self.height;
        if width == 0 || height == 0 {
            self.dirty = false;
            return Ok(());
        }

        self.screen.resize(width, height);

        // ---- Build the next frame in memory -----------------------------
        let mut frame = vec![Cell::default(); (width as usize) * (height as usize)];

        let gutter = self.gutter_width() as usize;
        let text_width = (width as usize).saturating_sub(gutter);
        let text_height = self.text_height() as usize;
        let row_offset = self.row_offset as usize;
        let col_offset = self.column_offset as usize;

        // Text area: line numbers + highlighted source.
        for (i, text) in self
            .content
            .iter()
            .enumerate()
            .skip(row_offset)
            .take(text_height)
        {
            let y = i - row_offset;
            let base = y * width as usize;

            // Line number, right aligned inside the gutter.
            let number = format!("{:>width$} ", i + 1, width = gutter - 1);
            for (x, ch) in number.chars().enumerate() {
                if x >= gutter {
                    break;
                }
                frame[base + x] = Cell {
                    ch,
                    fg: Color::DarkGrey,
                    bg: Color::Reset,
                };
            }

            // Visible slice of the line, painted with shell colours.
            let colors = tokenize_shell(text);
            let chars: Vec<char> = text.chars().collect();
            for (k, ch) in chars.iter().enumerate().skip(col_offset).take(text_width) {
                let x = gutter + (k - col_offset);
                frame[base + x] = Cell {
                    ch: *ch,
                    fg: colors.get(k).copied().flatten().unwrap_or(Color::Reset),
                    bg: Color::Reset,
                };
            }
        }

        // Status bar on the very last row.
        let status_y = (height - 1) as usize;
        let status_base = status_y * width as usize;
        let status = format!(
            " shell | Ln {}, Col {} | {} lines | Ctrl-Q quit ",
            self.row + 1,
            self.column + 1,
            self.content.len()
        );
        let mut bar: Vec<char> = status.chars().take(width as usize).collect();
        while bar.len() < width as usize {
            bar.push(' ');
        }
        for (x, ch) in bar.into_iter().enumerate() {
            frame[status_base + x] = Cell {
                ch,
                fg: Color::Black,
                bg: Color::White,
            };
        }

        // ---- Flush only the cells that changed --------------------------
        queue!(stdout, Hide)?;

        let mut cur_fg = Color::Reset;
        let mut cur_bg = Color::Reset;

        for y in 0..height as usize {
            for x in 0..width as usize {
                let idx = y * width as usize + x;
                if frame[idx] == self.screen.buffer[idx] {
                    continue;
                }
                let cell = frame[idx];

                // Always reposition: some glyphs (CJK, emoji) are double-width
                // and would otherwise shift the cursor by two cells.
                queue!(stdout, MoveTo(x as u16, y as u16))?;

                if cell.fg != cur_fg {
                    queue!(stdout, SetForegroundColor(cell.fg))?;
                    cur_fg = cell.fg;
                }
                if cell.bg != cur_bg {
                    queue!(stdout, SetBackgroundColor(cell.bg))?;
                    cur_bg = cell.bg;
                }
                queue!(stdout, Print(cell.ch))?;
            }
        }

        if cur_fg != Color::Reset || cur_bg != Color::Reset {
            queue!(stdout, ResetColor)?;
        }

        // ---- Place the terminal cursor ----------------------------------
        let cursor_visible = self.row >= self.row_offset
            && (self.row - self.row_offset) < self.text_height()
            && self.column >= self.column_offset;

        if cursor_visible {
            let cx = gutter as u16 + (self.column - self.column_offset);
            let cy = self.row - self.row_offset;
            if cx < width {
                queue!(stdout, MoveTo(cx, cy))?;
            } else {
                queue!(stdout, MoveTo(width.saturating_sub(1), cy))?;
            }
        } else {
            // Cursor scrolled out of view (mouse wheel): park it on the bar.
            queue!(stdout, MoveTo(width.saturating_sub(1), height - 1))?;
        }
        queue!(stdout, Show)?;

        self.screen.buffer = frame;
        self.dirty = false;
        stdout.flush()?;
        Ok(())
    }

    /// Called when the terminal is resized.
    fn resize(&mut self, w: u16, h: u16) {
        self.width = w;
        self.height = h;

        // Keep the cursor inside the buffer after the viewport shrank.
        let max_row = self.content.len().saturating_sub(1) as u16;
        self.row = self.row.min(max_row);
        let line_len = self.current_line_len();
        self.column = self.column.min(line_len);

        // Clamp the scroll offset so we don't end up below the last line.
        let max_off = (self.content.len() as u16).saturating_sub(self.text_height().max(1));
        self.row_offset = self.row_offset.min(max_off);

        self.ensure_visible();
        self.dirty = true;
    }
}

// ----------------------------------------------------------------------
// Minimal shell syntax highlighter
// ----------------------------------------------------------------------

/// Shell keywords (control-flow words, declarations, ...).
const KEYWORDS: &[&str] = &[
    "if", "then", "else", "elif", "fi", "for", "while", "until", "do", "done", "case",
    "esac", "in", "function", "select", "time", "return", "break", "continue", "local",
    "export", "readonly", "declare", "unset", "shift",
];

/// Common shell builtins / commands.
const BUILTINS: &[&str] = &[
    "echo", "cd", "pwd", "exit", "printf", "read", "test", "exec", "eval", "trap",
    "kill", "wait", "jobs", "fg", "bg", "umask", "type", "hash", "help", "let",
    "true", "false", "source", "alias", "set",
];

/// Characters that act as shell operators / redirections.
fn is_operator(c: char) -> bool {
    "|&;<>".contains(c)
}

/// Tokenize one line and return an optional colour for every character.
///
/// This is deliberately a *lightweight* lexer: it is not a full POSIX shell
/// parser, it just recognises the constructs that matter visually.
fn tokenize_shell(line: &str) -> Vec<Option<Color>> {
    let chars: Vec<char> = line.chars().collect();
    let mut colors: Vec<Option<Color>> = vec![None; chars.len()];
    let mut i = 0;

    while i < chars.len() {
        let c = chars[i];

        // --- Comment: '#' at the start of a word -----------------------
        if c == '#' && (i == 0 || chars[i - 1].is_whitespace() || chars[i - 1] == ';') {
            for slot in colors.iter_mut().skip(i) {
                *slot = Some(Color::DarkGrey);
            }
            break;
        }

        // --- Single quoted string (no escapes inside) ------------------
        if c == '\'' {
            colors[i] = Some(Color::Green);
            i += 1;
            while i < chars.len() {
                colors[i] = Some(Color::Green);
                let closing = chars[i] == '\'';
                i += 1;
                if closing {
                    break;
                }
            }
            continue;
        }

        // --- Double quoted string (backslash escapes) ------------------
        if c == '"' {
            colors[i] = Some(Color::Green);
            i += 1;
            while i < chars.len() {
                colors[i] = Some(Color::Green);
                if chars[i] == '\\' && i + 1 < chars.len() {
                    colors[i + 1] = Some(Color::Green);
                    i += 2;
                    continue;
                }
                let closing = chars[i] == '"';
                i += 1;
                if closing {
                    break;
                }
            }
            continue;
        }

        // --- Variables: $NAME, ${...}, $(...) --------------------------
        if c == '$' {
            colors[i] = Some(Color::Magenta);
            i += 1;

            if i < chars.len() && chars[i] == '{' {
                let mut depth = 0;
                while i < chars.len() {
                    colors[i] = Some(Color::Magenta);
                    if chars[i] == '{' {
                        depth += 1;
                    } else if chars[i] == '}' {
                        depth -= 1;
                        if depth == 0 {
                            i += 1;
                            break;
                        }
                    }
                    i += 1;
                }
            } else if i < chars.len() && chars[i] == '(' {
                let mut depth = 0;
                while i < chars.len() {
                    colors[i] = Some(Color::Magenta);
                    if chars[i] == '(' {
                        depth += 1;
                    } else if chars[i] == ')' {
                        depth -= 1;
                        if depth == 0 {
                            i += 1;
                            break;
                        }
                    }
                    i += 1;
                }
            } else {
                while i < chars.len() && (chars[i].is_alphanumeric() || chars[i] == '_') {
                    colors[i] = Some(Color::Magenta);
                    i += 1;
                }
            }
            continue;
        }

        // --- Operators and redirections --------------------------------
        if is_operator(c) {
            colors[i] = Some(Color::Red);
            i += 1;
            continue;
        }

        // --- A plain word ----------------------------------------------
        if !c.is_whitespace() {
            let start = i;
            while i < chars.len() {
                let w = chars[i];
                if w.is_whitespace()
                    || w == '\''
                    || w == '"'
                    || w == '$'
                    || w == '#'
                    || is_operator(w)
                {
                    break;
                }
                i += 1;
            }

            let word: String = chars[start..i].iter().collect();
            let color = if KEYWORDS.contains(&word.as_str()) {
                Some(Color::Yellow)
            } else if BUILTINS.contains(&word.as_str()) {
                Some(Color::Cyan)
            } else if word.starts_with('-') && word.len() > 1 {
                // Command line option, e.g. -l or --verbose.
                Some(Color::DarkYellow)
            } else {
                None
            };

            if let Some(color) = color {
                for slot in colors.iter_mut().take(i).skip(start) {
                    *slot = Some(color);
                }
            }
            continue;
        }

        i += 1;
    }

    colors
}

// ----------------------------------------------------------------------
// Interactive session (shared by the CLI and the C ABI)
// ----------------------------------------------------------------------

/// Run the interactive editor until the user presses `Ctrl-Q`.
///
/// Returns the buffer contents, one entry per line. On any I/O error the
/// terminal is still restored before the error is returned, so callers
/// (including C++ code) never have to worry about leaving the terminal in
/// raw mode / alternate screen.
pub fn run_editor_cli() -> io::Result<Vec<String>> {
    let mut stdout = io::stdout();
    enable_raw_mode()?;
    execute!(
        stdout,
        EnterAlternateScreen,
        EnableMouseCapture,
        EnableBracketedPaste,
    )?;

    let mut ed = Editor::new();
    let (w, h) = terminal::size()?;
    ed.width = w;
    ed.height = h;
    ed.screen.resize(w, h);
    execute!(stdout, MoveTo(0, 0))?;
    ed.render(&mut stdout)?;

    let result = run_event_loop(&mut ed, &mut stdout);

    // Teardown runs unconditionally so the terminal is always sane afterwards,
    // even if the event loop bailed out with an error.
    let _ = execute!(
        stdout,
        Show,
        DisableBracketedPaste,
        DisableMouseCapture,
        LeaveAlternateScreen,
    );
    let _ = disable_raw_mode();

    result?;
    Ok(ed.content)
}

/// The inner event loop, factored out so `run_editor_cli` can guarantee cleanup.
fn run_event_loop(ed: &mut Editor, stdout: &mut io::Stdout) -> io::Result<()> {
    loop {
        match event::read()? {
            Event::Key(k) if k.kind == KeyEventKind::Press => {
                match k.code {
                    // Ctrl-Q quits.
                    KeyCode::Char('q') if k.modifiers.contains(KeyModifiers::CONTROL) => {
                        return Ok(());
                    }
                    // Plain characters are inserted into the buffer.
                    KeyCode::Char(c) if !k.modifiers.contains(KeyModifiers::CONTROL) => {
                        ed.insert_char(c);
                    }
                    KeyCode::Enter => ed.enter(),
                    KeyCode::Backspace => ed.backspace(),
                    KeyCode::Delete => ed.delete(),
                    KeyCode::Up => ed.move_cursor(Move::Up, 1),
                    KeyCode::Down => ed.move_cursor(Move::Down, 1),
                    KeyCode::Left => ed.move_cursor(Move::Left, 1),
                    KeyCode::Right => ed.move_cursor(Move::Right, 1),
                    KeyCode::Home => ed.move_to_line_start(),
                    KeyCode::End => ed.move_to_line_end(),
                    KeyCode::PageUp => {
                        let step = ed.text_height().saturating_sub(1).max(1);
                        ed.move_cursor(Move::Up, step);
                    }
                    KeyCode::PageDown => {
                        let step = ed.text_height().saturating_sub(1).max(1);
                        ed.move_cursor(Move::Down, step);
                    }
                    _ => {}
                }
            }
            Event::Mouse(m) => ed.handle_mouse(m),
            // Bracketed paste: arrives as one event, newlines included.
            Event::Paste(text) => ed.paste(&text),
            Event::Resize(w, h) => ed.resize(w, h),
            _ => {}
        }

        if ed.dirty {
            ed.render(stdout)?;
        }
    }
}

// ----------------------------------------------------------------------
// JSON escaping
// ----------------------------------------------------------------------

/// Escape a string so it can be embedded inside a JSON string literal
/// (without the surrounding quotes).
///
/// Every control character — including NUL — is turned into an escape
/// sequence, so the result is guaranteed to contain no `'\0'` bytes and can
/// be safely passed to `CString::new`.
pub fn json_escape(s: &str) -> String {
    let mut out = String::with_capacity(s.len() + 8);
    for c in s.chars() {
        match c {
            '"' => out.push_str("\\\""),
            '\\' => out.push_str("\\\\"),
            '\n' => out.push_str("\\n"),
            '\r' => out.push_str("\\r"),
            '\t' => out.push_str("\\t"),
            '\x08' => out.push_str("\\b"),
            '\x0c' => out.push_str("\\f"),
            c if (c as u32) < 0x20 => {
                use std::fmt::Write as _;
                let _ = write!(out, "\\u{:04x}", c as u32);
            }
            c => out.push(c),
        }
    }
    out
}

/// Join lines with `\n` and JSON-escape the result.
pub fn content_to_json(lines: &[String]) -> String {
    json_escape(&lines.join("\n"))
}

// ----------------------------------------------------------------------
// C ABI
// ----------------------------------------------------------------------
//
// Convention for every function that returns a `*mut c_char`:
//
//   * The pointer owns a heap allocation made by Rust's allocator.
//   * The caller must hand it back to `editor_free_string` — never `free()`.
//   * A NULL return means "failed" or "no data".
//
// Every string is UTF-8 and NUL-terminated.

/// Run the interactive editor and return the buffer as a JSON-escaped string.
///
/// The returned text is the entire buffer joined with `\n` and then escaped
/// so it can be dropped verbatim into a JSON string literal, e.g.:
///
/// ```json
/// { "content": "line one\\nline two" }
/// ```
///
/// # Safety
/// The returned pointer must be released with [`editor_free_string`].
#[unsafe(no_mangle)]
pub extern "C" fn editor_run_and_get_json() -> *mut c_char {
    match run_editor_cli() {
        Ok(lines) => {
            // json_escape strips NUL bytes, so CString::new never fails here.
            match CString::new(content_to_json(&lines)) {
                Ok(c) => c.into_raw(),
                Err(_) => std::ptr::null_mut(),
            }
        }
        Err(_) => std::ptr::null_mut(),
    }
}

/// Escape an existing UTF-8 C string as JSON, without opening the editor.
///
/// Useful for re-escaping text the caller already has, or for testing.
///
/// # Safety
/// `input` must be either NULL or a valid NUL-terminated UTF-8 C string.
/// The returned pointer must be released with [`editor_free_string`].
#[unsafe(no_mangle)]
pub unsafe extern "C" fn editor_json_escape(input: *const c_char) -> *mut c_char {
    if input.is_null() {
        return std::ptr::null_mut();
    }
    // SAFETY: the caller guarantees `input` is a valid NUL-terminated string.
    let s = match unsafe { CStr::from_ptr(input) }.to_str() {
        Ok(s) => s,
        Err(_) => return std::ptr::null_mut(), // not valid UTF-8
    };
    match CString::new(json_escape(s)) {
        Ok(c) => c.into_raw(),
        Err(_) => std::ptr::null_mut(),
    }
}

/// Free a string previously returned by [`editor_run_and_get_json`] or
/// [`editor_json_escape`].
///
/// Passing NULL is a no-op.
///
/// # Safety
/// `ptr` must have come from one of the functions above and must not be
/// used again after this call.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn editor_free_string(ptr: *mut c_char) {
    if ptr.is_null() {
        return;
    }
    // SAFETY: the pointer was produced by `CString::into_raw` in this module,
    // so reclaiming it with `CString::from_raw` is sound.
    unsafe {
        let _ = CString::from_raw(ptr);
    }
}
