# Layout System (Phase 8)

The layout system positions and sizes items inside a window. It owns the window's
**layout cursor** and the shared **item-layout core** (`eli_item_size` /
`eli_item_add`) that every widget in Phases 9–24 builds on. Nothing here draws a
widget by itself — it advances the cursor, registers item geometry, and answers
questions about the current line, the content region, and item widths.

All layout calls operate on the current window opened by `eli_begin` and closed by
`eli_end`. Outside a window scope they are no-ops (returning zeroed values).

Header: `#include <eli/layout/eli_layout.h>` (aggregator). Split across:

| Header | Contents |
|--------|----------|
| `eli_layout_item.h` | Item-layout core, last-item accessors, content-region queries |
| `eli_layout_cursor.h` | Cursor get/set (window-local and screen-space) |
| `eli_layout_helpers.h` | Separator, same-line, spacing, dummy, indent, groups |
| `eli_layout_stack.h` | Item-width stack, text-wrap-position stack |
| `eli_layout_sizing.h` | Text-line and frame height helpers |

---

## Coordinate model

- **Screen space** — absolute pixels. The window's layout cursor
  (`eli_get_cursor_screen_pos`) lives here.
- **Window-local** — relative to the window origin, *including* scroll:
  `local = screen − window.pos + window.scroll`. `eli_get_cursor_pos`,
  `eli_set_cursor_pos`, and the content-region-min/max queries use this frame.

Each `eli_begin` seeds the cursor at the work-area origin and resets the per-line
layout state (line sizes, indent, group offset, baseline offsets).

---

## Item-layout core

Every widget follows the same two-step contract:

```c
eli_vec2 size = /* measured widget size */;
eli_item_size(size, -1.0f);                 // advance the cursor + line height
eli_rect bb = eli_make_rect(x, y, size.x, size.y);
if (eli_item_add(id, bb, 0)) {
    // item is visible/interactable — draw + handle input here
}
```

### `void eli_item_size(eli_vec2 size, float text_baseline_y)`
Advances the layout cursor to account for an item of `size`, then moves to the
start of the next line, adding `style.item_spacing.y`. Updates the current/previous
line heights and the window content-max extent (which drives the scroll range).
`text_baseline_y` is the item's text baseline offset from its top, or `< 0` when the
item has no baseline to align. `eli_item_size_rect(bb, baseline)` is the same by
bounding box.

The cursor advances vertically by `size.y + item_spacing.y` on a fresh line; on a
`same_line` continuation the line grows to the tallest item.

### `bool eli_item_add(eli_id id, eli_rect bb, int extra_flags)`
Registers the item: records its `id`, `bb`, and status flags on the context
(readable via the last-item accessors), runs the clip-rect visibility test, sets the
hovered-rect status bit, and consumes any one-shot next-item width. Returns `true`
when the item rect overlaps the window clip rect. `extra_flags` is reserved.

### Last-item accessors (consumed by later item-query phases)
`eli_get_item_id`, `eli_get_item_rect`, `eli_get_item_rect_min`,
`eli_get_item_rect_max`, `eli_get_item_rect_size`, `eli_get_item_status_flags`.

Status flags (`eli_item_status_flags`): `ELI_ITEM_STATUS_HOVERED_RECT`,
`ELI_ITEM_STATUS_VISIBLE`, plus edited/toggled/deactivated bits reserved for the
interaction phase.

---

## Cursor

| Function | Returns / effect |
|----------|------------------|
| `eli_get_cursor_pos()` / `_x` / `_y` | Cursor in window-local coords |
| `eli_set_cursor_pos(local)` / `_x` / `_y` | Move cursor (window-local); grows content-max |
| `eli_get_cursor_start_pos()` | Window body origin (window-local) |
| `eli_get_cursor_screen_pos()` | Cursor in absolute screen coords |
| `eli_set_cursor_screen_pos(pos)` | Move cursor to a screen position; grows content-max |

---

## Layout helpers

- **`eli_separator()`** — full-width 1px horizontal rule at the cursor; advances by
  the thickness plus item spacing.
