# Phase 22: Images

Draw-list image helpers and image widget family for elimgui.

---

## Overview

Phase 22 adds textured-quad rendering to the draw list and two convenience
widgets — `eli_image` and `eli_image_button` — that build on top of them.
The design mirrors Dear ImGui's image API (same names, same parameter order)
while staying in C11 and routing all libc calls through
`eli/core/eli_platform.h`.

---

## Draw-List API

These functions operate directly on an `eli_draw_list` and do not require a
window context.

### `eli_draw_list_add_image`

```c
void eli_draw_list_add_image(
    eli_draw_list *list,
    uint32_t       user_texture_id,
    eli_vec2       p_min,
    eli_vec2       p_max,
    eli_vec2       uv_min,
    eli_vec2       uv_max,
    eli_col32      col);
```

Appends a textured axis-aligned rectangle.

- Emits **4 vertices** and **6 indices** (two triangles, counter-clockwise).
- If the texture differs from the draw list's current `cmd_texture_id`, a new
  draw command is opened for `user_texture_id` and closed afterwards
  (push/pop pattern that avoids splitting unrelated batches).
- If `col` has zero alpha the call is a no-op.
- `uv_min / uv_max` are the UV coordinates for the top-left and bottom-right
  corners respectively (standard OpenGL-style: origin bottom-left, or
  top-left depending on your renderer).

### `eli_draw_list_add_image_quad`

```c
void eli_draw_list_add_image_quad(
    eli_draw_list *list,
    uint32_t       user_texture_id,
    eli_vec2 p1, eli_vec2 p2, eli_vec2 p3, eli_vec2 p4,
    eli_vec2 uv1, eli_vec2 uv2, eli_vec2 uv3, eli_vec2 uv4,
    eli_col32 col);
```

Like `add_image` but accepts four arbitrary corner positions. Useful for
rotated or skewed image quads. Vertices are stored in the order p1–p2–p3–p4
(top-left, top-right, bottom-right, bottom-left for the normal case).

### `eli_draw_list_add_image_rounded`

```c
void eli_draw_list_add_image_rounded(
    eli_draw_list   *list,
    uint32_t         user_texture_id,
    eli_vec2         p_min,
    eli_vec2         p_max,
    eli_vec2         uv_min,
    eli_vec2         uv_max,
    eli_col32        col,
    float            rounding,
    eli_draw_flags   flags);
```

Renders the image clipped to a rounded rectangle.

- Falls back to `add_image` when `rounding < 0.5` or when `flags` selects no
  rounded corners (`ELI_DRAW_ROUND_CORNERS_NONE`).
- Otherwise, builds the rounded path with `eli_draw_list_path_rect` →
  `eli_draw_list_path_fill_convex`, then applies bilinear UV remapping
  (`eli_image__shade_verts_linear_uv`) to map vertex positions to the
  `[uv_min, uv_max]` range — the same technique as Dear ImGui's
  `ShadeVertsLinearUV`.
- Produces more than 4 vertices / 6 indices for any rounding value ≥ 0.5.

**Round-corner flags** (subset of `eli_draw_flags`):

| Flag | Corners rounded |
|------|-----------------|
| `ELI_DRAW_ROUND_CORNERS_ALL` | All four |
| `ELI_DRAW_ROUND_CORNERS_TOP_LEFT` | Top-left only |
| `ELI_DRAW_ROUND_CORNERS_TOP_RIGHT` | Top-right only |
| `ELI_DRAW_ROUND_CORNERS_BOT_LEFT` | Bottom-left only |
| `ELI_DRAW_ROUND_CORNERS_BOT_RIGHT` | Bottom-right only |

---

## Widget API

Both widgets must be called inside an open window scope
(`eli_begin` … `eli_end`).

### `eli_image`

```c
void eli_image(
    uint32_t user_texture_id,
    eli_vec2 image_size,
    eli_vec2 uv0,
    eli_vec2 uv1,
    eli_vec4 tint_col,
    eli_vec4 border_col);
```

