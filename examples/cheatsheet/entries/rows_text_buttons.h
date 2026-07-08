/* rows_text_buttons.h — fragment: no include guard.
 * #include'd inside a cheat_entry array initializer.
 * Field order: { category, title, api, description, snippet, render_fn }
 */

/* ---- Text ---- */

{ "Text", "Text",
  "void eli_text(const char *fmt, ...)",
  "Draw printf-style formatted text at the cursor. Advances the layout cursor.",
  "eli_text(\"Hello, %s!\", \"elimgui\");",
  cheat_render_tb_text },

{ "Text", "Text Colored",
  "void eli_text_colored(eli_vec4 col, const char *fmt, ...)",
  "Draw formatted text in an explicit RGBA color.",
  "eli_vec4 col = eli_make_vec4(0.2f, 0.85f, 0.4f, 1.0f);\n"
  "eli_text_colored(col, \"Colored text\");",
  cheat_render_tb_text_colored },

{ "Text", "Text Disabled",
  "void eli_text_disabled(const char *fmt, ...)",
  "Draw formatted text in the style's disabled/dim color (ELI_COL_TEXT_DISABLED).",
  "eli_text_disabled(\"Disabled (dimmed) text\");",
  cheat_render_tb_text_disabled },

{ "Text", "Text Wrapped",
  "void eli_text_wrapped(const char *fmt, ...)",
  "Draw formatted text with greedy word-wrap to the content region width.",
  "eli_text_wrapped(\"This long sentence wraps to the available width.\");",
  cheat_render_tb_text_wrapped },

{ "Text", "Label Text",
  "void eli_label_text(const char *label, const char *fmt, ...)",
  "Display a value on the right and a label on the left, aligned to the item width.",
  "eli_label_text(\"Status\", \"%s\", \"active\");",
  cheat_render_tb_label_text },

{ "Text", "Bullet Text",
  "void eli_bullet_text(const char *fmt, ...)",
  "Draw a bullet glyph followed by formatted text on the same line.",
  "eli_bullet_text(\"First item\");\n"
  "eli_bullet_text(\"Second item\");",
  cheat_render_tb_bullet_text },

{ "Text", "Bullet",
  "void eli_bullet(void)",
  "Draw a standalone bullet glyph; use eli_same_line to place content beside it.",
  "eli_bullet();\n"
  "eli_same_line(0.0f, -1.0f);\n"
  "eli_text(\"inline content\");",
  cheat_render_tb_bullet },

{ "Text", "Separator Text",
  "void eli_separator_text(const char *label)",
  "A horizontal rule with a left-aligned text label; mirrors ImGui SeparatorText.",
  "eli_separator_text(\"Section\");",
  cheat_render_tb_separator_text },

/* ---- Basic Widgets ---- */

{ "Basic Widgets", "Button",
  "bool eli_button(const char *label)",
  "Clickable labelled button sized to its text. Returns true on the frame pressed.",
  "if (eli_button(\"Click me\"))\n"
  "    clicks++;",
  cheat_render_tb_button },

{ "Basic Widgets", "Small Button",
  "bool eli_small_button(const char *label)",
  "Like eli_button but with no vertical frame padding; useful inline in text lines.",
  "if (eli_small_button(\"Small\"))\n"
  "    clicks++;",
  cheat_render_tb_small_button },

{ "Basic Widgets", "Arrow Button",
  "bool eli_arrow_button(const char *str_id, eli_dir dir)",
  "Square button drawing a directional arrow (ELI_DIR_LEFT/RIGHT/UP/DOWN).",
  "if (eli_arrow_button(\"##left\", ELI_DIR_LEFT))\n"
  "    val--;\n"
  "eli_same_line(0.0f, -1.0f);\n"
  "if (eli_arrow_button(\"##right\", ELI_DIR_RIGHT))\n"
  "    val++;",
  cheat_render_tb_arrow_button },

{ "Basic Widgets", "Invisible Button",
  "bool eli_invisible_button(const char *str_id, eli_vec2 size, eli_button_flags flags)",
  "A non-visual clickable region. Useful for custom widgets drawn with the draw list.",
  "if (eli_invisible_button(\"##inv\", eli_make_vec2(80.0f, 24.0f), ELI_BUTTON_NONE)) {\n"
  "    /* handle click */\n"
  "}",
  cheat_render_tb_invisible_button },

{ "Basic Widgets", "Checkbox",
  "bool eli_checkbox(const char *label, bool *v)",
  "Labelled checkbox bound to a bool. Returns true on the frame toggled.",
  "static bool enabled = true;\n"
  "eli_checkbox(\"Enable feature\", &enabled);",
  cheat_render_tb_checkbox },

{ "Basic Widgets", "Checkbox Flags (int)",
  "bool eli_checkbox_flags_int(const char *label, int *flags, int flags_value)",
  "A checkbox that sets/clears a single bit mask within an int flags field.",
  "static int flags = 0x01;\n"
  "eli_checkbox_flags_int(\"Bit 0\", &flags, 0x01);\n"
  "eli_checkbox_flags_int(\"Bit 2\", &flags, 0x04);",
  cheat_render_tb_checkbox_flags_int },

{ "Basic Widgets", "Radio Button",
  "bool eli_radio_button_int(const char *label, int *v, int v_button)",
  "Mutually exclusive radio buttons bound to an int selection value.",
  "static int choice = 0;\n"
  "eli_radio_button_int(\"Alpha\", &choice, 0);\n"
  "eli_same_line(0.0f, -1.0f);\n"
  "eli_radio_button_int(\"Beta\",  &choice, 1);\n"
  "eli_same_line(0.0f, -1.0f);\n"
  "eli_radio_button_int(\"Gamma\", &choice, 2);",
  cheat_render_tb_radio_button },

{ "Basic Widgets", "Progress Bar",
  "void eli_progress_bar(float fraction, eli_vec2 size_arg, const char *overlay)",
  "Horizontal bar showing fraction [0,1]. Pass NULL overlay for a \"NN%\" default.",
  "eli_progress_bar(0.6f, eli_make_vec2(-1.0f, 0.0f), NULL);",
  cheat_render_tb_progress_bar },

{ "Basic Widgets", "Text Link",
  "bool eli_text_link(const char *label)",
  "Clickable underlined text link. Returns true on click. Use eli_text_link_open_url to open URLs.",
  "if (eli_text_link(\"elimgui docs\"))\n"
  "    /* handle click */;",
  cheat_render_tb_text_link },
