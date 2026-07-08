/* rows_windows_dock.h — "Child Windows", "Disabling & Clipping", "Docking",
 * and "List Clipper" cheatsheet entry initializers (fragment, no include guard).
 *
 * Included inside a `static const cheat_entry[]` initializer by cheat_entries.h
 * (or by the probe). render_windows_dock.h MUST be included before this fragment
 * so the cheat_render_* identifiers are already declared.
 *
 * Field order: { category, title, api, description, snippet, render_fn }
 */

{
    "Child Windows",
    "Scrollable Child Window",
    "bool eli_begin_child(str_id, size, child_flags, window_flags)"
    " / void eli_end_child(void)",
    "Creates a bordered, scrollable sub-region inside the current window. A"
    " zero axis fills the remaining parent work area. eli_end_child must always"
    " be called regardless of the return value of eli_begin_child.",
    "eli_vec2 sz = eli_make_vec2(0.0f, 120.0f);\n"
    "if (eli_begin_child(\"##child\", sz, ELI_CHILD_BORDERS, ELI_WINDOW_NONE)) {\n"
    "    eli_text(\"Line 1\");\n"
    "    eli_text(\"Line 2\");\n"
    "    /* ... more items ... */\n"
    "}\n"
    "eli_end_child();",
    cheat_render_child_window
},

{
    "Disabling & Clipping",
    "Disable Scope",
    "void eli_begin_disabled(bool disabled) / void eli_end_disabled(void)",
    "Wraps any number of widgets in a disabled scope: they render at reduced"
    " alpha and reject all mouse and keyboard input. Nestable; an already-"
    "disabled outer scope keeps widgets disabled even if an inner call passes"
    " false.",
    "eli_begin_disabled(true);\n"
    "if (eli_button(\"Disabled Button\"))\n"
    "    /* never fires */;\n"
    "static bool flag = true;\n"
    "eli_checkbox(\"Disabled Check\", &flag);\n"
    "eli_end_disabled();",
    cheat_render_disabled_scope
},

{
    "Disabling & Clipping",
    "Clip Rect",
    "void eli_push_clip_rect(clip_min, clip_max, intersect_with_current)"
    " / void eli_pop_clip_rect(void)",
    "Scissors the draw list and the window item-visibility rect to a screen-"
    "space bounding box. Pass intersect_with_current=true to AND with the"
    " existing clip (safe inside child windows); false replaces it entirely.",
    "eli_vec2 p  = eli_get_cursor_screen_pos();\n"
    "eli_vec2 mn = p;\n"
    "eli_vec2 mx = eli_make_vec2(p.x + 90.0f, p.y + 40.0f);\n"
    "eli_push_clip_rect(mn, mx, /*intersect=*/true);\n"
    "eli_text(\"Clipped at 90 px wide\");\n"
    "eli_pop_clip_rect();",
    cheat_render_clip_rect
},

{
    "Docking",
    "Dock Space",
    "eli_id eli_dock_space(eli_id id, eli_vec2 size, int flags)",
    "Reserves a rectangular region in the current window as a docking target."
    " Windows can be dragged into this region or programmatically docked via"
    " eli_set_next_window_dock_id. A zero size axis fills the available content"
    " region. Returns the dock-space node id (same as id).\n"
    "Note: live render shows an empty dock-space region inside the card.",
    "/* Each frame, inside your host window: */\n"
    "eli_id ds_id = eli_get_id(\"MyDockSpace\");\n"
    "eli_dock_space(ds_id, eli_make_vec2(0.0f, 0.0f), 0);\n"
    "\n"
    "/* Dock a window into the space on first use: */\n"
    "eli_set_next_window_dock_id(ds_id, ELI_COND_FIRST_USE_EVER);\n"
    "eli_begin(\"Docked Window\", NULL, ELI_WINDOW_NONE);\n"
    "eli_text(\"I am docked!\");\n"
    "eli_end();",
    cheat_render_dock_space
},

{
    "Docking",
    "Set Next Window Dock ID",
    "void eli_set_next_window_dock_id(eli_id dock_id, eli_cond cond)",
    "Requests that the next eli_begin attach the window to an existing dock"
    " node. Use ELI_COND_FIRST_USE_EVER to dock only on first appearance, or"
    " ELI_COND_ALWAYS to re-dock every frame. Pass dock_id=0 to detach"
    " (float) the window.",
    "eli_id ds_id = eli_get_id(\"MyDockSpace\");\n"
    "\n"
    "/* Dock on first appearance only: */\n"
    "eli_set_next_window_dock_id(ds_id, ELI_COND_FIRST_USE_EVER);\n"
    "eli_begin(\"My Window\", NULL, ELI_WINDOW_NONE);\n"
    "/* ... */\n"
    "eli_end();\n"
    "\n"
    "/* Query docking state from inside a window: */\n"
    "bool   docked   = eli_is_window_docked();\n"
    "eli_id cur_id   = eli_get_window_dock_id(); /* 0 if floating */",
    cheat_render_set_next_dock_id
},

{
    "List Clipper",
    "List Clipper",
    "eli_list_clipper_begin / eli_list_clipper_step / eli_list_clipper_end",
    "Efficiently renders large uniform-height item lists: only items visible in"
    " the scroll region are submitted each frame. The cursor is seeked past"
    " clipped ranges so the scroll bar reflects the full list height. Use"
    " inside a scrollable child window or any scrolling region.",
    "eli_list_clipper clipper;\n"
    "eli_list_clipper_begin(&clipper, 1000, -1.0f); /* -1 = auto-measure */\n"
    "while (eli_list_clipper_step(&clipper)) {\n"
    "    for (int i = clipper.display_start; i < clipper.display_end; i++)\n"
    "        eli_text(\"Item %04d\", i);\n"
    "}\n"
    "eli_list_clipper_end(&clipper);",
    cheat_render_list_clipper
},
