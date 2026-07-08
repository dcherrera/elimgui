# Input Widgets (Phase 12)

Phase 12 adds editable **text** and **numeric** input widgets. They build on the
Phase 9 interaction/render core, the Phase 8 layout contract (measure →
`eli_item_size` → `eli_item_add`), the Phase 5 active-id tracking, the Phase 4
keyboard/text/clipboard input, and the draw/font systems for cursor and selection
rendering.

Headers (include directly — they are not yet part of the `eli_widgets.h`
aggregator):

| Header | Contents |
|--------|----------|
| `eli/widgets/eli_input_text_widget.h` | `eli_input_text`, `_multiline`, `_with_hint`, `eli_input_text_flags`, callback contract |
| `eli/widgets/eli_input_number.h` | `eli_input_float[2/3/4]`, `eli_input_int[2/3/4]`, `eli_input_double`, `eli_input_scalar[_n]` |

All widgets operate on the current window opened by `eli_begin` / closed by
`eli_end`, and are no-ops returning `false` outside a window scope.

---

## Editing model

A single field edits at a time. The editing state (active id, caret byte offset,
selection anchor, horizontal scroll, one-level undo snapshot, and an escape-revert
snapshot) is **module-private and file-static** inside `eli_input_text_widget.h`.

- **Activate:** clicking inside the frame sets the active id and snapshots the
  buffer. With `ELI_INPUT_TEXT_AUTO_SELECT_ALL` the whole text is selected.
- **Edit:** the caller's buffer is mutated **in place**. Every change calls
  `eli_mark_item_edited`, so `eli_is_item_edited()` reflects the edit.
- **Commit:** Enter (single-line) or clicking elsewhere deactivates and keeps the
  buffer; `eli_is_item_deactivated()` / `_after_edit()` fire on that frame.
- **Cancel:** Escape reverts to the activation snapshot (or empties the buffer with
  `ELI_INPUT_TEXT_ESCAPE_CLEARS_ALL`) and deactivates.

Supported single-line editing: character insert (UTF-8 encoded from the IO text
queue), Backspace/Delete, Left/Right by codepoint, Home/End, shift-selection,
Ctrl+A select-all, Ctrl+C/X/V copy/cut/paste via the clipboard API, Ctrl+Z
one-level undo, char filtering, and password display. Multiline shares the core and
adds newline insertion plus Up/Down line navigation.

---

## Text input

### `eli_input_text_flags`

Behavior flags (subset most-used):

- `ELI_INPUT_TEXT_CHARS_DECIMAL` / `_HEXADECIMAL` / `_SCIENTIFIC` — restrict typed
  characters (digits plus the sign/point/exponent characters each set allows).
- `ELI_INPUT_TEXT_CHARS_UPPERCASE` — map `a-z` to `A-Z`.
- `ELI_INPUT_TEXT_CHARS_NO_BLANK` — reject spaces/tabs.
- `ELI_INPUT_TEXT_ALLOW_TAB_INPUT` — insert a literal tab.
- `ELI_INPUT_TEXT_ENTER_RETURNS_TRUE` — return `true` only on Enter (instead of on
  every edit).
- `ELI_INPUT_TEXT_ESCAPE_CLEARS_ALL` — Escape empties instead of reverting.
- `ELI_INPUT_TEXT_CTRL_ENTER_FOR_NEWLINE` — in multiline, Ctrl+Enter inserts a
  newline and Enter submits (roles swapped).
- `ELI_INPUT_TEXT_READ_ONLY`, `ELI_INPUT_TEXT_PASSWORD`,
  `ELI_INPUT_TEXT_AUTO_SELECT_ALL`, `ELI_INPUT_TEXT_NO_HORIZONTAL_SCROLL`,
  `ELI_INPUT_TEXT_NO_UNDO_REDO`.

### `bool eli_input_text(const char *label, char *buf, size_t buf_size, eli_input_text_flags flags, eli_input_text_callback callback, void *user_data)`

A single-line editor bound to a caller-owned, NUL-terminated `buf` of capacity
`buf_size`. The visible label is drawn to the right of the frame (hidden after
`##`). Returns `true` when the value changed this frame — or, with
`ENTER_RETURNS_TRUE`, only on the Enter frame.

