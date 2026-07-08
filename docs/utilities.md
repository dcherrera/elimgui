# Misc Utilities (Phase 27)

Small cross-cutting helpers that don't belong to any single widget category:
rectangle-visibility culling, time/frame accessors, the main **viewport**, and
the shared **background/foreground draw lists** that render behind and in front
of every window.

Include the whole category with:

```c
#include <eli/util/eli_util.h>
```

It is also pulled in transitively by the umbrella header (`<eli/elimgui.h>`) via
the frame-lifecycle layer. Every header routes its libc through
`eli/core/eli_platform.h`, so the same code compiles for `wasm32` (production)
and the host libc (`-DELI_TEST_HOSTED`).

---

## Time & frame (`eli_util_misc.h`)

```c
double eli_get_time(void);
int    eli_get_frame_count(void);
```

- **`eli_get_time`** — accumulated application time in seconds. This is the sum
  of `io.delta_time` advanced by `eli_new_frame`/`eli_frame_begin`. Returns `0.0`
  when no context is current.
- **`eli_get_frame_count`** — number of frames begun so far (incremented once per
  `eli_frame_begin`). Returns `0` when no context is current.

```c
eli_frame_begin();
/* ... */
eli_frame_end();
printf("frame %d at t=%.3fs\n", eli_get_frame_count(), eli_get_time());
```

---

## Visibility (`eli_util_misc.h`)

```c
bool eli_is_rect_visible(eli_vec2 size);
bool eli_is_rect_visible_vec2(eli_vec2 rect_min, eli_vec2 rect_max);
```

Both test a rectangle against the **current window's clip rectangle** and return
`true` when the rectangle is at least partially inside it — the standard hook for
skipping the layout/draw work of off-screen content.

- **`eli_is_rect_visible(size)`** — the rectangle's top-left corner is the current
  window's layout cursor (`cursor_pos`); its extent is `size`.
- **`eli_is_rect_visible_vec2(rect_min, rect_max)`** — an explicit screen-space
  rectangle.

Both return `false` when there is no current window (call them between
`eli_begin`/`eli_end`).

```c
if (eli_begin("Feed", NULL, 0)) {
    for (int i = 0; i < item_count; i++) {
        if (!eli_is_rect_visible(eli_make_vec2(0, row_height))) {
            eli_dummy(eli_make_vec2(0, row_height));   /* advance, skip drawing */
            continue;
        }
        draw_row(i);
    }
}
eli_end();
```

---

## Viewport (`eli_viewport.h`)

elimgui targets a single browser canvas, so exactly one viewport exists.

```c
typedef struct eli_viewport {
    eli_id             id;         /* stable identity (ELI_VIEWPORT_MAIN_ID) */
    eli_viewport_flags flags;      /* ELI_VIEWPORT_FLAGS_IS_PRIMARY */
    eli_vec2           pos;        /* top-left, screen space (always 0,0) */
    eli_vec2           size;       /* full size == io.display_size */
    eli_vec2           work_pos;   /* usable work-area origin */
    eli_vec2           work_size;  /* usable work-area size */
} eli_viewport;

eli_viewport *eli_get_main_viewport(void);
```

`eli_get_main_viewport` returns a pointer to a stable, file-static viewport whose
fields are refreshed from the current context's `io.display_size` on every call.
`pos` is `(0,0)`; the **work area equals the full viewport** for now (a future
multi-viewport phase may reserve space for OS decoration). Returns `NULL` when no
context is current. Because the fields are updated on each call, do not cache the
values across a display resize — re-query instead.

```c
eli_viewport *vp = eli_get_main_viewport();
eli_set_next_window_pos(vp->work_pos, ELI_COND_ALWAYS, eli_make_vec2(0, 0));
eli_set_next_window_size(vp->work_size, ELI_COND_ALWAYS);
eli_begin("Fullscreen", NULL, ELI_WINDOW_NO_DECORATION);
/* ... */
eli_end();
```

---

## Background & foreground draw lists (`eli_util_misc.h`)

```c
eli_draw_list *eli_get_background_draw_list(void);
eli_draw_list *eli_get_foreground_draw_list(void);
```

Two shared, persistent draw lists let you draw arbitrary geometry **behind** all
windows (background) or **in front of** all windows (foreground) — useful for a
global backdrop, an overlay, debug gizmos, or a custom cursor.

- **Lazy**: each list is allocated and initialized (full-screen clip) the first
  time it's requested; that first request also registers the context's
  `util_shutdown_fn`, so `eli_destroy_context` frees both lists.
- **Persistent & cleared per frame**: the lists survive across frames but are
  reset at the start of each frame by `eli_frame_begin` (which calls the internal
  `eli_util_new_frame`). Request the list and draw into it **after**
  `eli_frame_begin`.
- **Ordering**: when `eli_window_render` assembles the frame's draw data, it
  prepends the background list (index `0`) and appends the foreground list (last
  index) — but only when the list actually holds geometry. So in
  `eli_get_draw_data()->cmd_lists`, background renders first, then every window
  back-to-front, then foreground last.
- **Ownership**: both lists are owned by the context. Do **not** free them.

```c
eli_frame_begin();

/* A full-canvas backdrop behind every window. */
eli_viewport *vp = eli_get_main_viewport();
eli_draw_list *bg = eli_get_background_draw_list();
eli_draw_list_add_rect_filled(bg, vp->pos,
                              eli_vec2_add(vp->pos, vp->size),
                              ELI_COL32(20, 20, 28, 255), 0.0f, 0);

/* ... eli_begin / widgets / eli_end ... */

/* An overlay crosshair in front of everything. */
eli_draw_list *fg = eli_get_foreground_draw_list();
eli_draw_list_add_line(fg, eli_make_vec2(mx - 6, my), eli_make_vec2(mx + 6, my),
                       ELI_COL32(255, 255, 0, 255), 1.0f);

eli_frame_end();
```

---

## Gotchas

- The util lists are cleared at **frame begin**, so anything you drew last frame
  is gone — redraw every frame (immediate-mode).
- `eli_is_rect_visible*` need a current window; outside `eli_begin`/`eli_end`
  they return `false`.
- `eli_get_main_viewport()` returns a shared pointer whose fields mutate on each
  call. Don't stash the pointer expecting a snapshot.
- Filled rectangles into the util lists use the atlas white-pixel UV, which the
  per-frame reset copies from the current font when one is pushed; push a font
  (or use lines) if you need pixel-perfect solids in a fontless setup.
