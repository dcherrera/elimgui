# Combo & Selectable (Phase 14)

Selection widgets for elimgui, mirroring Dear ImGui's `Selectable`, `BeginCombo`
/ `Combo`, and `BeginListBox` / `ListBox`. All state is module-private and lives in
three self-contained headers under `include/eli/widgets/`:

| Header | Provides |
|--------|----------|
| `eli_selectable.h` | `eli_selectable`, `eli_selectable_bool` (the shared clickable row) |
| `eli_combo.h` | `eli_begin_combo` / `eli_end_combo`, `eli_combo`, `eli_combo_str`, `eli_combo_fn` |
| `eli_listbox.h` | `eli_begin_list_box` / `eli_end_list_box`, `eli_list_box`, `eli_list_box_fn` |

```c
#include <eli/widgets/eli_combo.h> /* pulls in selectable + list box too */
```

`eli_combo.h` aggregates the selectable and list-box headers, so including it makes
the whole phase available. The combo dropdown **is a popup** — it reuses the Phase 17
popup stack and window path (`eli_open_popup_id` / `eli_popup__begin` /
`eli_end_popup`), anchored just below the preview frame. That means a host driving
combos must call the popup frame hooks (`eli_popup_new_frame()` after
`eli_window_new_frame()`, and `eli_popup_end_frame()` before `eli_window_render()`).

## Selectable

A selectable is a full-width, tightly-packed clickable row that highlights on hover
or when selected (using the `ELI_COL_HEADER*` colors). It is the row primitive that
combos, list boxes, and menus build on.

```c
typedef int eli_selectable_flags;
enum eli_selectable_flags_ {
    ELI_SELECTABLE_NONE                 = 0,
    ELI_SELECTABLE_NO_AUTO_CLOSE_POPUPS = 1 << 0, /* don't close the enclosing popup on press */
    ELI_SELECTABLE_SPAN_ALL_COLUMNS     = 1 << 1, /* stretch across the full work-area width */
    ELI_SELECTABLE_ALLOW_DOUBLE_CLICK   = 1 << 2, /* (maps to click-release; no dbl-click yet) */
    ELI_SELECTABLE_DISABLED             = 1 << 3, /* greyed out, non-interactive */
    ELI_SELECTABLE_ALLOW_OVERLAP        = 1 << 4,
    ELI_SELECTABLE_HIGHLIGHT            = 1 << 5  /* draw the hovered highlight unconditionally */
};
```

The flags enum is defined in `eli_selectable.h` under an `#ifndef ELI_SELECTABLE_NONE`
guard so a later phase may promote it into `core/eli_enums.h` without a clash.

### `eli_selectable`

```c
bool eli_selectable(const char *label, bool selected,
                    eli_selectable_flags flags, eli_vec2 size);
```

Submit a row whose selected state the caller owns. Draws the header highlight when
`selected` (or hovered/`ELI_SELECTABLE_HIGHLIGHT`), renders the label (its visible
part stops at `##`), and returns `true` on the frame the row is pressed
(press-on-click-release). When the row lives inside a popup window, a press closes
that popup unless `ELI_SELECTABLE_NO_AUTO_CLOSE_POPUPS` is set.

- **label** — row text and identity (`##` hides trailing id text).
- **selected** — whether this row currently shows as selected.
- **flags** — `eli_selectable_flags`.
- **size** — a `0` axis fits the label (x) / fills the work-area width (x) and uses
  one text line (y); pass an explicit size to override.
- **returns** — `true` the frame the row is pressed.

### `eli_selectable_bool`

```c
bool eli_selectable_bool(const char *label, bool *p_selected,
                         eli_selectable_flags flags, eli_vec2 size);
```

Same as above but bound to a `bool`: a press toggles `*p_selected` and returns `true`.

```c
static bool show_grid = false;
if (eli_selectable_bool("Show grid", &show_grid, ELI_SELECTABLE_NONE, eli_make_vec2(0, 0)))
    rebuild_view();
```

## Combo

A combo is a preview button that opens a dropdown popup list. Clicking the preview
toggles the dropdown; each row is an `eli_selectable`, so choosing one auto-closes
the popup.

