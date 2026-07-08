/**
 * @file cheat_entries.h
 * @brief Starter aggregated entry list for the elimgui visual cheatsheet.
 *
 * This is the content registry the app renders. Each `cheat_entry` pairs a live
 * widget renderer with the exact eli_*() call and a copyable snippet. The list
 * below is a representative STARTER set spanning several categories; it exercises
 * every engine feature (search, category filter, live widget, snippet, copy).
 *
 * ---------------------------------------------------------------------------
 * HOW TO ADD ENTRIES (for content agents)
 * ---------------------------------------------------------------------------
 * 1. Write a `static void cheat_render_<name>(void)` function in the "Live
 *    widget renderers" section below. It draws ONE live widget and owns its own
 *    function-static state (retained across frames, immediate-mode style).
 * 2. Append a `CHEAT_ENTRY(...)` initializer to the `cheat_all_entries[]` array,
 *    inside the block marked `>>> APPEND NEW ENTRIES BELOW <<<`. Fill in
 *    category, title, api, description, snippet, and your render function.
 * 3. Group entries by `category`: the left pane lists distinct categories in
 *    first-seen order, so keep same-category entries adjacent for a tidy menu.
 * No other file needs editing — the app passes this array + count to cheat_show.
 *
 * @status Cheatsheet content registry (stage 1 starter set). Not library API.
 * @issues None
 * @todo Content agents extend cheat_all_entries[] with the full widget set.
 */
#ifndef CHEAT_ENTRIES_H
#define CHEAT_ENTRIES_H

#include <eli/elimgui.h>

#include "cheat_engine.h"

/* Convenience initializer so appended rows read consistently. */
#define CHEAT_ENTRY(cat, title, api, desc, snippet, fn) \
    { (cat), (title), (api), (desc), (snippet), (fn) }

/* ---------------------------------------------------------------------------
 * Live widget renderers
 *
 * Each function renders exactly one widget and owns its retained state. Keep the
 * body small; the visible result should match the entry's snippet.
 * ------------------------------------------------------------------------- */

/* --- Basic Widgets --- */

static void cheat_render_button(void)
{
    static int clicks = 0;
    if (eli_button("Click me"))
        clicks++;
    eli_same_line(0.0f, -1.0f);
    eli_text("clicked %d", clicks);
}

static void cheat_render_checkbox(void)
{
    static bool enabled = true;
    eli_checkbox("Enable feature", &enabled);
}

/* --- Sliders & Drags --- */

static void cheat_render_slider_float(void)
{
    static float value = 0.42f;
    eli_slider_float("Amount", &value, 0.0f, 1.0f, "%.3f", 0);
}

static void cheat_render_drag_int(void)
{
    static int count = 8;
    eli_drag_int("Count", &count, 0.2f, 0, 100, "%d", 0);
}

/* --- Input Widgets --- */

static void cheat_render_input_text(void)
{
    static char buf[64] = "edit me";
    eli_input_text("Name", buf, sizeof(buf), 0, NULL, NULL);
}

static void cheat_render_input_double(void)
{
    static double value = 3.14159;
    eli_input_double("Number", &value, 0.01, 1.0, "%.5f", 0);
}

/* --- Color --- */

static void cheat_render_color_edit4(void)
{
    static float color[4] = {0.26f, 0.59f, 0.98f, 1.0f};
    eli_color_edit4("Tint", color, 0);
}

/* --- Trees & Tables --- */

static void cheat_render_tree_node(void)
{
    if (eli_tree_node("Root node")) {
        eli_bullet_text("Child A");
        eli_bullet_text("Child B");
        eli_tree_pop();
    }
}

static void cheat_render_table(void)
{
    eli_table_flags flags = ELI_TABLE_BORDERS | ELI_TABLE_ROW_BG;
    if (eli_begin_table("cheat_table", 3, flags)) {
        eli_table_setup_column("Name", 0, 0.0f, 0);
        eli_table_setup_column("Kind", 0, 0.0f, 0);
        eli_table_setup_column("Value", 0, 0.0f, 0);
        eli_table_headers_row();

        static const char *names[] = {"alpha", "beta", "gamma"};
        static const char *kinds[] = {"float", "int", "bool"};
        for (int row = 0; row < 3; row++) {
            eli_table_next_row();
            eli_table_next_column();
            eli_text("%s", names[row]);
            eli_table_next_column();
            eli_text("%s", kinds[row]);
            eli_table_next_column();
            eli_text("%.2f", (float)row * 1.5f + 0.25f);
        }
        eli_end_table();
    }
}

