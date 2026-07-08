# Basic Widgets (Phase 9)

Phase 9 adds the first user-facing widgets — text, buttons, checkboxes, radio
buttons, a progress bar, and text links — plus the **shared interaction core**
(`eli_button_behavior` and the render helpers) that every later interactive widget
reuses. Each widget follows the layout contract from Phase 8: measure → `eli_item_size`
→ `eli_item_add` → interact → render.

Header: `#include <eli/widgets/eli_widgets.h>` (aggregator). Split across:

| Header | Contents |
|--------|----------|
| `eli_widget_behavior.h` | `eli_button_behavior`, `eli_button_flags`, item-size/edit helpers, render helpers |
| `eli_text_widgets.h` | text / colored / disabled / wrapped / label / bullet / separator text |
| `eli_button_widgets.h` | button, checkbox, radio, progress bar, text link |
| `eli_item_status.h` | Phase 10 item-status queries (see [item-status.md](item-status.md)) |

All widgets operate on the current window opened by `eli_begin` / closed by
`eli_end`. Outside a window scope, or when the window's items are skipped
(collapsed/clipped), they are no-ops returning `false`.

---

## The interaction core

### `bool eli_button_behavior(eli_rect bb, eli_id id, bool *out_hovered, bool *out_held, int flags)`

The primitive every clickable widget drives after `eli_item_add`. It computes hover
(the mouse over `bb`, over the top-most window, and not blocked by another active
item), sets the context **hot id** (`HoveredId`) while hovered, sets/clears the
**active id** (`ActiveId`) on press/release, and returns whether the widget was
*pressed* per its press policy.

- `out_hovered` / `out_held` — receive the hover/held state (may be `NULL`).
- `flags` — `eli_button_flags`:
  - Mouse selection: `ELI_BUTTON_MOUSE_BUTTON_LEFT` (default) / `_RIGHT` / `_MIDDLE`.
  - Press policy: `ELI_BUTTON_PRESSED_ON_CLICK` (fire on down),
    `ELI_BUTTON_PRESSED_ON_CLICK_RELEASE` (fire on release-inside — the default),
    `ELI_BUTTON_PRESSED_ON_RELEASE` (fire on any release).
- Honors `ELI_ITEM_DISABLED`: a disabled item never hovers or presses.

Hover uses the previous frame's window geometry (like Dear ImGui), so a freshly
created widget first reports hovered on the **second** frame it is submitted.

### Render helpers

- `eli_render_frame(p_min, p_max, fill, border, rounding)` — filled background plus
  optional style border.
- `eli_render_text(pos, col, text, text_end, hide_after_hash)` — clipped label draw;
  `hide_after_hash` stops the visible run at a `##` id marker.
- `eli_render_nav_highlight(bb, id)` — focus rectangle (a no-op until keyboard nav).
- `eli_render_arrow` / `eli_render_check_mark` / `eli_render_bullet` — glyph shapes.
- `eli_calc_item_size(size, default_w, default_h)` — resolve a size argument
  (`0` = default, `< 0` = measured back from the content-region edge).
- `eli_mark_item_edited(id)` — flag the last item as edited this frame (drives
  `eli_is_item_edited` / `eli_is_item_deactivated_after_edit`).

---

## Text

| Function | Purpose |
|----------|---------|
| `eli_text_unformatted(text, text_end)` | Draw a raw UTF-8 run (`text_end` NULL = strlen). |
| `eli_text(fmt, ...)` / `eli_text_v` | printf-style text. |
| `eli_text_colored(col, fmt, ...)` / `_v` | Text in an explicit `eli_vec4` color. |
| `eli_text_disabled(fmt, ...)` / `_v` | Text in the disabled-text color. |
| `eli_text_wrapped(fmt, ...)` / `_v` | Greedy word-wrap to the content-region width. |
| `eli_label_text(label, fmt, ...)` / `_v` | A value with a right-hand label. |
| `eli_bullet_text(fmt, ...)` / `_v` | A bullet glyph followed by text. |
| `eli_bullet(void)` | A standalone bullet, advancing one line. |
| `eli_separator_text(label)` | A horizontal rule carrying a text label. |

