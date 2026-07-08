# Sliders & Drags (Phase 11)

Value-adjustment widgets: **sliders** map the mouse position along a track to a
value between a fixed `[min, max]`, while **drags** adjust a value by the mouse
drag delta (times a per-pixel speed) and may be unbounded. Both share one scalar
core that works across every `eli_data_type` and supports linear or logarithmic
mapping plus printf-style display formats.

Headers (each usable directly):

```c
#include <eli/widgets/eli_slider.h>          /* sliders + vertical sliders */
#include <eli/widgets/eli_drag.h>            /* drags + range drags */
#include <eli/widgets/eli_slider_behavior.h> /* shared behavior + scalar helpers */
```

Drive them inside a window scope (`eli_begin` / `eli_end`).

## Flags — `eli_slider_flags`

Defined in `eli/core/eli_enums.h` (shared by sliders and drags):

| Flag | Effect |
|------|--------|
| `ELI_SLIDER_NONE` | Default behavior. |
| `ELI_SLIDER_LOGARITHMIC` | Map the value logarithmically along the track. |
| `ELI_SLIDER_NO_ROUND_TO_FORMAT` | Do not round the value to the format's precision. |
| `ELI_SLIDER_NO_INPUT` | Reserved: disables Ctrl+click / tab-to-type text entry. |
| `ELI_SLIDER_WRAP_AROUND` | Drags wrap past the bounds instead of clamping. |
| `ELI_SLIDER_CLAMP_ON_INPUT` | Clamp typed input to `[min, max]`. |
| `ELI_SLIDER_CLAMP_ZERO_RANGE` | Clamp even when `min == max == 0`. |
| `ELI_SLIDER_ALWAYS_CLAMP` | `CLAMP_ON_INPUT | CLAMP_ZERO_RANGE`. |

## Sliders

```c
bool eli_slider_float (const char *label, float *v, float v_min, float v_max,
                       const char *format, eli_slider_flags flags);
bool eli_slider_float2(const char *label, float v[2], float v_min, float v_max,
                       const char *format, eli_slider_flags flags);
bool eli_slider_float3(const char *label, float v[3], ...);
bool eli_slider_float4(const char *label, float v[4], ...);
bool eli_slider_angle (const char *label, float *v_rad,
                       float v_degrees_min, float v_degrees_max,
                       const char *format, eli_slider_flags flags);
bool eli_slider_int   (const char *label, int *v, int v_min, int v_max,
                       const char *format, eli_slider_flags flags);
bool eli_slider_int2/3/4(const char *label, int v[N], int v_min, int v_max, ...);
bool eli_slider_scalar  (const char *label, eli_data_type dt, void *p_data,
                         const void *p_min, const void *p_max,
                         const char *format, eli_slider_flags flags);
bool eli_slider_scalar_n(const char *label, eli_data_type dt, void *p_data, int components,
                         const void *p_min, const void *p_max,
                         const char *format, eli_slider_flags flags);
```

- **Return value:** `true` on any frame the value changes (also marks the item
  edited, queryable via `eli_is_item_edited`).
- **`format`:** printf spec used to render the value in the track and to round it
  (float types only); pass `NULL` for the type default (`"%.3f"` for float,
  `"%d"`/`"%u"` for ints).
- **`eli_slider_angle`:** stores radians but edits degrees; default format
  `"%.0f deg"`.
- **`_float2/3/4` / `_int2/3/4` / `_scalar_n`:** lay `components` sliders on one
  line sharing a range, followed by the label.

### Vertical sliders

```c
bool eli_v_slider_float (const char *label, eli_vec2 size, float *v,
                         float v_min, float v_max, const char *format, eli_slider_flags flags);
bool eli_v_slider_int   (const char *label, eli_vec2 size, int *v,
                         int v_min, int v_max, const char *format, eli_slider_flags flags);
bool eli_v_slider_scalar(const char *label, eli_vec2 size, eli_data_type dt, void *p_data,
                         const void *p_min, const void *p_max,
                         const char *format, eli_slider_flags flags);
```

`size` is an explicit pixel size; higher mouse position = larger value.

## Drags

