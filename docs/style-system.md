# Style System (Phase 6)

The style system defines elimgui's appearance: the per-slot color table, the
themable sizing/spacing/rounding values, the push/pop stacks that override them
temporarily, per-item behavior flags, and the color conversion/lookup helpers.
It lives under `include/eli/style/` and is pulled in with:

```c
#include <eli/style/eli_style.h>
```

The `eli_style` struct itself and the `eli_col` / `eli_style_var` enumerations are
defined by the core phase (`include/eli/core/`). This phase provides the behavior
that reads and mutates them. Stack and lookup functions operate on the current
context (`eli_get_current_context()`), so a context must exist before they are
called; the theme setters can also target a caller-provided `eli_style`.

---

## 1. Themes

Three built-in themes fill a style's `colors[ELI_COL_*]` table with the exact
Dear ImGui RGBA values (including the `ImLerp`-derived tab colors and the aliased
slots such as `Separator = Border`).

| Function | Theme |
|----------|-------|
| `void eli_style_colors_dark(eli_style *dst)` | Default dark theme. |
| `void eli_style_colors_light(eli_style *dst)` | Light theme. |
| `void eli_style_colors_classic(eli_style *dst)` | Classic (pre-1.60) theme. |

- **`dst`** — style to write into, or **`NULL`** to target the current context's
  style (`eli_get_style()`).
- Only the color table is written; sizing/spacing fields are left untouched
  (they are seeded with defaults by `eli_create_context`).

```c
eli_context *ctx = eli_create_context();
eli_style_colors_dark(NULL);          // theme the current context
```

---

## 2. Style color stack

Temporarily override a color slot; the matching pop restores the previous value
exactly. Backups are stored on `ctx->color_stack`.

| Function | Purpose |
|----------|---------|
| `void eli_push_style_color(eli_col idx, eli_col32 col)` | Override a slot from a packed color. |
| `void eli_push_style_color_vec4(eli_col idx, eli_vec4 col)` | Override a slot from a float RGBA color. |
| `void eli_pop_style_color(int count)` | Undo the last `count` overrides (clamped to depth). |

```c
eli_push_style_color(ELI_COL_TEXT, ELI_COL32(255, 0, 0, 255));
// ... draw red-text widgets ...
eli_pop_style_color(1);
```

---

## 3. Style variable stack

Temporarily override a sizing/spacing/alignment value. Each `eli_style_var` maps
to a field in `eli_style` and a kind (single `float` or `eli_vec2`). Pushing with
the wrong variant is **rejected** (a no-op), mirroring Dear ImGui's assert.

| Function | Purpose |
|----------|---------|
| `void eli_push_style_var(eli_style_var idx, float val)` | Override a single-float variable. |
| `void eli_push_style_var_vec2(eli_style_var idx, eli_vec2 val)` | Override a vec2 variable. |
| `void eli_pop_style_var(int count)` | Undo the last `count` overrides (clamped to depth). |

```c
eli_push_style_var(ELI_STYLE_VAR_ALPHA, 0.5f);
eli_push_style_var_vec2(ELI_STYLE_VAR_WINDOW_PADDING, eli_make_vec2(2, 2));
// ...
eli_pop_style_var(2);
```

---

## 4. Item flags

Per-item behavior flags (`eli_item_flags`, e.g. `ELI_ITEM_DISABLED`,
`ELI_ITEM_BUTTON_REPEAT`) drive `ctx->current_item_flags`, which widgets read.

| Function | Purpose |
|----------|---------|
| `void eli_push_item_flag(int option, bool enabled)` | Set (`enabled=true`) or clear (`enabled=false`) flag bits, saving the prior state. |
| `void eli_pop_item_flag(void)` | Restore the flags from the last push. |

```c
eli_push_item_flag(ELI_ITEM_DISABLED, true);
// ... widgets here are disabled ...
eli_pop_item_flag();
```

---

## 5. Color utilities

Resolve, convert, and name colors. The `get_color_u32` family applies the global
`style.alpha` so widgets fade consistently.

| Function | Purpose |
|----------|---------|
| `eli_col32 eli_get_color_u32(eli_col idx, float alpha_mul)` | Themed slot -> packed color, alpha scaled by `style.alpha * alpha_mul`. |
| `eli_col32 eli_get_color_u32_vec4(eli_vec4 col)` | Float color -> packed color, alpha scaled by `style.alpha`. |
| `eli_col32 eli_get_color_u32_col32(eli_col32 col, float alpha_mul)` | Rescale an existing packed color's alpha by `style.alpha * alpha_mul`. |
| `eli_vec4 eli_get_style_color_vec4(eli_col idx)` | Raw float color of a slot (no alpha scaling). |
| `const char *eli_get_style_color_name(eli_col idx)` | Slot name, e.g. `"TitleBgActive"` (`"Unknown"` if out of range). |
| `eli_vec4 eli_color_convert_u32_to_float4(eli_col32 in)` | Packed -> float RGBA. |
| `eli_col32 eli_color_convert_float4_to_u32(eli_vec4 in)` | Float RGBA -> packed (saturated). |
| `void eli_color_convert_rgb_to_hsv(float r,g,b, float *out_h,*out_s,*out_v)` | RGB -> HSV (all 0..1). |
| `void eli_color_convert_hsv_to_rgb(float h,s,v, float *out_r,*out_g,*out_b)` | HSV -> RGB (all 0..1). |

```c
eli_col32 text = eli_get_color_u32(ELI_COL_TEXT, 1.0f);

float h, s, v;
eli_color_convert_rgb_to_hsv(0.2f, 0.5f, 0.8f, &h, &s, &v);
```

**Note:** the HSV math uses local seam helpers (`eli_style_fabsf`/`eli_style_fmodf`)
rather than libm symbols, so it works unchanged in the wasm32/JAClibc build.

---

## 6. Files

| File | Contents |
|------|----------|
| `eli_style_colors.h` | `eli_style_colors_dark/_light/_classic` + the vec4 lerp helper. |
| `eli_style_stack.h` | Color/variable push-pop stacks and the style-var field mapping. |
| `eli_item_flags.h` | `eli_push_item_flag` / `eli_pop_item_flag`. |
| `eli_style_color_utils.h` | Color conversion, `get_color_u32` family, slot names. |
| `eli_style.h` | Aggregator that includes all of the above. |