Formatted variants route through `vsnprintf` into a 1 KB stack buffer — no per-frame
heap allocation. A text item is non-interactive (`id == 0`) but still records a
last-item rect for the status queries, and advances the cursor by its measured size.

---

## Buttons

| Function | Purpose |
|----------|---------|
| `bool eli_button(label)` | Default button sized to fit its label. |
| `bool eli_button_ex(label, size, flags)` | Button with an explicit size / button flags. |
| `bool eli_small_button(label)` | Compact button (no vertical frame padding). |
| `bool eli_invisible_button(str_id, size, flags)` | Sized clickable region, no visuals. |
| `bool eli_arrow_button(str_id, dir)` | Square button drawing an `eli_dir` arrow. |

Each returns `true` on the frame it is pressed (release-inside by default). Frame
color comes from `ELI_COL_BUTTON` / `_HOVERED` / `_ACTIVE`.

## Checkboxes & radio buttons

| Function | Purpose |
|----------|---------|
| `bool eli_checkbox(label, bool *v)` | Toggles `*v`; returns `true` the toggle frame. |
| `bool eli_checkbox_flags_int(label, int *flags, value)` | Sets/clears a bit mask. |
| `bool eli_checkbox_flags_uint(label, unsigned *flags, value)` | Unsigned variant. |
| `bool eli_radio_button(label, bool active)` | Radio showing an on/off state. |
| `bool eli_radio_button_int(label, int *v, v_button)` | Radio bound to an int selection. |

When a checkbox/radio changes value it calls `eli_mark_item_edited`, so
`eli_is_item_edited` is `true` that frame. Colors: `ELI_COL_FRAME_BG*` for the box,
`ELI_COL_CHECK_MARK` for the tick/dot.

## Progress bar & links

- `void eli_progress_bar(float fraction, eli_vec2 size_arg, const char *overlay)` —
  draws a framed bar filled to `fraction` (clamped to `[0,1]`); `overlay` NULL shows a
  `"NN%"` label. Width `0` = current item width, height `0` = one frame height.
- `bool eli_text_link(label)` — a clickable, underlined hyperlink; returns `true` when
  clicked.
- `void eli_text_link_open_url(label, url)` — a link that opens `url` in the browser on
  click (browser-only; the open is a no-op in hosted/test builds).

---

## Example

```c
#include <eli/widgets/eli_widgets.h>

static bool show_details = false;
static int  quality = 1;

void draw_panel(void)
{
    eli_begin("Settings", NULL, 0);

    eli_text("Rendering options");
    eli_separator_text("Quality");

    if (eli_button("Apply"))
        apply_settings();
    eli_checkbox("Show details", &show_details);
    eli_radio_button_int("Low",  &quality, 0);
    eli_radio_button_int("High", &quality, 1);

    eli_progress_bar(0.42f, eli_make_vec2(0, 0), NULL);
    if (eli_text_link("Documentation"))
        open_docs();

    eli_end();
}
```

## Gotchas

- Hover and press depend on the **previous** frame's window geometry, so a brand-new
  widget only becomes hoverable on its second submitted frame.
- `eli_new_frame` rolls the per-frame interaction edges (hot id reset, active/hot
  previous-frame snapshots). Call the standard frame sequence
  (`eli_new_frame` → `eli_input_update_begin_frame` → `eli_window_new_frame` → widgets
  → `eli_window_render` → `eli_render`).
- Formatted text truncates to 1 KB (`ELI_TEXT_FMT_BUFFER_SIZE`).
- Widget ids derive from the label via `eli_get_id`; use `"Label##uniquekey"` to give
  two same-looking widgets distinct ids.