- **`eli_same_line(offset_from_start_x, spacing)`** — place the next item on the
  current line. `offset_from_start_x != 0` positions it at an absolute window x
  (from the window's left content origin); `== 0` follows the previous item.
  `spacing < 0` uses `item_spacing.x` for the follow case, `0` otherwise.
- **`eli_new_line()`** — end the current line, advancing by the line height (or one
  text line if empty).
- **`eli_spacing()`** — advance by one empty item (a row of `item_spacing.y`).
- **`eli_dummy(size)`** — add an empty, non-interactive item of `size`.
- **`eli_indent(w)` / `eli_unindent(w)`** — shift the left edge of following lines
  by `w` pixels (`0` uses `style.indent_spacing`).
- **`eli_align_text_to_frame_padding()`** — raise the current line so following text
  aligns with a framed widget's text baseline.

### Groups

```c
eli_begin_group();
eli_dummy(eli_make_vec2(30, 30));
eli_dummy(eli_make_vec2(50, 10));
eli_end_group();
// The group is now the "last item": rect = (start .. enclosed extent),
// size = (50, 44)  ->  widest item = 50, height = 30 + spacing(4) + 10 = 44.
eli_vec2 group_size = eli_get_item_rect_size();
```

`eli_begin_group` snapshots the layout cursor/indent and starts measuring from the
current position; `eli_end_group` restores the pre-group state, then emits the
enclosing bounding box as a single item (advancing the cursor below it). This lets
you treat a cluster of widgets as one item for layout and hit-testing.

---

## Item width

Governs the pixel width of following framed widgets.

- **`eli_push_item_width(w)` / `eli_pop_item_width()`** — `w > 0` verbatim, `w == 0`
  selects the window default (`trunc(window.size.x * 0.65)`), `w < 0` measures back
  from the work-area right edge.
- **`eli_set_next_item_width(w)`** — one-shot override for the next widget only;
  cleared by the next `eli_item_add`.
- **`eli_calc_item_width()`** — resolves the effective width (next-item ▸ pushed ▸
  default), applying the negative-from-right rule. Returns `>= 1`.

## Text wrapping

- **`eli_push_text_wrap_pos(local_x)` / `eli_pop_text_wrap_pos()`** — window-local x
  at which text widgets wrap. `0` wraps at the work-area right edge; `< 0` disables
  wrapping. The stack stores the positions; text widgets read the top in a later
  phase.

## Sizing helpers

| Function | Value |
|----------|-------|
| `eli_get_text_line_height()` | `font_size` |
| `eli_get_text_line_height_with_spacing()` | `font_size + item_spacing.y` |
| `eli_get_frame_height()` | `font_size + frame_padding.y * 2` |
| `eli_get_frame_height_with_spacing()` | `font_size + frame_padding.y * 2 + item_spacing.y` |

---

## Content region

| Function | Returns |
|----------|---------|
| `eli_get_content_region_avail()` | Space from the cursor to the work-area edge (screen units); shrinks as the cursor advances |
| `eli_get_content_region_max()` | Work-area bottom-right, window-local |
| `eli_get_window_content_region_min()` | Work-area top-left, window-local |
| `eli_get_window_content_region_max()` | Work-area bottom-right, window-local |

---

## Example

```c
if (eli_begin("Demo", NULL, 0)) {
    eli_dummy(eli_make_vec2(80, 20));       // reserve space
    eli_same_line(0, -1);                   // next item to the right
    eli_dummy(eli_make_vec2(80, 20));

    eli_separator();

    eli_indent(0);
    eli_begin_group();
    eli_dummy(eli_make_vec2(120, 16));
    eli_dummy(eli_make_vec2(90, 16));
    eli_end_group();                        // cluster measured as one item
    eli_unindent(0);

    eli_vec2 avail = eli_get_content_region_avail();  // remaining space
    (void)avail;
    eli_end();
}
```

---

## Gotchas

- `eli_item_size` must precede `eli_item_add` — the widget size feeds the cursor,
  then the bounding box (at the *pre-advance* cursor) is registered.
- `same_line`'s `offset_from_start_x` is measured from the window's left content
  origin (`pos − scroll`), not from the indented cursor.
- Manual `eli_set_cursor_*` still grows the content-max extent, so hand-placed items
  correctly expand the scroll range.
- When `window.skip_items` is set (collapsed/clipped window), the advancement
  helpers early-out; `eli_item_add` returns `false`.