A non-interactive image widget. Advances the layout cursor by `image_size`
(plus a 1-pixel border on all sides when `border_col.w > 0`).

- **`image_size`** — dimensions in pixels. The layout item occupies exactly
  this size (or `image_size + (2,2)` with a border).
- **`tint_col`** — RGBA float4 multiplied against the texture color (`(1,1,1,1)`
  = no tint).
- **`border_col`** — when `.w > 0` a 1-pixel rectangle is drawn around the
  image in this color.
- `eli_get_item_rect_size()` after the call returns the full item size
  (including any border).

### `eli_image_button`

```c
bool eli_image_button(
    const char *str_id,
    uint32_t    user_texture_id,
    eli_vec2    image_size,
    eli_vec2    uv0,
    eli_vec2    uv1,
    eli_vec4    bg_col,
    eli_vec4    tint_col);
```

A clickable image. Returns `true` on the frame the mouse button is **released**
over the widget (standard press-on-release policy, matching Dear ImGui).

- **`str_id`** — string used to generate a stable widget ID (see `eli_get_id`).
  Use `##tag` suffixes to disambiguate multiple buttons.
- **`image_size`** — inner image dimensions, not including padding.
- Total hit area = `image_size + style.frame_padding * 2`.
- **`bg_col`** — optional background fill behind the image (drawn with
  `eli_render_frame`).
- **`tint_col`** — color multiplier for the texture.

---

## Texture IDs

Both draw-list functions and widgets accept a plain `uint32_t` texture handle.
The value is passed through to the renderer opaquely — elimgui never
dereferences it. Bind your GPU texture handle (or any renderer-side key) to
this field.

---

## Include

```c
#include <eli/widgets/eli_image.h>
```

Or pull in the full widget surface:

```c
#include <eli/widgets/eli_widgets.h>
```

`eli_image.h` depends only on `eli_widget_behavior.h` (already included
through `eli_widgets.h`) and `eli/core/eli_platform.h`.

---

## Example

```c
/* Draw a full-texture image at the cursor, with a red border. */
eli_image(my_tex, (eli_vec2){128, 128},
          (eli_vec2){0.0f, 0.0f}, (eli_vec2){1.0f, 1.0f},
          (eli_vec4){1.0f, 1.0f, 1.0f, 1.0f},   /* white tint */
          (eli_vec4){1.0f, 0.0f, 0.0f, 1.0f});   /* red border */

/* Clickable image button. */
if (eli_image_button("play##toolbar", play_tex,
                     (eli_vec2){32, 32},
                     (eli_vec2){0, 0}, (eli_vec2){1, 1},
                     (eli_vec4){0, 0, 0, 0},       /* no bg */
                     (eli_vec4){1, 1, 1, 1}))
{
    start_playback();
}

/* Rounded image directly on the draw list. */
eli_draw_list *dl = eli_get_window_draw_list();
eli_draw_list_add_image_rounded(dl, banner_tex,
    (eli_vec2){10, 10}, (eli_vec2){310, 110},
    (eli_vec2){0, 0}, (eli_vec2){1, 1},
    ELI_COL32_WHITE, 12.0f, ELI_DRAW_ROUND_CORNERS_ALL);
```

---

## Implementation Notes

- **Header-only**: all code lives in `include/eli/widgets/eli_image.h` as
  `static inline` functions. No translation unit is needed.
- **Texture batching**: `add_image` and `add_image_quad` only emit a new draw
  command when the requested texture differs from the draw list's active
  texture, preserving batch efficiency.
- **UV remapping for rounded images**: implemented via
  `eli_image__shade_verts_linear_uv` (file-internal helper). The function
  records the vertex count before `path_fill_convex`, then linearly maps each
  new vertex's (x,y) position into `[uv_min, uv_max]`.
- **Rounding fallback**: `add_image_rounded` with `rounding < 0.5` or
  `ELI_DRAW_ROUND_CORNERS_NONE` forwards to `add_image` (zero overhead).
