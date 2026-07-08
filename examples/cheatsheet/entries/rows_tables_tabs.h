/* rows_tables_tabs.h — NO include guard; included as an initializer fragment.
 *
 * Field order: { category, title, api, description, snippet, render_fn }
 * Every entry is comma-terminated so the enclosing array needs no trailing
 * comma adjustment. Render functions are defined in render_tables_tabs.h,
 * which must be included before this file.
 */

/* ---- Tables --------------------------------------------------------------- */

{
    "Tables",
    "Basic Table",
    "eli_begin_table(id, cols, flags) / eli_table_setup_column / eli_table_headers_row",
    "A multi-column table with named headers, full borders, and alternating "
    "row shading. Call eli_table_setup_column once per column before the "
    "first row, then eli_table_headers_row to emit the header strip. Use "
    "eli_table_next_row and eli_table_next_column to advance through cells.",
    "eli_table_flags flags = ELI_TABLE_BORDERS | ELI_TABLE_ROW_BG;\n"
    "if (eli_begin_table(\"demo\", 3, flags)) {\n"
    "    eli_table_setup_column(\"Name\",  ELI_TABLE_COLUMN_NONE, 0.0f, 0);\n"
    "    eli_table_setup_column(\"Type\",  ELI_TABLE_COLUMN_NONE, 0.0f, 0);\n"
    "    eli_table_setup_column(\"Value\", ELI_TABLE_COLUMN_NONE, 0.0f, 0);\n"
    "    eli_table_headers_row();\n"
    "    for (int r = 0; r < ROWS; r++) {\n"
    "        eli_table_next_row();\n"
    "        eli_table_next_column(); eli_text(\"%s\", col0[r]);\n"
    "        eli_table_next_column(); eli_text(\"%s\", col1[r]);\n"
    "        eli_table_next_column(); eli_text(\"%s\", col2[r]);\n"
    "    }\n"
    "    eli_end_table();\n"
    "}",
    cheat_render_basic_table
},

{
    "Tables",
    "Sortable Table",
    "ELI_TABLE_SORTABLE + eli_table_get_sort_specs()",
    "Pass ELI_TABLE_SORTABLE to enable click-to-sort on column headers. "
    "After eli_table_headers_row, call eli_table_get_sort_specs() and check "
    "specs->specs_dirty each frame; when true, re-sort your data according to "
    "specs->specs[0].column_index and specs->specs[0].sort_direction.",
    "eli_table_flags flags =\n"
    "    ELI_TABLE_SORTABLE | ELI_TABLE_BORDERS | ELI_TABLE_ROW_BG;\n"
    "if (eli_begin_table(\"demo\", 2, flags)) {\n"
    "    eli_table_setup_column(\"Name\",\n"
    "        ELI_TABLE_COLUMN_NONE, 0.0f, 0);\n"
    "    eli_table_setup_column(\"Score\",\n"
    "        ELI_TABLE_COLUMN_PREFER_SORT_DESCENDING, 0.0f, 1);\n"
    "    eli_table_headers_row();\n"
    "\n"
    "    eli_table_sort_specs *specs = eli_table_get_sort_specs();\n"
    "    if (specs && specs->specs_dirty && specs->specs_count > 0) {\n"
    "        int col = specs->specs[0].column_index;\n"
    "        eli_sort_direction dir = specs->specs[0].sort_direction;\n"
    "        /* sort your data here */\n"
    "    }\n"
    "\n"
    "    for (int r = 0; r < ROWS; r++) {\n"
    "        eli_table_next_row();\n"
    "        eli_table_next_column(); eli_text(\"%s\", name[r]);\n"
    "        eli_table_next_column(); eli_text(\"%d\", score[r]);\n"
    "    }\n"
    "    eli_end_table();\n"
    "}",
    cheat_render_sortable_table
},

