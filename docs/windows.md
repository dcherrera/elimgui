# Windows

Phase 7 adds elimgui's window system: the container every widget lives in. A
window owns its geometry (position, size, collapse), its scroll state, and its
own draw list, and is addressed immediate-mode style by a string name that also
serves as its visible title.

Header: `#include <eli/window/eli_window.h>` (pulls in the whole window category —
settings, scrolling, interaction, queries, and child windows).

## Contents

- [Frame flow](#frame-flow)
- [Lifecycle: begin / end](#lifecycle-begin--end)
- [Window flags](#window-flags)
- [Next-window settings](#next-window-settings)
- [State queries](#state-queries)
- [Child windows](#child-windows)
- [Scrolling](#scrolling)
- [Interaction](#interaction)
- [Compiling example](#compiling-example)

## Frame flow

Window per-frame work is exposed as public functions the application calls each
frame, mirroring the input phase. Core's `eli_new_frame`/`eli_render` are
unchanged; the window functions slot around them:

```c
eli_new_frame();                 /* core: advance frame counter + time */
eli_input_update_begin_frame();  /* input: drain events, derive state   */
eli_window_new_frame();          /* windows: hover/move/wheel, reset stack */

/* ... eli_begin(...) / widgets / eli_end() for each window ... */

eli_window_render();             /* windows: assemble draw data on ctx->draw_data */
eli_render();                    /* core: close the frame scope */
eli_input_update_end_frame();    /* input: roll per-frame accumulators */
```

- **`eli_window_new_frame(void)`** — resets the current-window stack, continues or
  finishes a title-bar drag, resolves the hovered window from the previous
  frame's geometry, and applies mouse-wheel scrolling to it.
- **`eli_window_render(void)`** — gathers every active window's draw list
  (back-to-front by focus order, each root followed by its child windows), builds
  an `eli_draw_data`, and stores it on the context so `eli_get_draw_data()`
  returns real geometry.

## Lifecycle: begin / end

```c
bool eli_begin(const char *name, bool *p_open, eli_window_flags flags);
void eli_end(void);
```

`eli_begin` retrieves the window named `name` (creating it on first use — the
same name maps to the same window and id every frame), applies any pending
next-window settings, lays out its decorations and work area, renders the
background/border/title bar, and makes it the current window. It returns `true`
when the window body should be emitted and `false` when it is collapsed or fully
clipped. **`eli_end` must be called regardless of the return value** — exactly
once per `eli_begin`:

```c
if (eli_begin("Settings", &open, 0)) {
    /* emit widgets here */
}
eli_end();
```

Passing a non-NULL `p_open` draws a close button in the title bar and clears
`*p_open` when it is clicked.

The window's id is the CRC-32 hash of `name` (honoring `##`/`###` label rules
from the ID system), so `"Cfg##1"` and `"Cfg##2"` are distinct windows that show
the same title `Cfg`.

## Window flags

`eli_window_flags` (defined in the core enums) selects behavior at `eli_begin`:

| Flag | Effect |
|------|--------|
| `ELI_WINDOW_NO_TITLEBAR` | No title bar (and no title-bar height). |
| `ELI_WINDOW_NO_RESIZE` | Disable the bottom-right resize grip. |
| `ELI_WINDOW_NO_MOVE` | Disable title-bar dragging. |
| `ELI_WINDOW_NO_SCROLLBAR` | Never show scrollbars. |
| `ELI_WINDOW_NO_SCROLL_WITH_MOUSE` | Ignore the mouse wheel. |
| `ELI_WINDOW_NO_COLLAPSE` | Disable the double-click collapse toggle. |
| `ELI_WINDOW_AUTO_RESIZE` | Size the window to its content each frame. |
| `ELI_WINDOW_NO_BACKGROUND` | Skip background + border fill. |
| `ELI_WINDOW_HORIZONTAL_SCROLLBAR` | Allow a horizontal scrollbar. |
| `ELI_WINDOW_ALWAYS_VERTICAL_SCROLLBAR` | Always reserve the vertical scrollbar. |
| `ELI_WINDOW_NO_FOCUS_ON_APPEARING` | Don't grab focus when first shown. |
| `ELI_WINDOW_NO_MOUSE_INPUTS` | Window is not hit-tested for hover. |

## Next-window settings

Call these *before* `eli_begin`; they apply to the next window opened and then
clear. Each takes an `eli_cond` where `0` (or `ELI_COND_ALWAYS`) means "every
frame", and `ELI_COND_ONCE`/`FIRST_USE_EVER`/`APPEARING` gate on newness.

```c
void eli_set_next_window_pos(eli_vec2 pos, eli_cond cond, eli_vec2 pivot);
void eli_set_next_window_size(eli_vec2 size, eli_cond cond);
void eli_set_next_window_size_constraints(eli_vec2 size_min, eli_vec2 size_max);
void eli_set_next_window_content_size(eli_vec2 size);
void eli_set_next_window_collapsed(bool collapsed, eli_cond cond);
void eli_set_next_window_focus(void);
void eli_set_next_window_scroll(eli_vec2 scroll);   /* per axis; <0 = unchanged */
void eli_set_next_window_bg_alpha(float alpha);
```

`pivot` in `set_next_window_pos` positions the window relative to a point inside
it: `(0,0)` = top-left, `(0.5,0.5)` = centered on `pos`, `(1,1)` = bottom-right.

The current window can also be mutated directly:

```c
void eli_set_window_pos(eli_vec2 pos, eli_cond cond);
void eli_set_window_size(eli_vec2 size, eli_cond cond);
void eli_set_window_collapsed(bool collapsed, eli_cond cond);
void eli_set_window_focus(void);
void eli_set_window_font_scale(float scale);
```

## State queries

Valid while the window is current (between `eli_begin`/`eli_end`):

```c
bool          eli_is_window_appearing(void);   /* first active frame after (re)show */
bool          eli_is_window_collapsed(void);
bool          eli_is_window_focused(void);      /* root-aware */
bool          eli_is_window_hovered(void);      /* root-aware */
eli_draw_list *eli_get_window_draw_list(void);
eli_vec2      eli_get_window_pos(void);
eli_vec2      eli_get_window_size(void);
float         eli_get_window_width(void);
float         eli_get_window_height(void);
```

## Child windows

A child is a scrollable, clipped sub-region that is a full window internally
(with its own draw list) but is decoration-free and linked to the parent's root
for focus/hover. `eli_end_child` must always be called after `eli_begin_child`,
just like the top-level pair.

```c
bool eli_begin_child(const char *str_id, eli_vec2 size,
                     eli_child_flags child_flags, eli_window_flags window_flags);
bool eli_begin_child_id(eli_id id, eli_vec2 size,
                        eli_child_flags child_flags, eli_window_flags window_flags);
void eli_end_child(void);
```

For `size`, an axis of `0` fills the parent's remaining work area and a negative
axis fills that area minus `|axis|`. `ELI_CHILD_BORDERS` draws a border;
`ELI_CHILD_RESIZE_X`/`_Y` enable the resize grip.

```c
if (eli_begin_child("list", eli_make_vec2(0, 120), ELI_CHILD_BORDERS, 0)) {
    /* scrollable region */
}
eli_end_child();
```

## Scrolling

The scroll range is derived at `eli_begin` from the window's content size versus
its work area, so scroll setters clamp against the current range immediately.
Content size comes from `eli_set_next_window_content_size` (explicit) or, in
later phases, from the layout cursor.

```c
float eli_get_scroll_x(void);      float eli_get_scroll_y(void);
float eli_get_scroll_max_x(void);  float eli_get_scroll_max_y(void);
void  eli_set_scroll_x(float x);   void  eli_set_scroll_y(float y);
void  eli_set_scroll_from_pos_x(float local_x, float center_x_ratio);
void  eli_set_scroll_from_pos_y(float local_y, float center_y_ratio);
void  eli_set_scroll_here_x(float center_x_ratio);
void  eli_set_scroll_here_y(float center_y_ratio);
```

Vertical and horizontal scrollbars appear automatically when content overflows
the work area (the horizontal one requires `ELI_WINDOW_HORIZONTAL_SCROLLBAR`).
The mouse wheel scrolls the hovered window in `eli_window_new_frame`, stepped by
`5 x font size` (capped to two-thirds of the visible height).

## Interaction

- **Move**: press-and-drag the title bar (unless `ELI_WINDOW_NO_MOVE`). The drag
  is armed in `eli_begin` and tracked in `eli_window_new_frame`.
- **Resize**: drag the bottom-right grip (unless `ELI_WINDOW_NO_RESIZE` /
  `ELI_WINDOW_AUTO_RESIZE`).
- **Collapse**: double-click the title bar (unless `ELI_WINDOW_NO_COLLAPSE`).
- **Focus / z-order**: clicking a window, or `eli_set_next_window_focus`, brings
  it to the front; the top-most window under the mouse is the hovered one.

## Compiling example

```c
#include <eli/window/eli_window.h>

int main(void)
{
    eli_context *ctx = eli_create_context();
    eli_style_colors_dark(&ctx->style);
    ctx->io.display_size = eli_make_vec2(1280.0f, 720.0f);
    ctx->io.delta_time = 1.0f / 60.0f;

    /* one frame */
    eli_new_frame();
    eli_input_update_begin_frame();
    eli_window_new_frame();

    eli_set_next_window_size(eli_make_vec2(320.0f, 240.0f), ELI_COND_FIRST_USE_EVER);
    if (eli_begin("Hello", NULL, 0)) {
        /* widgets go here in later phases */
    }
    eli_end();

    eli_window_render();
    eli_render();
    eli_input_update_end_frame();

    eli_draw_data *dd = eli_get_draw_data();  /* hand to a renderer backend */
    (void)dd;

    eli_destroy_context(ctx);
    return 0;
}
```

Build for the browser with `./build.sh serve` (see the top-level README), or run
the native window tests with `./build.sh test window`.
