# List Clipper (Phase 26)

The list clipper renders very large, evenly-spaced item lists efficiently. Instead of
emitting all N items every frame (and paying layout/draw cost for items scrolled far
out of view), the clipper computes which item indices intersect the window's visible
region and lets you submit only those. It still advances the layout cursor past the
clipped items, so the window's content height — and therefore the scrollbar — stays
correct as if every item had been submitted.

This mirrors Dear ImGui's `ImGuiListClipper`.

Include it with:

```c
#include <eli/util/eli_list_clipper.h>
```

The header routes its libc through `eli/core/eli_platform.h`, so the same code
compiles for `wasm32` (production) and the host libc (`-DELI_TEST_HOSTED`).

> **When to use it.** Reach for the clipper when a list has hundreds or more rows of
> **known, uniform height** (text rows, table-like lists, log lines). For a handful of
> items, or items of varying height, submit them directly.

---

## The `eli_list_clipper` struct

The clipper is **caller-owned**: you declare it on the stack and drive it through the
begin/step/end API. All of its state lives in the struct, so no context storage is
needed and multiple clippers can coexist.

| Field | Meaning |
|-------|---------|
| `display_start` | First item index to render for the current step. |
| `display_end` | One past the last item index to render for the current step. |
| `items_count` | Total item count passed to `begin`. |
| `items_height` | Per-item row pitch in pixels (measured on the first step if `<= 0`). |
| `start_pos_y` | Screen-space cursor Y captured at `begin` — the top of item 0. |
| `start_seek_offset_y`, `step_no`, `ranges_count`, `ranges[]` | Internal step state. |

Only `display_start` / `display_end` are meant to be read by callers. Treat the rest
as opaque.

---

## API

### `eli_list_clipper_begin`

```c
void eli_list_clipper_begin(eli_list_clipper *clipper, int items_count, float items_height);
```

Initialize the clipper for a list of `items_count` items. Captures the current
window's layout cursor as the origin of item 0.

- `items_height` — fixed per-item **row pitch** (item height + item spacing). Pass a
  value like `eli_get_text_line_height_with_spacing()` for text rows, or
  `eli_get_frame_height_with_spacing()` for framed widgets.
- If `items_height <= 0`, the clipper submits one item on the first step to **measure**
  its height, then infers the pitch and clips the remainder.

### `eli_list_clipper_step`

```c
bool eli_list_clipper_step(eli_list_clipper *clipper);
```

Advance to the next visible range. On success sets `display_start` / `display_end` to
the item index range to render and positions the layout cursor at the first item of
that range. Returns `false` when the list is exhausted — at which point the cursor has
been seeked past **all** items so the content height is correct. Call it in a `while`
loop.

### `eli_list_clipper_end`

```c
void eli_list_clipper_end(eli_list_clipper *clipper);
```

Finish clipping. Guarantees the cursor has advanced past every item (so the scrollbar
is sized for the full list) even if the loop was exited early. Safe to call once the
loop finishes, and safe to call more than once.

### `eli_list_clipper_include_item_by_index` / `eli_list_clipper_include_items_by_index`

```c
void eli_list_clipper_include_item_by_index(eli_list_clipper *clipper, int item_index);
void eli_list_clipper_include_items_by_index(eli_list_clipper *clipper, int item_begin, int item_end);
```

Force an item index (or a half-open `[item_begin, item_end)` range) to be rendered
even if it falls **outside** the visible region — e.g. a selected row or the
keyboard-focused item that must be laid out so its state stays live. Call these after
`begin` and **before** the first `step`. Forced ranges are merged with the computed
visible range, so an offscreen index is emitted in its own step.

### `eli_list_clipper_seek_cursor_for_item`

```c
void eli_list_clipper_seek_cursor_for_item(eli_list_clipper *clipper, int item_index);
```

Position the layout cursor at the top of a specific item (`start_pos_y +
item_index * items_height`). The clipper calls this internally to skip clipped items;
you only need it if you passed an indeterminate count to `begin` and must seek the
cursor manually.

---

## Usage

```c
#include <eli/util/eli_list_clipper.h>

if (eli_begin("Big list", NULL, 0)) {
    eli_list_clipper clipper;
    eli_list_clipper_begin(&clipper, 10000, eli_get_text_line_height_with_spacing());
    while (eli_list_clipper_step(&clipper)) {
        for (int i = clipper.display_start; i < clipper.display_end; i++) {
            char label[32];
            /* render row i ... */
            eli_text(label);
        }
    }
    eli_list_clipper_end(&clipper);
}
eli_end();
```

Keeping the currently-selected row laid out even when scrolled away:

```c
eli_list_clipper_begin(&clipper, item_count, row_height);
eli_list_clipper_include_item_by_index(&clipper, selected_index);
while (eli_list_clipper_step(&clipper)) {
    for (int i = clipper.display_start; i < clipper.display_end; i++)
        render_row(i);
}
eli_list_clipper_end(&clipper);
```

---

## Gotchas

- **`items_height` is the row pitch, not just the glyph height.** It should include the
  vertical item spacing between rows, otherwise the computed content height and the
  visible range drift. The `*_with_spacing()` metric helpers give you the right value.
- **Uniform height only.** The clipper assumes every row is the same height. Variable
  heights break the index math; submit those lists directly instead.
- **Scroll takes effect at `eli_begin`.** The visible range depends on the cursor
  origin, which is computed from the window's scroll at Begin. To clip against a new
  scroll position, set it with `eli_set_next_window_scroll` before `eli_begin` (or let
  the user scroll, which applies on the following frame).
- **Content height is a two-frame settle.** On the very first frame a window has no
  measured content size, so it isn't scrollable yet. After the clipper runs once and
  seeks the cursor to the end, the measured content height feeds the next frame's
  scroll range — standard immediate-mode behavior.
- **Don't force-include after stepping.** `include_*_by_index` must be called before
  the first `step`; calls afterward are ignored.