### `bool eli_input_text_multiline(const char *label, char *buf, size_t buf_size, eli_vec2 size, eli_input_text_flags flags, eli_input_text_callback callback, void *user_data)`

A multi-line editor. `size` sets the frame; `0` components select defaults
(width = item width, height = 8 text lines). Enter inserts a newline by default.

### `bool eli_input_text_with_hint(const char *label, const char *hint, char *buf, size_t buf_size, eli_input_text_flags flags, eli_input_text_callback callback, void *user_data)`

Like `eli_input_text` but draws `hint` in the disabled-text color while the buffer
is empty **and** the field is inactive.

### Callback

```c
typedef int (*eli_input_text_callback)(eli_input_text_callback_data *data);
```

`eli_input_text_callback_data` carries the buffer, length, capacity, caret and
selection offsets, and the `event_flag`. The widget currently invokes the callback
for the `CALLBACK_ALWAYS` / `CALLBACK_EDIT` events; a callback may edit `buf` and set
`buf_dirty` to have the widget re-measure it. `CHARS_*` filtering is applied natively
before insertion.

---

## Numeric input

Numeric widgets format the bound value into a text field, parse edits back on any
change, and optionally show `-` / `+` step buttons. While a field is being edited
its text is left untouched so keystrokes survive across frames; when inactive it is
reformatted from the bound value each frame. Values are clamped to the target type's
representable range on store.

### `bool eli_input_scalar(const char *label, eli_data_type data_type, void *p_data, const void *p_step, const void *p_step_fast, const char *format, eli_input_text_flags flags)`

The generic single-component input. `p_step` (and `p_step_fast`, used while Ctrl is
held) point to a value of `data_type`; pass `NULL` to hide the step buttons.
`format` is a printf format, or `NULL` for the type default (`%.3f` for reals, `%d`
/ `%u` for integers). Returns `true` when the bound value changed.

### `bool eli_input_scalar_n(const char *label, eli_data_type data_type, void *p_data, int components, const void *p_step, const void *p_step_fast, const char *format, eli_input_text_flags flags)`

A row of `components` scalar fields over a contiguous array, sharing one label.

### Typed wrappers

- `eli_input_float(label, float *v, step, step_fast, format, flags)` and
  `eli_input_float2/3/4(label, v[N], format, flags)`.
- `eli_input_int(label, int *v, step, step_fast, flags)` and
  `eli_input_int2/3/4(label, v[N], flags)`.
- `eli_input_double(label, double *v, step, step_fast, format, flags)`.

`step`/`step_fast` of `0` hides the buttons.

---

## Example

```c
#include <eli/widgets/eli_input_text_widget.h>
#include <eli/widgets/eli_input_number.h>

static char g_name[64];
static int  g_count = 1;
static float g_scale = 1.0f;

void draw_form(void)
{
    if (eli_input_text_with_hint("Name", "enter a name", g_name, sizeof(g_name),
                                 ELI_INPUT_TEXT_ENTER_RETURNS_TRUE, NULL, NULL)) {
        /* fires on Enter */
    }
    eli_input_int("Count", &g_count, 1, 10, 0);          /* -/+ step 1, Ctrl = 10 */
    if (eli_input_float("Scale", &g_scale, 0.1f, 1.0f, "%.2f", 0))
        recompute();                                     /* fires on any edit */
}
```

---

## Gotchas

- **Empty labels get id 0.** As in Dear ImGui, a fully empty label produces a zero
  id and the field will not activate; use `"##id"` to give it a hidden identity. The
  numeric widgets do this internally.
- **In-place editing.** Text widgets mutate the caller's buffer directly (no hidden
  allocation); the buffer must persist across frames.
- **Snapshot cap.** Undo, escape-revert, and password display use a fixed scratch of
  `ELI_INPUT_TEXT_SNAPSHOT_MAX` (256) bytes; longer buffers still edit, but those
  features truncate beyond the cap.
- **Multiline selection rendering** is simplified to the caret line; the caret and
  editing are fully functional.
- **Clipboard** uses the Phase 4 API, which routes to the browser under `ELI_JSIO`
  and to an internal buffer in hosted/test builds.
