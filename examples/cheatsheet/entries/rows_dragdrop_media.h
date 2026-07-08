/* rows_dragdrop_media.h
 * Fragment — no include guard. Include inside a cheat_entry array initializer.
 * Field order: { category, title, api, description, snippet, render }.
 * All entries are comma-terminated (C11 trailing comma is valid).
 */

{ "Drag & Drop", "Drag & Drop Source -> Target",
  "eli_begin_drag_drop_source(flags) / eli_set_drag_drop_payload(type,data,sz,cond)\n"
  "eli_end_drag_drop_source() / eli_begin_drag_drop_target()\n"
  "eli_accept_drag_drop_payload(type, flags) / eli_end_drag_drop_target()",
  "Left-drag \"Drag me\" onto \"Drop here\" to transfer an int payload. "
  "The received value is shown below the buttons.",
  "eli_button(\"Drag me\");\n"
  "if (eli_begin_drag_drop_source(ELI_DRAG_DROP_NONE)) {\n"
  "    eli_set_drag_drop_payload(\"INT\", &val, sizeof(val), ELI_COND_ALWAYS);\n"
  "    eli_end_drag_drop_source();\n"
  "}\n"
  "eli_button(\"Drop here\");\n"
  "if (eli_begin_drag_drop_target()) {\n"
  "    const eli_payload *p =\n"
  "        eli_accept_drag_drop_payload(\"INT\", ELI_DRAG_DROP_NONE);\n"
  "    if (p != NULL && p->data_size == (int)sizeof(int))\n"
  "        received = *(const int *)p->data;\n"
  "    eli_end_drag_drop_target();\n"
  "}",
  cheat_render_drag_drop_demo },

{ "Images", "Image",
  "void eli_image(user_texture_id, image_size, uv0, uv1, tint_col, border_col)",
  "Display a textured rectangle. Pass zero-alpha border_col to suppress the border. "
  "Tex id 1 shown here is the font atlas set by eli_font_atlas_set_tex_id(g_atlas, 1) "
  "in js_start; substitute your own backend texture id for real image content.",
  "/* tex id 1 = font atlas (placeholder; use your own texture id) */\n"
  "eli_image(1u,\n"
  "          eli_make_vec2(64.0f, 64.0f),\n"
  "          eli_make_vec2(0.0f, 0.0f),              /* uv0 */\n"
  "          eli_make_vec2(1.0f, 1.0f),              /* uv1 */\n"
  "          eli_make_vec4(1.0f, 1.0f, 1.0f, 1.0f), /* tint: white */\n"
  "          eli_make_vec4(1.0f, 1.0f, 1.0f, 0.5f)); /* border */",
  cheat_render_image },

{ "Images", "Image Button",
  "bool eli_image_button(str_id, user_texture_id, image_size, uv0, uv1, bg_col, tint_col)",
  "A clickable button framed around a texture; returns true when pressed. "
  "Tex id 1 shown here is the font atlas (placeholder; see eli_image entry).",
  "static int clicks = 0;\n"
  "if (eli_image_button(\"##img\", 1u,\n"
  "                     eli_make_vec2(64.0f, 64.0f),\n"
  "                     eli_make_vec2(0.0f, 0.0f),\n"
  "                     eli_make_vec2(1.0f, 1.0f),\n"
  "                     eli_make_vec4(0.0f, 0.0f, 0.0f, 0.0f), /* no bg */\n"
  "                     eli_make_vec4(1.0f, 1.0f, 1.0f, 1.0f))) /* tint */\n"
  "    clicks++;",
  cheat_render_image_button },

{ "Plots", "Plot Lines",
  "void eli_plot_lines(label, values, values_count, values_offset, overlay_text,\n"
  "                    scale_min, scale_max, graph_size, stride)",
  "Line graph from a contiguous float array. Pass ELI_PLOT_SCALE_AUTO for "
  "scale_min/max to auto-range from data. Hover to see per-sample values inline.",
  "static const float sine[32] = {\n"
  "     0.000f,  0.195f,  0.383f,  0.556f,  0.707f,  0.831f,  0.924f,  0.981f,\n"
  "     1.000f,  0.981f,  0.924f,  0.831f,  0.707f,  0.556f,  0.383f,  0.195f,\n"
  "     0.000f, -0.195f, -0.383f, -0.556f, -0.707f, -0.831f, -0.924f, -0.981f,\n"
  "    -1.000f, -0.981f, -0.924f, -0.831f, -0.707f, -0.556f, -0.383f, -0.195f\n"
  "};\n"
  "eli_plot_lines(\"Sine##lines\", sine, 32, 0,\n"
  "               NULL, -1.1f, 1.1f,\n"
  "               eli_make_vec2(0.0f, 0.0f), 0);",
  cheat_render_plot_lines },

{ "Plots", "Plot Histogram",
  "void eli_plot_histogram(label, values, values_count, values_offset, overlay_text,\n"
  "                        scale_min, scale_max, graph_size, stride)",
  "Vertical-bar histogram. Each bar spans 1/N of the frame width. "
  "Hovered bar is drawn in ELI_COL_PLOT_HISTOGRAM_HOVERED.",
  "static const float dist[10] = {\n"
  "    0.10f, 0.30f, 0.60f, 0.90f, 1.00f,\n"
  "    0.80f, 0.50f, 0.20f, 0.10f, 0.05f\n"
  "};\n"
  "eli_plot_histogram(\"Dist##hist\", dist, 10, 0,\n"
  "                   NULL, 0.0f, 1.1f,\n"
  "                   eli_make_vec2(0.0f, 0.0f), 0);",
  cheat_render_plot_histogram },

{ "Value", "Value (bool)",
  "void eli_value_bool(const char *prefix, bool b)",
  "Display a bool as \"prefix: true\" or \"prefix: false\". "
  "Toggle the checkbox to see the output update.",
  "static bool active = true;\n"
  "eli_value_bool(\"Active\", active);\n"
  "eli_checkbox(\"##toggle\", &active);",
  cheat_render_value_bool },

{ "Value", "Value (int)",
  "void eli_value_int(const char *prefix, int v)",
  "Display a signed integer as \"prefix: <v>\". "
  "Drag the control below to change the displayed value.",
  "static int count = 7;\n"
  "eli_value_int(\"Count\", count);\n"
  "eli_drag_int(\"##n\", &count, 1.0f, 0, 100, \"%d\", 0);",
  cheat_render_value_int },

{ "Value", "Value (float)",
  "void eli_value_float(const char *prefix, float v, const char *float_format)",
  "Display a float as \"prefix: <v>\". Pass NULL for float_format to use \"%.3f\". "
  "Drag the control to update; two format variants are shown.",
  "eli_value_float(\"Pi\", 3.14159f, NULL);     /* -> \"Pi: 3.142\" */\n"
  "eli_value_float(\"Pi\", 3.14159f, \"%.5f\"); /* -> \"Pi: 3.14159\" */\n"
  "/* with a live variable: */\n"
  "static float v = 3.14159f;\n"
  "eli_value_float(\"val\", v, NULL);\n"
  "eli_drag_float(\"##v\", &v, 0.01f, 0.0f, 10.0f, \"%.3f\", 0);",
  cheat_render_value_float },
