# Core Types (Phase 1)

The core category is the foundation every other phase builds on. It defines the value
types (vectors, rectangles, ids, packed colors), the full set of core enumerations, the
input/output bridge (`eli_io`), the appearance configuration (`eli_style`), and the
context container with its create/destroy and per-frame lifecycle.

All of it is reachable through the umbrella header:

```c
#include <eli/elimgui.h>
```

Headers live under `include/eli/core/`:

| Header | Provides |
|--------|----------|
| `eli_platform.h` | Libc seam (provided; every header includes it, never `<jaclibc.h>`) |
| `eli_types.h` | `eli_vec2`, `eli_vec4`, `eli_rect`, `eli_id`, `eli_col32`, color macros, inline helpers |
| `eli_enums.h` | All core enums and flag sets |
| `eli_io.h` | `eli_io` structure |
| `eli_style_types.h` | `eli_style` structure |
| `eli_context.h` | `eli_context` + context/frame lifecycle |
| `eli_core.h` | Aggregator that includes the five headers above |

## Value Types

```c
typedef struct { float x, y; }        eli_vec2;   // position / size / spacing
typedef struct { float x, y, z, w; }  eli_vec4;   // RGBA color (0..1) or quad
typedef struct { float x, y, w, h; }  eli_rect;   // origin (top-left) + size
typedef uint32_t eli_id;                           // widget/window identity hash
typedef uint32_t eli_col32;                        // packed R,G,B,A color
```

### Colors

Colors pack into an `eli_col32` with red in the low byte:

```c
#define ELI_COL32(r, g, b, a)  // (a<<24)|(b<<16)|(g<<8)|r
#define ELI_COL32_WHITE        0xFFFFFFFF
#define ELI_COL32_BLACK        0x000000FF
#define ELI_COL32_BLACK_TRANS  0x00000000
// also: ELI_COL32_RED / _GREEN / _BLUE, and the shift macros
// ELI_COL32_R_SHIFT / _G_SHIFT / _B_SHIFT / _A_SHIFT, plus ELI_COL32_A_MASK
```

### Inline Helpers

Constructors: `eli_make_vec2`, `eli_make_vec4`, `eli_make_rect`.

Vector math: `eli_vec2_add`, `eli_vec2_sub`, `eli_vec2_scale`.

Scalars: `eli_min_f`, `eli_max_f`, `eli_clamp_f`.

Rectangles: `eli_rect_width`, `eli_rect_height`, `eli_rect_size`, `eli_rect_min`,
`eli_rect_max`, `eli_rect_center`, and:

```c
// Min edges inclusive, max edges exclusive (matches ImGui's ImRect::Contains):
// contains [x, x+w) x [y, y+h). Adjacent rects never both claim a shared border.
bool eli_rect_contains(eli_rect r, eli_vec2 p);
```

Color conversion (round-trips exactly for integer-valued colors):

```c
eli_vec4  eli_color_u32_to_vec4(eli_col32 c);   // unpack to 0..1 RGBA
eli_col32 eli_color_vec4_to_u32(eli_vec4 v);    // pack, saturating to [0,1]
```

## Enumerations

`eli_enums.h` defines, with Dear ImGui-compatible values:

- **Scalars/behavior:** `eli_dir`, `eli_cond`, `eli_data_type`
- **Input:** `eli_key` (keyboard, keypad, modifiers, F-keys, and mouse/wheel-as-key codes;
  `ELI_KEY_COUNT` ends the named-key range), `eli_mouse_button`, `eli_mouse_cursor`
- **Appearance indices:** `eli_col` (ends `ELI_COL_COUNT` = 56), `eli_style_var`
  (ends `ELI_STYLE_VAR_COUNT` = 33)
- **Flags:** `eli_window_flags`, `eli_child_flags`, `eli_item_flags`, `eli_hovered_flags`,
  `eli_config_flags`, `eli_backend_flags`

## IO and Style

`eli_io` is the bridge to the host backend: display size, delta time, mouse/keyboard
state, the text-input queue, capture-intent outputs (`want_capture_mouse`, etc.), and
clipboard callbacks. The backend fills the inputs; elimgui writes the outputs each frame.

`eli_style` holds every sizing/spacing/rounding/alignment/behavior value plus
`eli_vec4 colors[ELI_COL_COUNT]`. `eli_create_context` populates the sizing values with the
Dear ImGui defaults; the `colors[]` theme is left zeroed for the style phase to fill.

## Context and Frame Lifecycle

```c
eli_context* eli_create_context(void);          // alloc + default IO/style; becomes current if none is
void         eli_destroy_context(eli_context*); // NULL = destroy current; clears current if it was
eli_context* eli_get_current_context(void);
void         eli_set_current_context(eli_context*);
eli_io*      eli_get_io(void);                   // &current->io   (NULL if no context)
eli_style*   eli_get_style(void);                // &current->style

void           eli_new_frame(void);   // advance frame_count, open frame scope, derive framerate
void           eli_end_frame(void);   // close frame scope (no-op if already closed)
void           eli_render(void);      // ensure frame scope closed; draw-data assembly arrives in Phase 2
eli_draw_data* eli_get_draw_data(void); // NULL until the draw phase produces output
```

A single global "current context" backs these calls. Header-only builds are a single
translation unit (the app includes `elimgui.h` once), so the current context is file-static.

### Thread Safety

Context/frame functions mutate the global current-context pointer and are **not**
thread-safe. The pure helpers in `eli_types.h` are thread-safe and reentrant.

## Usage Example

```c
#include <eli/elimgui.h>

int main(void)
{
    eli_context *ctx = eli_create_context();   // becomes the current context
    eli_set_current_context(ctx);

    eli_io *io = eli_get_io();
    io->display_size = eli_make_vec2(1280.0f, 720.0f);
    io->delta_time = 1.0f / 60.0f;

    // Per-frame loop:
    eli_new_frame();
    //   ... widget calls will go here in later phases ...
    eli_render();
    eli_draw_data *draw_data = eli_get_draw_data(); // NULL until Phase 2

    (void)draw_data;
    eli_destroy_context(ctx);
    return 0;
}
```

## Gotchas

- Include `eli_platform.h` (or a header that does), never `<jaclibc.h>` directly.
- `eli_rect_contains` treats max edges as exclusive — do not expect the bottom-right
  corner point to be "inside".
- The style `colors[]` array is zeroed in Phase 1; a visible theme comes from the style
  phase.
- `eli_get_draw_data` returns `NULL` until the draw phase; `eli_render` currently only
  closes the frame scope.