{
    "Tables",
    "Row Backgrounds & Borders",
    "ELI_TABLE_ROW_BG + eli_table_set_bg_color(target, color, col_n)",
    "ELI_TABLE_ROW_BG automatically alternates row shading using the theme "
    "colors ELI_COL_TABLE_ROW_BG and ELI_COL_TABLE_ROW_BG_ALT. To highlight "
    "individual rows call eli_table_set_bg_color with ELI_TABLE_BG_TARGET_ROW_BG0 "
    "or ROW_BG1 (an overlay layer) right after eli_table_next_row. "
    "Use ELI_TABLE_BORDERS for the full grid, or the individual "
    "BORDERS_INNER / BORDERS_OUTER flags for finer control.",
    "eli_table_flags flags = ELI_TABLE_BORDERS | ELI_TABLE_ROW_BG;\n"
    "if (eli_begin_table(\"demo\", 2, flags)) {\n"
    "    eli_table_setup_column(\"Item\",   ELI_TABLE_COLUMN_NONE, 0.0f, 0);\n"
    "    eli_table_setup_column(\"Status\", ELI_TABLE_COLUMN_NONE, 0.0f, 0);\n"
    "    eli_table_headers_row();\n"
    "    for (int r = 0; r < ROWS; r++) {\n"
    "        eli_table_next_row();\n"
    "        if (is_warning[r]) {\n"
    "            eli_col32 c =\n"
    "                eli_get_color_u32(ELI_COL_HEADER_HOVERED, 0.4f);\n"
    "            eli_table_set_bg_color(\n"
    "                ELI_TABLE_BG_TARGET_ROW_BG1, c, -1);\n"
    "        }\n"
    "        eli_table_next_column(); eli_text(\"%s\", items[r]);\n"
    "        eli_table_next_column(); eli_text(\"%s\", status[r]);\n"
    "    }\n"
    "    eli_end_table();\n"
    "}",
    cheat_render_row_bg_table
},

/* ---- Tab Bars ------------------------------------------------------------- */

{
    "Tab Bars",
    "Tab Bar (3 tabs)",
    "eli_begin_tab_bar(id, flags) / eli_begin_tab_item / eli_end_tab_item / eli_end_tab_bar",
    "A tab bar renders a horizontal strip of tabs above its contents. Call "
    "eli_begin_tab_bar once per frame (always pair with eli_end_tab_bar). For "
    "each tab call eli_begin_tab_item; it returns true only for the currently "
    "selected tab, so draw that tab's content and then call eli_end_tab_item.",
    "if (eli_begin_tab_bar(\"tabs\", ELI_TAB_BAR_NONE)) {\n"
    "    if (eli_begin_tab_item(\"One\", NULL, ELI_TAB_ITEM_NONE)) {\n"
    "        eli_text(\"Content of tab One.\");\n"
    "        eli_end_tab_item();\n"
    "    }\n"
    "    if (eli_begin_tab_item(\"Two\", NULL, ELI_TAB_ITEM_NONE)) {\n"
    "        eli_text(\"Content of tab Two.\");\n"
    "        eli_end_tab_item();\n"
    "    }\n"
    "    if (eli_begin_tab_item(\"Three\", NULL, ELI_TAB_ITEM_NONE)) {\n"
    "        eli_text(\"Content of tab Three.\");\n"
    "        eli_end_tab_item();\n"
    "    }\n"
    "    eli_end_tab_bar();\n"
    "}",
    cheat_render_tab_bar
},

{
    "Tab Bars",
    "Tab with Close Button",
    "eli_begin_tab_item(label, bool *p_open, flags)",
    "Pass a non-NULL bool pointer as p_open to show a close button on the "
    "tab. When the user clicks the x or middle-clicks the tab, the library "
    "sets *p_open to false and removes the tab on the next layout. Check "
    "*p_open before calling eli_begin_tab_item to avoid submitting a dead tab.",
    "static bool open_a = true;\n"
    "static bool open_b = true;\n"
    "if (eli_begin_tab_bar(\"tabs\", ELI_TAB_BAR_NONE)) {\n"
    "    if (eli_begin_tab_item(\"Alpha\", &open_a, ELI_TAB_ITEM_NONE)) {\n"
    "        eli_text(\"Alpha content.\");\n"
    "        eli_end_tab_item();\n"
    "    }\n"
    "    if (eli_begin_tab_item(\"Beta\", &open_b, ELI_TAB_ITEM_NONE)) {\n"
    "        eli_text(\"Beta content.\");\n"
    "        eli_end_tab_item();\n"
    "    }\n"
    "    eli_end_tab_bar();\n"
    "}",
    cheat_render_tab_close
},
