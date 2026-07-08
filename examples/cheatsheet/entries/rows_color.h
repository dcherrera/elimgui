/* rows_color.h — Color category cheat_entry initializers.
 * NO include guard: this file is a raw array-initializer fragment included
 * inside a cheat_entry[] definition.  render_color.h must be included first
 * so the render function names are in scope.
 *
 * Field order: category, title, api, description, snippet, render_fn
 */

{ "Color", "Color Edit (RGB)",
  "bool eli_color_edit3(label, float col[3], flags)",
  "Edit a 3-component RGB color. A small swatch opens a picker popup on click."
  " Alpha is always treated as 1.0.",
  "static float col[3] = {0.26f, 0.59f, 0.98f};\n"
  "eli_color_edit3(\"Background\", col, ELI_COLOR_EDIT_NONE);",
  cheat_render_color_edit3 },

{ "Color", "Color Edit (RGBA)",
  "bool eli_color_edit4(label, float col[4], flags)",
  "Edit a 4-component RGBA color with drag inputs and a preview swatch."
  " Click the swatch to open the full color picker popup.",
  "static float col[4] = {0.26f, 0.59f, 0.98f, 0.80f};\n"
  "eli_color_edit4(\"Tint\", col, ELI_COLOR_EDIT_NONE);",
  cheat_render_color_edit4_rgba },

{ "Color", "Color Picker (RGBA)",
  "bool eli_color_picker4(label, float col[4], flags, ref_col)",
  "Inline saturation/value square, a hue bar, and optional alpha bar and side"
  " preview swatch. Pass ELI_COLOR_EDIT_ALPHA_BAR for the alpha slider."
  " ref_col may be NULL.",
  "static float col[4] = {0.80f, 0.20f, 0.30f, 1.0f};\n"
  "eli_color_picker4(\"##picker\", col, ELI_COLOR_EDIT_NONE, NULL);",
  cheat_render_color_picker4 },

{ "Color", "Color Button",
  "bool eli_color_button(desc_id, eli_vec4 col, flags, eli_vec2 size)",
  "A clickable color swatch. Draws an alpha checkerboard when translucent."
  " Pass ELI_COLOR_EDIT_NO_BORDER to suppress the frame border."
  " Returns true on the frame it is pressed.",
  "eli_vec4 col = {0.4f, 0.7f, 0.2f, 1.0f};\n"
  "eli_vec2 sz  = {40.0f, 40.0f};\n"
  "if (eli_color_button(\"##swatch\", col, ELI_COLOR_EDIT_NONE, sz))\n"
  "    /* handle click */;",
  cheat_render_color_button },

{ "Color", "Color Edit - HSV + Alpha Bar",
  "bool eli_color_edit4(label, float col[4], DISPLAY_HSV | ALPHA_BAR)",
  "Display color components in HSV mode. ELI_COLOR_EDIT_ALPHA_BAR adds a"
  " vertical alpha slider inside the picker popup. Combine both flags to"
  " expose hue, saturation, value, and alpha in one widget.",
  "static float col[4] = {0.55f, 0.85f, 0.40f, 0.90f};\n"
  "eli_color_edit4(\"Accent\", col,\n"
  "    ELI_COLOR_EDIT_DISPLAY_HSV | ELI_COLOR_EDIT_ALPHA_BAR);",
  cheat_render_color_edit4_hsv },