/* ---------------------------------------------------------------------------
 * Aggregated entry registry
 *
 * >>> APPEND NEW ENTRIES BELOW <<< (keep same-category rows adjacent)
 * ------------------------------------------------------------------------- */

static const cheat_entry cheat_all_entries[] = {
    CHEAT_ENTRY(
        "Basic Widgets", "Button",
        "bool eli_button(const char *label)",
        "A clickable button. Returns true on the frame it is pressed.",
        "if (eli_button(\"Click me\"))\n"
        "    clicks++;",
        cheat_render_button),

    CHEAT_ENTRY(
        "Basic Widgets", "Checkbox",
        "bool eli_checkbox(const char *label, bool *v)",
        "A boolean toggle bound to a bool. Returns true when the value changes.",
        "static bool enabled = true;\n"
        "eli_checkbox(\"Enable feature\", &enabled);",
        cheat_render_checkbox),

    CHEAT_ENTRY(
        "Sliders & Drags", "Slider (float)",
        "bool eli_slider_float(label, &v, min, max, fmt, flags)",
        "Drag a float between min and max. fmt is a printf format for the readout.",
        "static float value = 0.42f;\n"
        "eli_slider_float(\"Amount\", &value, 0.0f, 1.0f, \"%.3f\", 0);",
        cheat_render_slider_float),

    CHEAT_ENTRY(
        "Sliders & Drags", "Drag (int)",
        "bool eli_drag_int(label, &v, speed, min, max, fmt, flags)",
        "Click-drag to change an int; speed scales the per-pixel step.",
        "static int count = 8;\n"
        "eli_drag_int(\"Count\", &count, 0.2f, 0, 100, \"%d\", 0);",
        cheat_render_drag_int),

    CHEAT_ENTRY(
        "Input Widgets", "Input Text",
        "bool eli_input_text(label, buf, buf_size, flags, cb, ud)",
        "Single-line text edit into a caller-owned NUL-terminated buffer.",
        "static char buf[64] = \"edit me\";\n"
        "eli_input_text(\"Name\", buf, sizeof(buf), 0, NULL, NULL);",
        cheat_render_input_text),

    CHEAT_ENTRY(
        "Input Widgets", "Input Double",
        "bool eli_input_double(label, &v, step, step_fast, fmt, flags)",
        "Numeric field for a double with +/- step buttons.",
        "static double value = 3.14159;\n"
        "eli_input_double(\"Number\", &value, 0.01, 1.0, \"%.5f\", 0);",
        cheat_render_input_double),

    CHEAT_ENTRY(
        "Color", "Color Edit (RGBA)",
        "bool eli_color_edit4(label, float col[4], flags)",
        "Edit an RGBA color; click the swatch to open the full picker.",
        "static float color[4] = {0.26f, 0.59f, 0.98f, 1.0f};\n"
        "eli_color_edit4(\"Tint\", color, 0);",
        cheat_render_color_edit4),

    CHEAT_ENTRY(
        "Trees & Tables", "Tree Node",
        "bool eli_tree_node(const char *label) / eli_tree_pop()",
        "A collapsible node. Emit children while it returns true, then tree_pop.",
        "if (eli_tree_node(\"Root node\")) {\n"
        "    eli_bullet_text(\"Child A\");\n"
        "    eli_bullet_text(\"Child B\");\n"
        "    eli_tree_pop();\n"
        "}",
        cheat_render_tree_node),

    CHEAT_ENTRY(
        "Trees & Tables", "Table",
        "bool eli_begin_table(id, cols, flags) / eli_end_table()",
        "A multi-column table with headers, borders, and striped rows.",
        "if (eli_begin_table(\"t\", 3, ELI_TABLE_BORDERS | ELI_TABLE_ROW_BG)) {\n"
        "    eli_table_setup_column(\"Name\", 0, 0.0f, 0);\n"
        "    eli_table_headers_row();\n"
        "    eli_table_next_row();\n"
        "    eli_table_next_column();\n"
        "    eli_text(\"alpha\");\n"
        "    eli_end_table();\n"
        "}",
        cheat_render_table),

    /* >>> APPEND NEW ENTRIES ABOVE THIS LINE <<< */
};

/** Number of entries in cheat_all_entries. */
static const int cheat_all_entries_count =
    (int)(sizeof(cheat_all_entries) / sizeof(cheat_all_entries[0]));

#endif /* CHEAT_ENTRIES_H */