```c
typedef int eli_combo_flags;
enum eli_combo_flags_ {
    ELI_COMBO_NONE              = 0,
    ELI_COMBO_POPUP_ALIGN_LEFT  = 1 << 0,
    ELI_COMBO_HEIGHT_SMALL      = 1 << 1, /* ~4 item rows before scrolling */
    ELI_COMBO_HEIGHT_REGULAR    = 1 << 2, /* ~8 rows (default) */
    ELI_COMBO_HEIGHT_LARGE      = 1 << 3, /* ~20 rows */
    ELI_COMBO_HEIGHT_LARGEST    = 1 << 4, /* ~30 rows */
    ELI_COMBO_NO_ARROW_BUTTON   = 1 << 5, /* hide the down-arrow button */
    ELI_COMBO_NO_PREVIEW        = 1 << 6, /* arrow only, no preview text */
    ELI_COMBO_WIDTH_FIT_PREVIEW = 1 << 7  /* size the frame to the preview text */
};
```

### `eli_begin_combo` / `eli_end_combo`

```c
bool eli_begin_combo(const char *label, const char *preview_value, eli_combo_flags flags);
void eli_end_combo(void);
```

Draw the preview frame (showing `preview_value`) and, if the dropdown is open, begin
its popup. Call `eli_end_combo` **only when `eli_begin_combo` returns `true`**, after
emitting the item rows yourself:

```c
static const char *items[] = { "Red", "Green", "Blue" };
static int current = 0;
if (eli_begin_combo("Color", items[current], ELI_COMBO_NONE)) {
    for (int i = 0; i < 3; i++) {
        bool selected = (i == current);
        if (eli_selectable(items[i], selected, ELI_SELECTABLE_NONE, eli_make_vec2(0, 0)))
            current = i;
    }
    eli_end_combo();
}
```

### `eli_combo`, `eli_combo_str`, `eli_combo_fn`

Self-contained combos that render the list for you and write the chosen index back:

```c
bool eli_combo(const char *label, int *current_item,
               const char *const items[], int items_count, int popup_max_height_in_items);
bool eli_combo_str(const char *label, int *current_item,
                   const char *items_separated_by_zeros, int popup_max_height_in_items);
bool eli_combo_fn(const char *label, int *current_item,
                  const char *(*getter)(void *user_data, int idx), void *user_data,
                  int items_count, int popup_max_height_in_items);
```

- **current_item** — in/out selected index.
- **items / items_separated_by_zeros / getter** — the item source. `_str` takes one
  `"a\0b\0c\0"` NUL-separated string; `_fn` takes a getter callback + `user_data`.
- **popup_max_height_in_items** — visible rows before the popup scrolls (`-1` = the
  flag-derived default; feeds a next-window size constraint on the popup).
- **returns** — `true` on the frame the selection changes.

```c
static int mode = 0;
eli_combo("Mode", &mode, (const char *const[]){ "Off", "Auto", "On" }, 3, -1);
```

## List box

A list box is a framed, scrollable child region filled with selectable rows.

### `eli_begin_list_box` / `eli_end_list_box`

```c
bool eli_begin_list_box(const char *label, eli_vec2 size);
void eli_end_list_box(void);
```

Open the framed child; fill it with `eli_selectable` rows; close with
`eli_end_list_box` **only when `eli_begin_list_box` returns `true`**. A `0` size axis
defaults to the item width (x) and ~7.25 text lines tall (y). A trailing label is
drawn to the right of the frame.

```c
if (eli_begin_list_box("Files", eli_make_vec2(0, 0))) {
    for (int i = 0; i < n; i++)
        if (eli_selectable(names[i], i == sel, ELI_SELECTABLE_NONE, eli_make_vec2(0, 0)))
            sel = i;
    eli_end_list_box();
}
```

### `eli_list_box`, `eli_list_box_fn`

```c
bool eli_list_box(const char *label, int *current_item,
                  const char *const items[], int items_count, int height_in_items);
bool eli_list_box_fn(const char *label, int *current_item,
                     const char *(*getter)(void *user_data, int idx), void *user_data,
                     int items_count, int height_in_items);
```

Self-contained list boxes that render one selectable per item and write the chosen
index back through `current_item`, returning `true` when it changes.
`height_in_items < 0` uses `min(items_count, 7)` visible rows.

## Gotchas

- **`eli_end_combo` / `eli_end_list_box` are only balanced when the matching `begin`
  returned `true`** — the same contract as `eli_begin`/`eli_begin_child`.
- **Combos need the popup frame hooks.** Without `eli_popup_new_frame()` /
  `eli_popup_end_frame()` the dropdown will never open or close.
- **Distinct row ids.** When items may share visible text (e.g. inside the convenience
  `eli_combo`/`eli_list_box`), push a per-index id (`eli_push_id_int(i)`) around each
  selectable so their ids don't collide. The convenience wrappers already do this.
- **Colors must be initialized.** The header highlight uses `ELI_COL_HEADER*`; call
  `eli_style_colors_dark(NULL)` (or another theme) once, or those colors are
  transparent and nothing draws.
