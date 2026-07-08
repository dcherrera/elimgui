# Item Status Queries (Phase 10)

After a widget is submitted, the item-status queries report what happened to it this
frame — whether it is hovered, active, edited, or crossed an activation edge — and
expose the last item's id and geometry. These are the foundation for **composing**
widgets (tooltips on hover, reacting to edits, custom drag handles).

Every query describes the **most-recently-submitted item**, so call it immediately
after the widget it refers to (and before the next widget). They read the last-item
record written by `eli_item_add` plus the interaction edges maintained by
`eli_button_behavior` on the context.

Header: pulled in by `#include <eli/widgets/eli_widgets.h>`; defined in
`eli_widgets/eli_item_status.h`. The rect accessors (`eli_get_item_id`,
`eli_get_item_rect_min/_max/_size`) come from the layout item core and are
re-exported here so `<eli/widgets/...>` is a single entry point.

---

## Per-item queries

| Function | Returns `true` when… |
|----------|----------------------|
| `eli_is_item_hovered(flags)` | the item's rect is hovered, the mouse is over the top-most window, and no other item is active. `flags` are `eli_hovered_flags` that relax the gating (`ALLOW_WHEN_BLOCKED_BY_ACTIVE_ITEM`, `ALLOW_WHEN_DISABLED`, …). |
| `eli_is_item_active()` | the item is the active item (e.g. a held button). |
| `eli_is_item_focused()` | the item is the focused (nav) item (nav lands in a later phase; currently always `false`). |
| `eli_is_item_clicked(mouse_button)` | the item is hovered and `mouse_button` was clicked this frame. |
| `eli_is_item_visible()` | the item's rect overlapped the clip rect. |
| `eli_is_item_edited()` | the item's value changed this frame (set via `eli_mark_item_edited`). |
| `eli_is_item_activated()` | the item became active this frame (press edge). |
| `eli_is_item_deactivated()` | the item stopped being active this frame (release edge). |
| `eli_is_item_deactivated_after_edit()` | the item deactivated this frame **and** was edited during its active spell. |
| `eli_is_item_toggled_open()` | the item (e.g. a tree node) was toggled open this frame. |

## Any-item aggregates

| Function | Returns `true` when… |
|----------|----------------------|
| `eli_is_any_item_hovered()` | any item is hovered (`hot_id != 0`). |
| `eli_is_any_item_active()` | any item is active (`active_id != 0`). |
| `eli_is_any_item_focused()` | any item is focused (`nav_id != 0`; nav-dependent). |

## Last-item accessors

| Function | Returns |
|----------|---------|
| `eli_id eli_get_item_id()` | the id of the most-recent item (`0` for non-interactive items). |
| `eli_vec2 eli_get_item_rect_min()` | the item's top-left corner (screen space). |
| `eli_vec2 eli_get_item_rect_max()` | the item's bottom-right corner. |
| `eli_vec2 eli_get_item_rect_size()` | the item's size. |

---

## Activation edges across frames

For a mouse-driven button held for a few frames, the edges fire like this:

| Frame | mouse | `is_item_activated` | `is_item_active` | `is_item_deactivated` |
|-------|-------|---------------------|------------------|-----------------------|
| press   | down    | `true`  | `true`  | `false` |
| hold    | (held)  | `false` | `true`  | `false` |
| release | up      | `false` | `false` | `true`  |

These rely on `eli_new_frame` rolling `active_id` into `active_id_previous_frame`
each frame — so drive the standard per-frame sequence
(`eli_new_frame` → `eli_input_update_begin_frame` → `eli_window_new_frame` → widgets →
`eli_window_render` → `eli_render`).

---

## Example

```c
#include <eli/widgets/eli_widgets.h>

void draw(void)
{
    eli_begin("W", NULL, 0);

    eli_button("Hover me");
    if (eli_is_item_hovered(ELI_HOVERED_NONE))
        show_tooltip("A button");

    static float value = 0.0f;
    eli_progress_bar(value, eli_make_vec2(0, 0), NULL);

    static bool on = false;
    if (eli_checkbox("Enabled", &on) && eli_is_item_edited())
        persist_setting(on);          /* only on the frame it actually changed */

    /* Gate work on any item currently being dragged/held. */
    if (eli_is_any_item_active())
        pause_background_animation();

    eli_end();
}
```

## Gotchas

- A query always describes the **last** item; calling it after a different widget
  reports that widget instead.
- `eli_is_item_hovered` uses the previous frame's window geometry (Dear ImGui parity),
  so a freshly created widget reports hovered a frame late.
- `eli_is_item_edited` is only `true` for widgets that call `eli_mark_item_edited`
  (checkbox, radio, and later value widgets) on the frame their value changes.
- Non-interactive items (plain text) have `eli_get_item_id() == 0` and are never
  active or focused, but are still visible and carry a rect.