```c
bool eli_drag_float (const char *label, float *v, float v_speed,
                     float v_min, float v_max, const char *format, eli_slider_flags flags);
bool eli_drag_float2/3/4(const char *label, float v[N], float v_speed,
                         float v_min, float v_max, const char *format, eli_slider_flags flags);
bool eli_drag_float_range2(const char *label, float *v_current_min, float *v_current_max,
                           float v_speed, float v_min, float v_max,
                           const char *format, const char *format_max, eli_slider_flags flags);
bool eli_drag_int   (const char *label, int *v, float v_speed,
                     int v_min, int v_max, const char *format, eli_slider_flags flags);
bool eli_drag_int2/3/4(const char *label, int v[N], float v_speed,
                       int v_min, int v_max, const char *format, eli_slider_flags flags);
bool eli_drag_int_range2(const char *label, int *v_current_min, int *v_current_max,
                         float v_speed, int v_min, int v_max,
                         const char *format, const char *format_max, eli_slider_flags flags);
bool eli_drag_scalar  (const char *label, eli_data_type dt, void *p_data, float v_speed,
                       const void *p_min, const void *p_max,
                       const char *format, eli_slider_flags flags);
bool eli_drag_scalar_n(const char *label, eli_data_type dt, void *p_data, int components,
                       float v_speed, const void *p_min, const void *p_max,
                       const char *format, eli_slider_flags flags);
```

- **`v_speed`:** value change per pixel dragged. `0` auto-derives from the range
  (1% of `v_max - v_min`). Holding **Alt** slows the drag 100×, **Shift** speeds it
  10× (matches Dear ImGui).
- **Bounds:** pass equal `v_min`/`v_max` (both from the same pointer) for an
  unbounded drag. When bounded, the value clamps unless `ELI_SLIDER_WRAP_AROUND`.
- **`_range2`:** two side-by-side drags editing a `[min, max]` pair, each clamped
  so the pair stays ordered — usually pass `ELI_SLIDER_ALWAYS_CLAMP`.

## Example

```c
#include <eli/widgets/eli_slider.h>
#include <eli/widgets/eli_drag.h>

static float amount   = 0.5f;
static int   count    = 3;
static float color[3] = {0.2f, 0.4f, 0.8f};
static float rng[2]   = {2.0f, 8.0f};

void frame(void)
{
    eli_begin("Controls", NULL, 0);

    if (eli_slider_float("Amount", &amount, 0.0f, 1.0f, "%.2f", 0))
        recompute();

    eli_slider_int("Count", &count, 0, 10, NULL, 0);
    eli_slider_float3("Color", color, 0.0f, 1.0f, "%.3f", 0);

    eli_drag_float("Speed", &amount, 0.01f, 0.0f, 4.0f, "%.2f", 0);
    eli_drag_float_range2("Range", &rng[0], &rng[1], 0.1f, 0.0f, 10.0f,
                          NULL, NULL, ELI_SLIDER_ALWAYS_CLAMP);

    eli_end();
}
```

## Scalar core (`eli_slider_behavior.h`)

The two internal behaviors and the reusable scalar helpers:

- `eli_slider_behavior(bb, id, dt, p_data, p_min, p_max, format, flags, out_grab_bb)`
  — track-relative mouse mapping; also outputs the grab rectangle to draw.
- `eli_drag_behavior(id, dt, p_data, v_speed, p_min, p_max, format, flags)`
  — delta-accumulating adjustment.
- Data-type helpers: `eli_scalar_read` / `eli_scalar_write` / `eli_scalar_clamp` /
  `eli_scalar_format`, `eli_data_type_size` / `_is_float` / `_default_format`.
- Mapping: `eli_scale_ratio_from_value` (value → `t` in `[0,1]`) and
  `eli_scale_value_from_ratio` (`t` → value), linear and logarithmic.
- Rounding: `eli_parse_format_precision`, `eli_round_scalar_with_format`.

## Gotchas

- **Ctrl+click / tab to type a value is deferred** to the input-text phase
  (Phase 12). `ELI_SLIDER_NO_INPUT` is accepted but the text-entry path it gates is
  not yet implemented. Interaction today is mouse-drag / mouse-track only.
- **64-bit integers** are carried through the math as `double`, so values above
  2^53 lose precision — the same "half natural range" caveat Dear ImGui documents.
- Values are carried and rounded internally; for integer types the write rounds to
  nearest and clamps to the destination type's representable range.
- Keyboard/gamepad nav tweaking is not wired yet (nav lands in a later phase).
