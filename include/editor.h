/*
 * editor.h — C ABI for the Rust terminal editor.
 *
 * All functions are thread-unsafe with respect to the terminal: only one
 * interactive session may be active at a time. Callers that need concurrency
 * must serialise their calls.
 */

#ifndef EDITOR_H
#define EDITOR_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Run the interactive editor and return the buffer as a JSON-escaped,
 * NUL-terminated UTF-8 string.
 *
 * The string is the whole buffer joined with '\n' and then escaped so it can
 * be embedded verbatim inside a JSON string literal (the surrounding quotes
 * are NOT included).
 *
 * Returns NULL on failure.
 *
 * The returned pointer MUST be released with editor_free_string().
 */
char *editor_run_and_get_json(void);

/*
 * JSON-escape an existing NUL-terminated UTF-8 C string.
 *
 * Returns NULL if `input` is NULL or not valid UTF-8.
 * The returned pointer MUST be released with editor_free_string().
 */
char *editor_json_escape(const char *input);

/*
 * Release a string returned by editor_run_and_get_json() or
 * editor_json_escape(). Passing NULL is a no-op.
 *
 * Do NOT call free() on these pointers — they come from Rust's allocator.
 */
void editor_free_string(char *ptr);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* EDITOR_H */
