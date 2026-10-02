// Example C++ consumer of the Rust editor library.
//
//   * Runs the editor interactively.
//   * Writes the buffer to `output.json` as a single escaped string.
//
// Build (Linux/macOS):
//   cargo build --release
//   g++ -std=c++17 -Iinclude -o app cpp/main.cpp \
//       -Ltarget/release -leditor -lpthread -ldl -lm

#include "editor.h"

#include <cstdio>
#include <fstream>
#include <string>

int main() {
    // --- 1. Run the editor and collect the escaped buffer --------------
    char *raw = editor_run_and_get_json();
    if (raw == nullptr) {
        std::fprintf(stderr, "editor_run_and_get_json() failed\n");
        return 1;
    }

    // Copy into a std::string so we can free the Rust allocation right away
    // instead of holding it across the file I/O below.
    std::string escaped(raw);
    editor_free_string(raw);

    // --- 2. Emit a small JSON document --------------------------------
    std::ofstream out("output.json", std::ios::binary | std::ios::trunc);
    if (!out) {
        std::fprintf(stderr, "cannot open output.json for writing\n");
        return 1;
    }
    out << "{\n";
    out << "  \"content\": \"" << escaped << "\"\n";
    out << "}\n";
    out.close();

    std::printf("wrote %zu escaped bytes to output.json\n", escaped.size());

    // --- 3. (Optional) demonstrate the standalone escaper -------------
    char *demo = editor_json_escape("tab:\there\nnewline above");
    if (demo != nullptr) {
        std::printf("editor_json_escape demo: %s\n", demo);
        editor_free_string(demo);
    }

    return 0;
}
