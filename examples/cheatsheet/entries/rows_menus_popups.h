/* rows_menus_popups.h — cheat_entry initializers for Menus, Popups, Tooltips.
 * NO include guard: this file is a fragment #included inside an array literal.
 * Every row ends with a comma. Render functions are defined in
 * render_menus_popups.h which must be included before this fragment. */

/* ---- Menus ---- */

{ "Menus", "Window Menu Bar",
  "eli_begin_menu_bar() / eli_begin_menu(label, enabled) / eli_end_menu() / eli_end_menu_bar()",
  "Give a child window ELI_WINDOW_MENU_BAR, then emit the bar inside "
  "eli_begin_menu_bar / eli_end_menu_bar. Each eli_begin_menu opens a dropdown; "
  "eli_menu_item populates it.",
  "eli_vec2 sz = eli_make_vec2(-1.0f, 80.0f);\n"
  "if (eli_begin_child(\"##panel\", sz, ELI_CHILD_BORDERS,\n"
  "                    ELI_WINDOW_MENU_BAR)) {\n"
  "    if (eli_begin_menu_bar()) {\n"
  "        if (eli_begin_menu(\"File\", true)) {\n"
  "            eli_menu_item(\"New\",     NULL,     false, true);\n"
  "            eli_menu_item(\"Open...\", \"Ctrl+O\", false, true);\n"
  "            eli_end_menu();\n"
  "        }\n"
  "        eli_end_menu_bar();\n"
  "    }\n"
  "    /* content below the bar */\n"
  "}\n"
  "eli_end_child();",
  cheat_render_menu_bar },

{ "Menus", "Menu Item (shortcut + checked)",
  "bool eli_menu_item(label, shortcut, selected, enabled)",
  "Inside an open eli_begin_menu, emit a selectable row. shortcut is displayed "
  "right-aligned (display only — you wire the actual key yourself). selected draws "
  "a check mark. Returns true on activation; you toggle the bool manually.",
  "static bool checked = false;\n"
  "/* inside eli_begin_menu body: */\n"
  "if (eli_menu_item(\"Copy\", \"Ctrl+C\", checked, true))\n"
  "    checked = !checked;",
  cheat_render_menu_item_checked },

{ "Menus", "Menu Item (bool toggle)",
  "bool eli_menu_item_bool(label, shortcut, bool *p_selected, enabled)",
  "Like eli_menu_item but takes a bool pointer: it reads selected for the check "
  "mark and writes the toggled value back on activation. Removes the manual "
  "if/toggle from the call site.",
  "static bool dark_mode = true;\n"
  "/* inside eli_begin_menu body: */\n"
  "eli_menu_item_bool(\"Dark Mode\", NULL, &dark_mode, true);",
  cheat_render_menu_item_bool },

/* ---- Popups ---- */

{ "Popups", "Popup (button open)",
  "eli_open_popup(str_id, flags) / eli_begin_popup(str_id, flags) / eli_end_popup()",
  "Call eli_open_popup when the trigger fires (e.g. button click), then call "
  "eli_begin_popup every frame using the same str_id. The popup window appears "
  "at the mouse position and closes on click-outside or Escape.",
  "if (eli_button(\"Options...\"))\n"
  "    eli_open_popup(\"##opts\", ELI_POPUP_NONE);\n"
  "if (eli_begin_popup(\"##opts\", ELI_WINDOW_NONE)) {\n"
  "    if (eli_menu_item(\"Cut\",   \"Ctrl+X\", false, true)) { /* ... */ }\n"
  "    if (eli_menu_item(\"Copy\",  \"Ctrl+C\", false, true)) { /* ... */ }\n"
  "    if (eli_menu_item(\"Paste\", \"Ctrl+V\", false, true)) { /* ... */ }\n"
  "    eli_end_popup();\n"
  "}",
  cheat_render_open_popup },

{ "Popups", "Context Popup (right-click item)",
  "bool eli_begin_popup_context_item(str_id, flags)",
  "Combines the open and begin: right-clicking (or the button selected by the "
  "low flag bits) the previous item triggers the open; the popup is begun on the "
  "same call. No explicit eli_open_popup needed.",
  "eli_text(\"[Right-click me]\");\n"
  "if (eli_begin_popup_context_item(\"##ctx\",\n"
  "        ELI_POPUP_MOUSE_BUTTON_RIGHT)) {\n"
  "    if (eli_menu_item(\"Inspect\", NULL, false, true)) { /* ... */ }\n"
  "    if (eli_menu_item(\"Delete\",  NULL, false, true)) { /* ... */ }\n"
  "    eli_end_popup();\n"
  "}",
  cheat_render_context_item },

{ "Popups", "Popup Modal",
  "eli_open_popup(name, flags) / eli_begin_popup_modal(name, p_open, flags) / eli_end_popup()",
  "A modal dims and blocks all windows behind it. Open with eli_open_popup using "
  "the modal's name, then begin with eli_begin_popup_modal every frame. Close "
  "from inside with eli_close_current_popup.",
  "if (eli_button(\"Delete item...\"))\n"
  "    eli_open_popup(\"Confirm Delete##modal\", ELI_POPUP_NONE);\n"
  "if (eli_begin_popup_modal(\"Confirm Delete##modal\", NULL,\n"
  "                          ELI_WINDOW_NONE)) {\n"
  "    eli_text(\"Delete this item? This cannot be undone.\");\n"
  "    eli_spacing();\n"
  "    if (eli_button(\"OK\"))     eli_close_current_popup();\n"
  "    eli_same_line(0.0f, -1.0f);\n"
  "    if (eli_button(\"Cancel\")) eli_close_current_popup();\n"
  "    eli_end_popup();\n"
  "}",
  cheat_render_popup_modal },

/* ---- Tooltips ---- */

{ "Tooltips", "Tooltip (instant)",
  "void eli_set_tooltip(const char *fmt, ...)",
  "Manually check eli_is_item_hovered then call eli_set_tooltip for an immediate "
  "tooltip with no hover delay. eli_set_tooltip opens a tooltip window, emits the "
  "formatted text, and closes it — all in one call.",
  "eli_button(\"Hover me\");\n"
  "if (eli_is_item_hovered(ELI_HOVERED_NONE))\n"
  "    eli_set_tooltip(\"Tooltip text: %s\", value);",
  cheat_render_set_tooltip },

{ "Tooltips", "Item Tooltip (delayed, one-shot)",
  "void eli_set_item_tooltip(const char *fmt, ...)",
  "Like eli_set_tooltip but automatically gates on the style hover delay "
  "(hover_flags_for_tooltip_mouse). No manual is_item_hovered check needed; "
  "just call it right after the widget you want to describe.",
  "eli_button(\"Hover (with delay)\");\n"
  "eli_set_item_tooltip(\"Appears after the style hover delay\");",
  cheat_render_set_item_tooltip },

{ "Tooltips", "Rich Tooltip (begin/end)",
  "bool eli_begin_item_tooltip(void) / eli_end_tooltip(void)",
  "eli_begin_item_tooltip returns true once the previous item has been hovered "
  "past the style delay. Inside the scope you can emit any widget — text, images, "
  "tables. Balance with eli_end_tooltip only when it returns true.",
  "eli_button(\"Rich tooltip\");\n"
  "if (eli_begin_item_tooltip()) {\n"
  "    eli_text(\"Tooltip Title\");\n"
  "    eli_text_disabled(\"(any widgets here)\");\n"
  "    eli_end_tooltip();\n"
  "}",
  cheat_render_begin_item_tooltip },
