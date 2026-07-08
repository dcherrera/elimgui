# Draw System (Phase 2)

The draw system is elimgui's renderer-agnostic output layer. Widgets never touch
pixels — they append geometry and draw commands to an **`eli_draw_list`**, and a
backend (Canvas2D, WebGL, …) translates the resulting vertex/index buffers into
pixels. This document covers the draw-list data types, the primitive and path
APIs, the clip/texture stacks, primitive reservation, and channel splitting.

Include the whole draw category with:

```c
#include <eli/draw/eli_draw.h>
```

Every draw header routes its libc through `eli/core/eli_platform.h`, so the same
code compiles for `wasm32` (production) and the host libc (`-DELI_TEST_HOSTED`).

> **Rendering model.** Geometry is generated **without anti-aliasing**, so vertex
> and index counts are fully deterministic. Smoothing, if wanted, is the
> backend's job. `add_text` is intentionally **not** part of this phase — text
> rendering arrives with the font system (Phase 3).

---

## Data types (`eli_draw_types.h`)

| Type | Purpose |
|------|---------|
| `eli_draw_vert` | One vertex: `x, y` position, `u, v` texture coords, `col` packed color. |
| `eli_draw_idx` | 16-bit index into a list's vertex buffer. |
| `eli_draw_cmd` | One draw call: `clip_rect` (origin+size scissor), `texture_id`, `idx_offset`, `elem_count`, optional `user_callback`. |
| `eli_draw_list` | Accumulates commands, vertices, indices, plus path/clip/texture/channel working state. |
| `eli_draw_data` | A frame's finished output: an array of draw lists and totals. |
| `eli_draw_callback` | `void (*)(const eli_draw_list *parent, const eli_draw_cmd *cmd)`. |

### Flags

`eli_draw_flags` (per-draw): `ELI_DRAW_CLOSED`, and corner selectors
`ELI_DRAW_ROUND_CORNERS_TOP_LEFT/TOP_RIGHT/BOT_LEFT/BOT_RIGHT`, the composites
`_TOP/_BOTTOM/_LEFT/_RIGHT/_ALL`, and `ELI_DRAW_ROUND_CORNERS_NONE`.

`eli_draw_list_flags` (construction): `ELI_DRAW_LIST_NONE`,
`ELI_DRAW_LIST_ANTI_ALIASED_LINES`, `_ANTI_ALIASED_LINES_USE_TEX`,
`_ANTI_ALIASED_FILL`, `_ALLOW_VTX_OFFSET`. (The AA bits are recognized but the
current geometry path is non-AA.)

---

## Draw-list lifecycle (`eli_draw_list.h`)

```c
void eli_draw_list_init(eli_draw_list *list, eli_draw_list_flags flags);
void eli_draw_list_reset(eli_draw_list *list);   // reuse without freeing buffers
void eli_draw_list_clear(eli_draw_list *list);   // release all buffers, zero it
```

`init` zeroes the list, sets a fullscreen clip rect, and opens one empty
command. Buffers grow geometrically (~1.5×) via the libc seam; `clear` frees
everything (commands, vertices, indices, path, clip/texture stacks, channels).

### Command batching

Geometry accumulates into the last command. Changing the clip rect or texture
either splits off a new command (if the current one already has geometry) or
retags/merges the trailing empty command — so consecutive draws that share clip
and texture collapse into a single command.

---

## Primitives (`eli_draw_prim.h`)

All positions are in screen pixels; `col` is an `eli_col32` (a fully transparent
color makes the call a no-op).

```c
void eli_draw_list_add_line(list, p1, p2, col, thickness);
void eli_draw_list_add_rect(list, p_min, p_max, col, rounding, flags, thickness);
void eli_draw_list_add_rect_filled(list, p_min, p_max, col, rounding, flags);
void eli_draw_list_add_rect_filled_multi_color(list, p_min, p_max, tl, tr, br, bl);
void eli_draw_list_add_triangle(list, p1, p2, p3, col, thickness);
void eli_draw_list_add_triangle_filled(list, p1, p2, p3, col);
void eli_draw_list_add_quad(list, p1, p2, p3, p4, col, thickness);
void eli_draw_list_add_quad_filled(list, p1, p2, p3, p4, col);
void eli_draw_list_add_circle(list, center, radius, col, num_segments, thickness);
void eli_draw_list_add_circle_filled(list, center, radius, col, num_segments);
void eli_draw_list_add_ngon(list, center, radius, col, num_segments, thickness);
void eli_draw_list_add_ngon_filled(list, center, radius, col, num_segments);
void eli_draw_list_add_ellipse(list, center, radius, col, rot, num_segments, thickness);
void eli_draw_list_add_ellipse_filled(list, center, radius, col, rot, num_segments);
void eli_draw_list_add_polyline(list, points, num_points, col, flags, thickness);
void eli_draw_list_add_convex_poly_filled(list, points, num_points, col);
void eli_draw_list_add_bezier_cubic(list, p1, p2, p3, p4, col, thickness, num_segments);
void eli_draw_list_add_bezier_quadratic(list, p1, p2, p3, col, thickness, num_segments);
```

**Deterministic counts** (non-AA):

- Filled rect (no rounding): **4 vertices, 6 indices**.
- Stroked line/segment: each segment → **4 vertices, 6 indices**.
- Open polyline of *n* points → *n−1* segments; closed → *n* segments.
- Convex fill of *n* points → **n vertices, (n−2)·3 indices**.
- Filled circle/ngon with `num_segments = N` → **N vertices, (N−2)·3 indices**.
- Circle/ngon supports `num_segments <= 0` to auto-pick a count from the radius
  and the list's `circle_segment_max_error` (default `0.30`).

