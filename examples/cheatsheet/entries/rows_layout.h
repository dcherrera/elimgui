/* rows_layout.h — "Layout" cheatsheet entries (fragment, no include guard).
 *
 * Included inside a `static const cheat_entry[]` initializer by cheat_entries.h
 * (or by the probe). render_layout.h MUST be included before this fragment so
 * the cheat_render_* symbols below are already declared.
 *
 * Field order: { category, title, api, description, snippet, render_fn }
 */

{
    "Layout",
    "Separator",
    "void eli_separator(void)",
    "Draws a full-width horizontal rule at the cursor and advances the layout"
    " by one pixel of thickness.",
    "eli_text(\"Above the line\");\n"
    "eli_separator();\n"
    "eli_text(\"Below the line\");",
    cheat_render_separator
},

{
    "Layout",
    "Same Line",
    "void eli_same_line(float offset_from_start_x, float spacing)",
    "Places the next item on the current row. Pass 0, -1 to follow the"
    " previous item with default item-spacing; pass an absolute x offset to"
    " pin to a column.",
    "eli_button(\"Alpha\");\n"
    "eli_same_line(0.0f, -1.0f);\n"
    "eli_button(\"Beta\");\n"
    "eli_same_line(0.0f, -1.0f);\n"
    "eli_button(\"Gamma\");",
    cheat_render_same_line
},

{
    "Layout",
    "Group",
    "void eli_begin_group(void)  /  void eli_end_group(void)",
    "Wraps a sequence of items into one bounding box so eli_same_line and"
    " last-item queries treat the block as a single unit.",
    "eli_begin_group();\n"
    "eli_button(\"Save\");\n"
    "eli_button(\"Cancel\");\n"
    "eli_end_group();\n"
    "eli_same_line(0.0f, 16.0f);\n"
    "eli_text(\"<-- one group\");",
    cheat_render_group
},

{
    "Layout",
    "Indent / Unindent",
    "void eli_indent(float indent_w)  /  void eli_unindent(float indent_w)",
    "Shifts the left content origin right (indent) or left (unindent) by"
    " indent_w pixels. Pass 0 to use the style indent_spacing.",
    "eli_text(\"No indent\");\n"
    "eli_indent(0.0f);\n"
    "eli_text(\"Indented once\");\n"
    "eli_indent(0.0f);\n"
    "eli_text(\"Indented twice\");\n"
    "eli_unindent(0.0f);\n"
    "eli_unindent(0.0f);\n"
    "eli_text(\"Back to baseline\");",
    cheat_render_indent
},

{
    "Layout",
    "Spacing",
    "void eli_spacing(void)",
    "Inserts one row of vertical item-spacing without drawing anything."
    " Stack multiple calls for larger gaps.",
    "eli_text(\"Row A\");\n"
    "eli_spacing();\n"
    "eli_spacing();\n"
    "eli_text(\"Row B  (two spacings gap above)\");",
    cheat_render_spacing
},

{
    "Layout",
    "Dummy",
    "void eli_dummy(eli_vec2 size)",
    "Reserves a pixel-sized invisible block in the layout. Useful as a"
    " fixed-width gap between same-line items or as vertical padding.",
    "eli_button(\"Left\");\n"
    "eli_same_line(0.0f, -1.0f);\n"
    "eli_dummy(eli_make_vec2(40.0f, 20.0f)); /* 40 px gap */\n"
    "eli_same_line(0.0f, -1.0f);\n"
    "eli_button(\"Right\");",
    cheat_render_dummy
},

{
    "Layout",
    "New Line",
    "void eli_new_line(void)",
    "Forces a line break, cancelling a pending eli_same_line and advancing"
    " the cursor to the next row.",
    "eli_button(\"A\");\n"
    "eli_same_line(0.0f, -1.0f); /* would put B on same row */\n"
    "eli_new_line();              /* cancel: B goes to next row */\n"
    "eli_button(\"B  (new line after A)\");",
    cheat_render_new_line
},

{
    "Layout",
    "Align Text to Frame Padding",
    "void eli_align_text_to_frame_padding(void)",
    "Raises the current line height so a plain text label placed with"
    " eli_same_line next to a framed widget shares the same text baseline.",
    "eli_align_text_to_frame_padding();\n"
    "eli_text(\"Label:\");\n"
    "eli_same_line(0.0f, -1.0f);\n"
    "eli_button(\"Action\");",
    cheat_render_align_text_to_frame_padding
},

{
    "Layout",
    "Push / Pop Item Width",
    "void eli_push_item_width(float w)  /  void eli_pop_item_width(void)",
    "Overrides the default framed-widget width for all subsequent widgets"
    " until eli_pop_item_width. Positive = absolute px; negative = offset"
    " from right edge; 0 = window default.",
    "static float v = 0.5f;\n"
    "eli_push_item_width(120.0f);\n"
    "eli_slider_float(\"Narrow (120 px)\", &v, 0.0f, 1.0f, \"%.2f\", 0);\n"
    "eli_pop_item_width();\n"
    "eli_slider_float(\"Default width\", &v, 0.0f, 1.0f, \"%.2f\", 0);",
    cheat_render_push_item_width
},

{
    "Layout",
    "Content Region Avail",
    "eli_vec2 eli_get_content_region_avail(void)",
    "Returns the remaining space from the cursor to the work-area right/bottom"
    " edge. Pass -FLT_MIN to eli_push_item_width to stretch a widget to fill"
    " the entire remaining row width.",
    "eli_vec2 avail = eli_get_content_region_avail();\n"
    "eli_text(\"avail.x = %.0f px\", avail.x);\n"
    "static float v = 0.5f;\n"
    "eli_push_item_width(-FLT_MIN); /* fill to right edge */\n"
    "eli_slider_float(\"##full\", &v, 0.0f, 1.0f, \"%.2f\", 0);\n"
    "eli_pop_item_width();",
    cheat_render_content_region_avail
},
