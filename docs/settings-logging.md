# Settings & Logging (Phase 28)

Persistence of window layout to INI text and a capture-based text logging system.
Both live under `include/eli/util/` as standalone, header-only modules:

- `eli/util/eli_settings.h` — INI settings store, parse/serialize, disk hooks.
- `eli/util/eli_logging.h` — text capture and flush to TTY / file / clipboard.

Libc is routed through `eli/core/eli_platform.h`; disk and console I/O are
browser-only and guarded by `ELI_JSIO` (host JS hooks). Under hosted/test builds
those paths compile out and behave as documented no-ops.

---

## Settings (INI)

Settings capture per-window geometry into a flat, file-static store. The INI
format matches Dear ImGui:

```
[Window][Alpha]
Pos=10,20
Size=300,200
Collapsed=0

```

Section names are the window's `eli_begin` name. `Pos`/`Size` are integer pixel
pairs; `Collapsed` is `0`/`1`. Windows flagged `ELI_WINDOW_NO_SAVED_SETTINGS`
(children, popups, tooltips, menus) and unnamed windows are not saved.

### API

| Function | Purpose |
|----------|---------|
| `void eli_load_ini_settings_from_memory(const char *ini_data, size_t ini_size)` | Parse INI text into the store. Malformed/orphan lines are ignored; matching names are updated in place. `ini_data` need not be NUL-terminated. |
| `char *eli_save_ini_settings_to_memory(size_t *out_ini_size)` | Serialize the current context's saveable windows to a newly allocated string. Returns `NULL` if there is no current context or on OOM. **Caller frees with `eli_mem_free`.** `out_ini_size` (optional) receives the length. |
| `void eli_load_ini_settings_from_disk(const char *ini_filename)` | Under `ELI_JSIO`, read the file via the host hook and parse it. Hosted builds: no-op (no disk). |
| `void eli_save_ini_settings_to_disk(const char *ini_filename)` | Serialize via the memory path and, under `ELI_JSIO`, write via the host hook. Hosted builds: serializes then frees (no disk write). |
| `bool eli_settings_get_window(const char *name, eli_vec2 *out_pos, eli_vec2 *out_size, bool *out_collapsed)` | Look up a stored entry so a caller can re-apply it before `eli_begin`. Returns `false` if absent. Any out-pointer may be `NULL`. |
| `void eli_settings_clear(void)` | Empty the store and release its memory. |

`eli_save_ini_settings_to_memory` reads the live window registry
(`ctx->windows`, read-only) and uses each window's expanded size (`size_full`).

### Applying a stored entry

Automatic apply-in-`Begin` (auto-restore) is **deferred**. To restore a window,
fetch the entry and feed it to the next-window setters before `eli_begin`:

```c
eli_vec2 pos, size;
bool collapsed;
if (eli_settings_get_window("Alpha", &pos, &size, &collapsed)) {
    eli_set_next_window_pos(pos, ELI_COND_FIRST_USE_EVER, eli_make_vec2(0, 0));
    eli_set_next_window_size(size, ELI_COND_FIRST_USE_EVER);
    eli_set_next_window_collapsed(collapsed, ELI_COND_FIRST_USE_EVER);
}
eli_begin("Alpha", NULL, 0);
/* ... */
eli_end();
```

### Round-trip example

```c
size_t len;
char *ini = eli_save_ini_settings_to_memory(&len);   /* after a frame */
/* ... persist `ini` somewhere, later: */
eli_load_ini_settings_from_memory(ini, len);
eli_mem_free(ini);
```

---

## Logging

Logging captures formatted text into a growable file-static buffer while a
session is active, then flushes it to a chosen target on finish. This mirrors
Dear ImGui's `LogToTTY/LogToFile/LogToClipboard` model.

### API

| Function | Purpose |
|----------|---------|
| `void eli_log_to_tty(int auto_open_depth)` | Start a session flushed to the TTY (dev console under `ELI_JSIO`, else no-op flush). |
| `void eli_log_to_file(int auto_open_depth, const char *filename)` | Start a session flushed to `filename` (host hook under `ELI_JSIO`). |
| `void eli_log_to_clipboard(int auto_open_depth)` | Start a session flushed via `eli_set_clipboard_text`. |
| `void eli_log_text(const char *fmt, ...)` | Append formatted text to the active session (no-op if none active). |
| `void eli_log_text_v(const char *fmt, va_list args)` | `va_list` form of `eli_log_text`. |
| `void eli_log_finish(void)` | End the session and flush the buffer to its target. |
| `void eli_log_buttons(void)` | Render the standard "Log To TTY/File/Clipboard" buttons + a default-depth slider (demo/browser helper). |

`auto_open_depth` is accepted and stored (`-1` = unlimited) for callers/tree
widgets that expand nodes while logging; the expansion itself is out of scope
here. Each `eli_log_text` call is formatted into a bounded stack buffer
(`ELI_LOG_FORMAT_BUFFER`, 1024 bytes) and truncated at that size before append.

### Example

```c
eli_log_to_clipboard(0);
eli_log_text("value = %d\n", 42);
eli_log_text("name = %s\n", "elimgui");
eli_log_finish();               /* the two lines are now on the clipboard */
```

### Gotchas

- Logging routines are no-ops when no session is active; `eli_log_finish`
  without a session is safe.
- The clipboard target uses the IO clipboard callback if installed, otherwise a
  fallback buffer (which is what hosted unit tests read back).
- TTY and file flush require `ELI_JSIO` (browser). In hosted builds those targets
  capture normally but the flush does nothing.