---

## Path API (`eli_draw_path.h`)

Build a point list, then finish it with a stroke or a convex fill:

```c
void eli_draw_list_path_clear(list);
void eli_draw_list_path_line_to(list, pos);
void eli_draw_list_path_line_to_merge_duplicate(list, pos);
void eli_draw_list_path_fill_convex(list, col);                 // fill + clear
void eli_draw_list_path_stroke(list, col, flags, thickness);    // stroke + clear
void eli_draw_list_path_arc_to(list, center, radius, a_min, a_max, num_segments);
void eli_draw_list_path_arc_to_fast(list, center, radius, a_min_of_12, a_max_of_12);
void eli_draw_list_path_elliptical_arc_to(list, center, radius, rot, a_min, a_max, num_segments);
void eli_draw_list_path_bezier_cubic_curve_to(list, p2, p3, p4, num_segments);
void eli_draw_list_path_bezier_quadratic_curve_to(list, p2, p3, num_segments);
void eli_draw_list_path_rect(list, a, b, rounding, flags);
```

`path_arc_to`/`elliptical_arc_to` accept `num_segments <= 0` for automatic
tessellation. `path_arc_to_fast` uses the 12-o'clock convention (0: east,
3: south, 6: west, 9: north). Bezier `num_segments == 0` auto-tessellates
adaptively.

---

## Clip and texture stacks (`eli_draw_list.h`)

```c
void     eli_draw_list_push_clip_rect(list, clip_min, clip_max, intersect_with_current);
void     eli_draw_list_push_clip_rect_full_screen(list);
void     eli_draw_list_pop_clip_rect(list);
eli_vec2 eli_draw_list_get_clip_rect_min(list);
eli_vec2 eli_draw_list_get_clip_rect_max(list);
void     eli_draw_list_push_texture_id(list, texture_id);
void     eli_draw_list_pop_texture_id(list);
```

Clip rects are held internally as min/max for easy intersection; each emitted
command stores its clip as an origin+size `eli_rect` for the backend's scissor.

---

## Primitive reservation (`eli_draw_list.h`)

For hand-writing geometry: reserve first, then fill the reserved slots.

```c
void eli_draw_list_prim_reserve(list, idx_count, vtx_count);
void eli_draw_list_prim_unreserve(list, idx_count, vtx_count);
void eli_draw_list_prim_rect(list, a, c, col);
void eli_draw_list_prim_rect_uv(list, a, c, uv_a, uv_c, col);
void eli_draw_list_prim_quad_uv(list, a, b, c, d, uv_a, uv_b, uv_c, uv_d, col);
void eli_draw_list_prim_write_vtx(list, pos, uv, col);
void eli_draw_list_prim_write_idx(list, idx);
void eli_draw_list_prim_vtx(list, pos, uv, col);   // write idx + vtx together
```

---

## Advanced (`eli_draw_list.h`)

```c
void           eli_draw_list_add_callback(list, callback, callback_data);
void           eli_draw_list_add_draw_cmd(list);
eli_draw_list *eli_draw_list_clone_output(const eli_draw_list *list);
```

`add_callback` attaches a backend callback in place of geometry.
`clone_output` deep-copies the rendered command/vertex/index buffers into a new
heap list (release with `eli_draw_list_clear` then `free`).

---

## Channels (`eli_draw_channels.h`)

Split a list into ordered command/index streams so geometry emitted out of order
still merges back in the right order (e.g. draw a background *after* the widgets
that sit on top of it). Vertices stay shared — only commands and indices split.

```c
void eli_draw_list_channels_split(list, count);
void eli_draw_list_channels_set_current(list, n);
void eli_draw_list_channels_merge(list);
```

On merge, commands concatenate in channel order (0, 1, 2, …) regardless of the
order they were drawn, and each command's index offset is rebuilt. Nested
splitting is not supported.

---

## Example

```c
#include <eli/draw/eli_draw.h>

void build_frame(void)
{
    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);

    // A filled panel and its border.
    eli_draw_list_add_rect_filled(&dl, eli_make_vec2(20, 20), eli_make_vec2(220, 140),
                                  ELI_COL32(40, 40, 48, 255), 6.0f, ELI_DRAW_ROUND_CORNERS_ALL);
    eli_draw_list_add_rect(&dl, eli_make_vec2(20, 20), eli_make_vec2(220, 140),
                           ELI_COL32(90, 90, 100, 255), 6.0f, ELI_DRAW_ROUND_CORNERS_ALL, 1.0f);

    // A filled circle clipped to the panel interior.
    eli_draw_list_push_clip_rect(&dl, eli_make_vec2(20, 20), eli_make_vec2(220, 140), true);
    eli_draw_list_add_circle_filled(&dl, eli_make_vec2(120, 80), 30.0f, ELI_COL32_GREEN, 24);
    eli_draw_list_pop_clip_rect(&dl);

    // Hand over dl.cmds / dl.vtx / dl.idx to your renderer here.

    eli_draw_list_clear(&dl);
}
```

Compile the WebAssembly build with the Homebrew LLVM clang:

```bash
/opt/homebrew/opt/llvm/bin/clang --target=wasm32 -nostdlib \
    -Ivendor/jaclibc/include -Iinclude -Ivendor -O2 -c -o frame.o frame.c
```

---

## Gotchas

- Fully transparent colors short-circuit — no geometry is emitted.
- After a `channels_split`, the current channel's buffers alias the list's live
  buffers; use `channels_merge` before reading final output.
- `add_text` and image helpers are **not** in this phase (font system / images).
