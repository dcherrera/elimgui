# Color Widgets (Phase 13)

Color editing and picking, mirroring Dear ImGui's `ColorEdit` / `ColorPicker` /
`ColorButton` family. Colors are stored as RGBA floats in the `0..1` range.

All widgets live in `include/eli/widgets/eli_color.h` and are usable via a direct
include:

```c
#include <eli/widgets/eli_color.h>
```

## Overview

| Function | Purpose |
|----------|---------|
| `eli_color_button` | A clickable color swatch (with alpha checkerboard + border). |
| `eli_color_edit3` / `eli_color_edit4` | A numeric edit row + small preview that opens a picker popup. |
| `eli_color_picker3` / `eli_color_picker4` | The full interactive picker (SV square, hue bar, alpha bar, side preview). |
| `eli_set_color_edit_options` | Set the default option bits merged into every color widget. |

The picker's building blocks (all rendered by `eli_color_picker4`):

- **Saturation/Value square** — click/drag to set S (x-axis) and V (y-axis) for the
  current hue.
- **Hue bar** — a vertical 6-segment rainbow; click/drag to set the hue.
- **Alpha bar** — optional (enable with `ELI_COLOR_EDIT_ALPHA_BAR`); a checkerboard +
  gradient; click/drag to set alpha.
- **Preview square** — the current color swatch (side preview unless
  `ELI_COLOR_EDIT_NO_SIDE_PREVIEW`).

## Flags (`eli_color_edit_flags`)

Defined in this header (guarded, so it is a no-op if a core enum already provides
them). Notable bits:

- Display space: `ELI_COLOR_EDIT_DISPLAY_RGB`, `_DISPLAY_HSV`, `_DISPLAY_HEX`.
- Data type: `ELI_COLOR_EDIT_UINT8` (0..255), `ELI_COLOR_EDIT_FLOAT` (0..1).
- Picker type: `ELI_COLOR_EDIT_PICKER_HUE_BAR` (the implemented one).
- Input space: `ELI_COLOR_EDIT_INPUT_RGB`, `_INPUT_HSV`.
- Toggles: `_NO_ALPHA`, `_NO_PICKER`, `_NO_INPUTS`, `_NO_SMALL_PREVIEW`,
  `_NO_SIDE_PREVIEW`, `_NO_LABEL`, `_NO_BORDER`, `_ALPHA_BAR`.

When a widget leaves an option group unset, the group is filled from the module
defaults (see `eli_set_color_edit_options`). The built-in defaults are
`UINT8 | DISPLAY_RGB | INPUT_RGB | PICKER_HUE_BAR`.

## API

### `bool eli_color_button(const char *desc_id, eli_vec4 col, eli_color_edit_flags flags, eli_vec2 size)`

Draws a clickable swatch of `col`. An alpha checkerboard shows through when the color
is translucent; a border is drawn unless `ELI_COLOR_EDIT_NO_BORDER`. `size` of `0`
falls back to one frame height. Returns `true` on the frame it is clicked.

### `bool eli_color_edit4(const char *label, float col[4], eli_color_edit_flags flags)`

Renders numeric component drags (RGB or HSV per the display flag, or a `#RRGGBBAA`
hex field for `DISPLAY_HEX`), a small preview swatch that opens a picker popup on
click, and a trailing label. Writes edits back into `col` and calls
`eli_mark_item_edited`. Returns `true` when the color changed this frame.
`eli_color_edit3` is the alpha-less wrapper over a `float[3]`.

> Note: with `ELI_COLOR_EDIT_UINT8` (the default), editing any component quantizes
> every component to the nearest `1/255` step — matching Dear ImGui.

### `bool eli_color_picker4(const char *label, float col[4], eli_color_edit_flags flags, const float *ref_col)`

The full picker. Handles SV-square, hue-bar and (optional) alpha-bar interaction,
renders the swatches/cursors, and — unless `ELI_COLOR_EDIT_NO_INPUTS` — a numeric
edit row beneath the square. Hue is preserved across gray columns (where RGB→HSV
cannot recover it) via a per-picker cache. `ref_col` is accepted for API parity and
currently unused. Returns `true` when the color changed. `eli_color_picker3` is the
alpha-less wrapper.

### `void eli_set_color_edit_options(eli_color_edit_flags flags)`

Overrides the default option bits (display / data type / picker / input group) used
when a widget leaves them unspecified.

## Usage

```c
#include <eli/widgets/eli_color.h>

static float bg[4] = { 0.10f, 0.35f, 0.80f, 1.00f };
static float tint[3] = { 1.0f, 0.5f, 0.2f };

void draw_settings(void)
{
    eli_begin("Settings", NULL, 0);

    /* Edit row with a preview swatch that opens a picker popup. */
    eli_color_edit4("Background", bg, ELI_COLOR_EDIT_ALPHA_BAR);

    /* RGB-only editor. */
    eli_color_edit3("Tint", tint, 0);

    /* Standalone picker, HSV inputs, no side preview. */
    eli_color_picker4("Picker", bg,
                      ELI_COLOR_EDIT_DISPLAY_HSV | ELI_COLOR_EDIT_NO_SIDE_PREVIEW, NULL);

    /* A bare swatch button. */
    if (eli_color_button("swatch", eli_make_vec4(bg[0], bg[1], bg[2], bg[3]), 0,
                         eli_make_vec2(0.0f, 0.0f)))
        /* clicked */;

    eli_end();
}
```

## Gotchas

- Storage is always RGBA `0..1`. `DISPLAY_HSV` / `DISPLAY_HEX` only change how the
  values are shown and edited, not how they are stored.
- The hex field commits on Enter (`ENTER_RETURNS_TRUE`) and ignores non-hex
  characters; a partial (<6 digit) value is not applied.
- The picker only implements the hue **bar** (`PICKER_HUE_BAR`); the hue wheel flag
  is accepted but falls back to the bar.
