/**
 * @file render_tables_tabs.h
 * @brief Live widget render functions for the "Tables" and "Tab Bars"
 *        cheatsheet entries. Each function owns its own function-static state
 *        so it can be called once per frame in immediate-mode style.
 *
 * Include this file before rows_tables_tabs.h so the render function
 * identifiers are in scope when the entry initializers are compiled.
 *
 * @status Cheatsheet content (Tables, Tab Bars). Not part of the library.
 * @issues None
 * @todo None
 */
#ifndef CHEAT_RENDER_TABLES_TABS_H
#define CHEAT_RENDER_TABLES_TABS_H

#include <eli/elimgui.h>

/* ---------------------------------------------------------------------------
 * Tables
 * ------------------------------------------------------------------------- */

/** Basic 3-column table with headers, borders, and alternating row shading. */
static void cheat_render_basic_table(void)
{
    eli_table_flags flags = ELI_TABLE_BORDERS | ELI_TABLE_ROW_BG;
    if (eli_begin_table("cs_tbl_basic", 3, flags)) {
        eli_table_setup_column("Name",  ELI_TABLE_COLUMN_NONE, 0.0f, 0);
        eli_table_setup_column("Type",  ELI_TABLE_COLUMN_NONE, 0.0f, 0);
        eli_table_setup_column("Value", ELI_TABLE_COLUMN_NONE, 0.0f, 0);
        eli_table_headers_row();

        static const char *s_names[] = { "alpha", "beta",  "gamma" };
        static const char *s_types[] = { "float", "int",   "bool"  };
        static const char *s_vals[]  = { "0.25",  "42",    "true"  };
        for (int r = 0; r < 3; r++) {
            eli_table_next_row();
            eli_table_next_column(); eli_text("%s", s_names[r]);
            eli_table_next_column(); eli_text("%s", s_types[r]);
            eli_table_next_column(); eli_text("%s", s_vals[r]);
        }
        eli_end_table();
    }
}

/** Sortable table — click a column header to sort ascending / descending. */
static void cheat_render_sortable_table(void)
{
    static struct { const char *name; int score; } s_rows[] = {
        { "Alice",   92 },
        { "Bob",     74 },
        { "Charlie", 88 },
    };
    static const int N = 3;

    eli_table_flags flags = ELI_TABLE_SORTABLE | ELI_TABLE_BORDERS | ELI_TABLE_ROW_BG;
    if (eli_begin_table("cs_tbl_sort", 2, flags)) {
        eli_table_setup_column("Name",  ELI_TABLE_COLUMN_NONE,                   0.0f, 0);
        eli_table_setup_column("Score", ELI_TABLE_COLUMN_PREFER_SORT_DESCENDING, 0.0f, 1);
        eli_table_headers_row();

        eli_table_sort_specs *specs = eli_table_get_sort_specs();
        if (specs && specs->specs_dirty && specs->specs_count > 0) {
            int col = specs->specs[0].column_index;
            eli_sort_direction dir = specs->specs[0].sort_direction;
            /* Bubble sort for demo — replace with your preferred algorithm. */
            for (int i = 0; i < N - 1; i++) {
                for (int j = 0; j < N - 1 - i; j++) {
                    int cmp = (col == 0)
                        ? strcmp(s_rows[j].name, s_rows[j + 1].name)
                        : (s_rows[j].score - s_rows[j + 1].score);
                    bool do_swap = (dir == ELI_SORT_ASCENDING) ? (cmp > 0) : (cmp < 0);
                    if (do_swap) {
                        const char *tmp_n  = s_rows[j].name;
                        int          tmp_s  = s_rows[j].score;
                        s_rows[j].name     = s_rows[j + 1].name;
                        s_rows[j].score    = s_rows[j + 1].score;
                        s_rows[j + 1].name  = tmp_n;
                        s_rows[j + 1].score = tmp_s;
                    }
                }
            }
        }

        for (int r = 0; r < N; r++) {
            eli_table_next_row();
            eli_table_next_column(); eli_text("%s", s_rows[r].name);
            eli_table_next_column(); eli_text("%d", s_rows[r].score);
        }
        eli_end_table();
    }
}

/** Row backgrounds — ELI_TABLE_ROW_BG plus a per-row color override on WARN rows. */
static void cheat_render_row_bg_table(void)
{
    eli_table_flags flags = ELI_TABLE_BORDERS | ELI_TABLE_ROW_BG;
    if (eli_begin_table("cs_tbl_rowbg", 2, flags)) {
        eli_table_setup_column("Item",   ELI_TABLE_COLUMN_NONE, 0.0f, 0);
        eli_table_setup_column("Status", ELI_TABLE_COLUMN_NONE, 0.0f, 0);
        eli_table_headers_row();

        static const char *s_items[]  = { "Widget A", "Widget B", "Widget C" };
        static const char *s_status[] = { "OK",       "WARN",     "OK"       };
        static const bool  s_warn[]   = { false,      true,       false      };
        for (int r = 0; r < 3; r++) {
            eli_table_next_row();
            if (s_warn[r]) {
                eli_col32 c = eli_get_color_u32(ELI_COL_HEADER_HOVERED, 0.4f);
                eli_table_set_bg_color(ELI_TABLE_BG_TARGET_ROW_BG1, c, -1);
            }
            eli_table_next_column(); eli_text("%s", s_items[r]);
            eli_table_next_column(); eli_text("%s", s_status[r]);
        }
        eli_end_table();
    }
}

/* ---------------------------------------------------------------------------
 * Tab Bars
 * ------------------------------------------------------------------------- */

/** Basic tab bar with three static tabs. */
static void cheat_render_tab_bar(void)
{
    if (eli_begin_tab_bar("cs_tabs_basic", ELI_TAB_BAR_NONE)) {
        if (eli_begin_tab_item("One", NULL, ELI_TAB_ITEM_NONE)) {
            eli_text("Content of tab One.");
            eli_end_tab_item();
        }
        if (eli_begin_tab_item("Two", NULL, ELI_TAB_ITEM_NONE)) {
            eli_text("Content of tab Two.");
            eli_end_tab_item();
        }
        if (eli_begin_tab_item("Three", NULL, ELI_TAB_ITEM_NONE)) {
            eli_text("Content of tab Three.");
            eli_end_tab_item();
        }
        eli_end_tab_bar();
    }
}

/** Tab bar where each tab shows a close button via p_open. Auto-resets for demo. */
static void cheat_render_tab_close(void)
{
    static bool s_open_a = true;
    static bool s_open_b = true;
    /* Auto-reset so the demo is always interactive. */
    if (!s_open_a && !s_open_b)
        s_open_a = s_open_b = true;

    if (eli_begin_tab_bar("cs_tabs_close", ELI_TAB_BAR_NONE)) {
        if (eli_begin_tab_item("Alpha", &s_open_a, ELI_TAB_ITEM_NONE)) {
            eli_text("Tab Alpha — click the x to close.");
            eli_end_tab_item();
        }
        if (eli_begin_tab_item("Beta", &s_open_b, ELI_TAB_ITEM_NONE)) {
            eli_text("Tab Beta — click the x to close.");
            eli_end_tab_item();
        }
        eli_end_tab_bar();
    }
}

#endif /* CHEAT_RENDER_TABLES_TABS_H */
