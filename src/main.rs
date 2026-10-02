//! Thin command-line wrapper around the library.
//!
//! Running `cargo run` drops you into the editor; when you press Ctrl-Q the
//! buffer is printed to stdout, one line per line.

use std::io;

fn main() -> io::Result<()> {
    let lines = editor::run_editor_cli()?;
    for text in &lines {
        println!("{text}");
    }
    Ok(())
}
