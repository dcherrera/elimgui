# Popups & Modals (Phase 17)

Popups are transient, auto-sized, chrome-less windows that float above everything
else and close when the user clicks away or presses Escape. Modals are popups that
additionally dim and block the windows behind them until they are explicitly
dismissed. Both are built on the window system: a popup **is** a window
(`ELI_WINDOW_POPUP` composed with the no-title / no-resize / auto-resize flags) and
reuses `eli_begin_ex` / `eli_end` for layout, focus, and z-order.

Header: `include/eli/widgets/eli_popup.h` (usable via direct include).

```c
#include <eli/widgets/eli_popup.h>
```

## Model

The module owns two stacks (module-private, file-static):

- **Open-popup stack** — every popup that has been opened and not yet closed, from
  outer to inner. Each entry records the popup id, the owning window, the frame it
  was opened, and the mouse position on open.
- **Begin-popup stack** — the popups actually being emitted this frame. Pushed by
  `eli_begin_popup*` and popped by `eli_end_popup`. Its depth is the current
  "nesting level" used to decide which open popup a query/begin refers to.

An id is considered *open at the current level* when the open stack is deeper than
the begin stack and the entry at the begin-stack depth matches the id — this is why
`eli_is_popup_open("x")` reports `false` from *inside* that popup's own
begin/end scope (there it queries the next, nested level), matching Dear ImGui.

## Frame integration

Two per-frame hooks must be composed into the frame flow by the orchestrator:

```c
eli_new_frame();
eli_input_update_begin_frame();
eli_window_new_frame();      // resolves the hovered window
eli_popup_new_frame();       // <-- reset begin stack; reap/close popups
    ... eli_begin(...) / widgets / eli_begin_popup(...) ...
eli_popup_end_frame();       // <-- drop never-begun stale popups
eli_window_render();
eli_render();
eli_input_update_end_frame();
```

- **`eli_popup_new_frame(void)`** — run *after* `eli_window_new_frame` (so the
  hovered window is known). It resets the begin-popup stack, closes popups whose
  owning window vanished, closes the top popup on Escape, and closes popups the
  user clicked away from (`ClosePopupsOverWindow` semantics). Modal popups are
  never closed by a click-outside; just-opened popups (window not yet created) are
  always kept.
- **`eli_popup_end_frame(void)`** — run after all widgets, before
  `eli_window_render`. It trims popups that were opened on a previous frame but
  never begun, so the open stack cannot outgrow the emitted nesting.

## API

### Opening / closing / querying

| Function | Purpose |
|----------|---------|
| `void eli_open_popup(const char *str_id, eli_popup_flags flags)` | Open a popup by string id (hashed against the current id seed). |
| `void eli_open_popup_id(eli_id id, eli_popup_flags flags)` | Open a popup by explicit id. Honors `NoReopen` / `NoOpenOverExistingPopup`. |
| `void eli_open_popup_on_item_click(const char *str_id, eli_popup_flags flags)` | Open when the last item is clicked with the flag's mouse button (release edge). `NULL` id keys off the last item's id. |
| `void eli_close_current_popup(void)` | Close the innermost popup being emitted (and any nested inside it). |
| `bool eli_is_popup_open(const char *str_id, eli_popup_flags flags)` | Is the popup open at the current level (or any level/any id per flags)? |

### Popup windows

| Function | Purpose |
|----------|---------|
| `bool eli_begin_popup(const char *str_id, eli_window_flags flags)` | Begin the popup window if open; balance with `eli_end_popup` only when it returns `true`. |
| `void eli_end_popup(void)` | End the innermost popup window. |
| `bool eli_begin_popup_context_item(const char *str_id, eli_popup_flags flags)` | Open (right-click of the previous item, by default) + begin a context menu for that item. `NULL` id uses the last item's id. |
| `bool eli_begin_popup_context_window(const char *str_id, eli_popup_flags flags)` | Open (click over the current window) + begin a window context menu. |
| `bool eli_begin_popup_context_void(const char *str_id, eli_popup_flags flags)` | Open (click over empty space) + begin a void context menu. |

### Modals

| Function | Purpose |
|----------|---------|
| `bool eli_begin_popup_modal(const char *name, bool *p_open, eli_window_flags flags)` | Begin a modal dialog if open. Keeps a title bar; renders a full-display dim backdrop behind it; traps hover/focus. `p_open` (optional) drives a title-bar close button. |

`flags` on the begin/context functions is an `eli_window_flags` mask merged into
the popup window (the module already adds the no-title / auto-resize / popup flags).
`eli_popup_flags` controls opening: the low bits select the mouse button
(`ELI_POPUP_MOUSE_BUTTON_LEFT` / `_RIGHT` / `_MIDDLE`), plus `ELI_POPUP_NO_REOPEN`,
`ELI_POPUP_NO_OPEN_OVER_EXISTING_POPUP`, `ELI_POPUP_NO_OPEN_OVER_ITEMS`, and the
`ELI_POPUP_ANY_POPUP*` query gates.

## Usage

Simple popup opened by a button:

```c
if (eli_button("Options"))
    eli_open_popup("options_popup", ELI_POPUP_NONE);

if (eli_begin_popup("options_popup", 0)) {
    eli_text("Quality");
    if (eli_button("Close"))
        eli_close_current_popup();
    eli_end_popup();
}
```

Right-click context menu on an item:

```c
eli_button("Right-click me");
if (eli_begin_popup_context_item("item_ctx", ELI_POPUP_MOUSE_BUTTON_RIGHT)) {
    eli_text("Copy");
    eli_text("Paste");
    eli_end_popup();
}
```

Modal dialog:

```c
if (eli_button("Delete"))
    eli_open_popup("Confirm delete", ELI_POPUP_NONE);

bool open = true;
if (eli_begin_popup_modal("Confirm delete", &open, 0)) {
    eli_text("Are you sure?");
    if (eli_button("Yes"))
        eli_close_current_popup();
    eli_end_popup();
}
```

## Gotchas

- Only call `eli_end_popup` when the matching `eli_begin_popup*` returned `true`
  (identical to the `eli_begin`/`eli_end` contract for regular windows).
- `eli_is_popup_open("id")` returns `false` while you are *inside* that popup's
  begin/end scope; query it *before* calling begin, or use `ELI_POPUP_ANY_POPUP`.
- A modal is not closed by clicking outside or by its own backdrop; dismiss it with
  `eli_close_current_popup` or by clearing its `p_open` flag. Escape closes the
  top-most popup (modals included), matching Dear ImGui.
- `eli_popup_new_frame` must run after `eli_window_new_frame`, and
  `eli_popup_end_frame` before `eli_window_render`.
- The modal dim uses `ELI_COL_MODAL_WINDOW_DIM_BG`; with an un-themed context that
  color is transparent, so the backdrop is invisible but focus trapping still works.
