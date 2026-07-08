/**
 * @file eli_demo.h
 * @brief Showcase demo window (eli_show_demo_window) that exercises the public
 *        widget/layout/container API through collapsing-header sections, mirroring
 *        Dear ImGui's ShowDemoWindow.
 *
 * @status None
 * @issues None
 * @todo None
 */
#ifndef ELI_DEMO_ELI_DEMO_H
#define ELI_DEMO_ELI_DEMO_H

#include <eli/elimgui.h>

#include "eli_debug.h"

/* ---------------------------------------------------------------------------
 * File-static demo widget state. A demo needs somewhere to hold the values its
 * widgets read/write across frames; keeping it in one struct makes the sections
 * below pure UI composition with no hidden globals scattered around.
 * ------------------------------------------------------------------------- */

typedef struct eli_demo_state {
    /* Basic widgets. */
    bool        check;
    int         radio;
    int         combo_current;
    int         counter;
    int         clicked;
    float       color3[3];
    /* Input widgets. */
    char        text_buf[128];
    char        multiline_buf[512];
    float       input_f;
    int         input_i;
    /* Sliders & drags. */
    float       slider_f;
    int         slider_i;
    float       drag_f;
    int         drag_i;
    float       slider_angle;
    /* Color widgets. */
    float       color4[4];
    float       picker4[4];
    /* Tables. */
    bool        table_borders;
    /* Popups. */
    int         popup_selected;
    char        modal_confirmed;
    /* Drag & drop. */
    int         dnd_source_value;
    int         dnd_target_value;
    bool        initialized;
} eli_demo_state;

/**
 * Access the process-wide demo state, lazily seeding first-use defaults.
 *
 * @return  Pointer to the singleton demo state (never NULL).
 *
 * Thread-safe: no
 * Reentrant: yes
 */
static inline eli_demo_state *eli_demo__get_state(void)
{
    static eli_demo_state s = {0};
    if (!s.initialized) {
        s.initialized = true;
        s.combo_current = 0;
        s.slider_f = 0.5f;
        s.drag_f = 1.0f;
        s.input_f = 1.0f;
        s.color3[0] = 0.9f; s.color3[1] = 0.4f; s.color3[2] = 0.2f;
        s.color4[0] = 0.2f; s.color4[1] = 0.6f; s.color4[2] = 0.9f; s.color4[3] = 1.0f;
        s.picker4[0] = 0.4f; s.picker4[1] = 0.7f; s.picker4[2] = 0.3f; s.picker4[3] = 1.0f;
        s.table_borders = true;
        s.dnd_source_value = 42;
        s.dnd_target_value = 0;
        const char *hello = "edit me";
        for (int i = 0; hello[i] != '\0'; i++)
            s.text_buf[i] = hello[i];
    }
    return &s;
}

/* ---------------------------------------------------------------------------
 * Sections. Each renders one themed group of widgets and stays well under the
 * ~100-LOC-per-function target so they read as focused examples.
 * ------------------------------------------------------------------------- */

/** Text, buttons, checkbox, radio, combo, and a color button. */
static inline void eli_demo__basic_widgets(eli_demo_state *s)
{
    static const char *combo_items[] = {"Apple", "Banana", "Cherry", "Date"};

    eli_text("Basic widgets:");
    eli_text_colored(eli_make_vec4(1.0f, 0.8f, 0.2f, 1.0f), "Colored text");
    eli_text_disabled("Disabled text");
    eli_bullet_text("A bulleted line");

    if (eli_button("Click me"))
        s->clicked++;
    eli_same_line(0.0f, -1.0f);
    eli_text("clicked %d times", s->clicked);

    if (eli_small_button("counter -"))
        s->counter--;
    eli_same_line(0.0f, -1.0f);
    if (eli_small_button("counter +"))
        s->counter++;
    eli_same_line(0.0f, -1.0f);
    eli_text("= %d", s->counter);

    eli_checkbox("Enable feature", &s->check);
    eli_radio_button("Low", s->radio == 0);
    eli_same_line(0.0f, -1.0f);
    if (eli_radio_button("Medium", s->radio == 1)) s->radio = 1;
    eli_same_line(0.0f, -1.0f);
    if (eli_radio_button("High", s->radio == 2)) s->radio = 2;

    eli_combo("Fruit", &s->combo_current, combo_items, 4, -1);
    eli_color_edit3("A color", s->color3, 0);
    eli_progress_bar((float)((s->counter % 100) / 100.0f), eli_make_vec2(0.0f, 0.0f), NULL);
}

/** Same-line, groups, indent, separators, spacing. */
static inline void eli_demo__layout(void)
{
    eli_text("Three on a line:");
    eli_same_line(0.0f, -1.0f);
    eli_button("A");
    eli_same_line(0.0f, -1.0f);
    eli_button("B");
    eli_same_line(0.0f, -1.0f);
    eli_button("C");

    eli_separator_text("Groups");
    eli_begin_group();
    eli_text("Group 1");
    eli_button("G1 button");
    eli_end_group();
    eli_same_line(0.0f, 40.0f);
    eli_begin_group();
    eli_text("Group 2");
    eli_button("G2 button");
    eli_end_group();

    eli_separator_text("Indent");
    eli_text("Base level");
    eli_indent(0.0f);
    eli_text("Indented once");
    eli_indent(0.0f);
    eli_text("Indented twice");
    eli_unindent(0.0f);
    eli_unindent(0.0f);
    eli_spacing();
    eli_separator();
    eli_text("After a separator");
}

/** Text input (single/multi-line/hint) plus numeric input widgets. */
static inline void eli_demo__input_widgets(eli_demo_state *s)
{
    static char hint_buf[64] = {0};

    eli_input_text("Single line", s->text_buf, sizeof(s->text_buf), 0, NULL, NULL);
    eli_input_text_with_hint("With hint", "type here...", hint_buf, sizeof(hint_buf), 0, NULL, NULL);
    eli_input_text_multiline("Multi line", s->multiline_buf, sizeof(s->multiline_buf),
                             eli_make_vec2(0.0f, 60.0f), 0, NULL, NULL);
    eli_input_float("Input float", &s->input_f, 0.1f, 1.0f, "%.3f", 0);
    eli_input_int("Input int", &s->input_i, 1, 10, 0);
}

/** Slider and drag widgets across float/int/angle. */
static inline void eli_demo__sliders_drags(eli_demo_state *s)
{
    eli_slider_float("Slider float", &s->slider_f, 0.0f, 1.0f, "%.3f", 0);
    eli_slider_int("Slider int", &s->slider_i, 0, 100, "%d", 0);
    eli_slider_angle("Angle", &s->slider_angle, -360.0f, 360.0f, "%.0f deg", 0);
    eli_drag_float("Drag float", &s->drag_f, 0.01f, 0.0f, 10.0f, "%.3f", 0);
    eli_drag_int("Drag int", &s->drag_i, 1.0f, 0, 100, "%d", 0);
}

/** Color edit and picker widgets, including the alpha channel. */
static inline void eli_demo__color_widgets(eli_demo_state *s)
{
    eli_color_edit4("Color edit", s->color4, 0);
    eli_color_edit4("Color (HSV)", s->color4, ELI_COLOR_EDIT_DISPLAY_HSV);
    eli_color_button("A color button", eli_make_vec4(s->color4[0], s->color4[1],
                                                     s->color4[2], s->color4[3]), 0,
                     eli_make_vec2(0.0f, 0.0f));
    eli_color_picker4("Color picker", s->picker4, ELI_COLOR_EDIT_ALPHA_BAR, NULL);
}

/** Tree nodes and collapsing headers. */
static inline void eli_demo__trees(void)
{
    static const char child_ids[3] = {0, 1, 2};
    if (eli_tree_node("Root node")) {
        for (int i = 0; i < 3; i++) {
            if (eli_tree_node_ptr(&child_ids[i], "Child %d", i)) {
                eli_text("Leaf content for child %d", i);
                eli_tree_pop();
            }
        }
        eli_tree_pop();
    }
    if (eli_collapsing_header("A collapsing header", 0))
        eli_text("Header body content.");
}

/** A small sortable-looking table with borders toggle. */
static inline void eli_demo__tables(eli_demo_state *s)
{
    eli_checkbox("Borders", &s->table_borders);
    eli_table_flags flags = ELI_TABLE_ROW_BG | ELI_TABLE_RESIZABLE;
    if (s->table_borders)
        flags |= ELI_TABLE_BORDERS;
    if (eli_begin_table("demo_table", 3, flags)) {
        eli_table_setup_column("Name", 0, 0.0f, 0);
        eli_table_setup_column("Kind", 0, 0.0f, 0);
        eli_table_setup_column("Value", 0, 0.0f, 0);
        eli_table_headers_row();
        for (int row = 0; row < 4; row++) {
            eli_table_next_row();
            eli_table_set_column_index(0);
            eli_text("item_%d", row);
            eli_table_set_column_index(1);
            eli_text("row");
            eli_table_set_column_index(2);
            eli_text("%d", row * 10);
        }
        eli_end_table();
    }
}

/** A tab bar with a few tabs. */
static inline void eli_demo__tabs(void)
{
    if (eli_begin_tab_bar("demo_tabs", ELI_TAB_BAR_REORDERABLE)) {
        if (eli_begin_tab_item("Overview", NULL, 0)) {
            eli_text("Overview tab body.");
            eli_end_tab_item();
        }
        if (eli_begin_tab_item("Details", NULL, 0)) {
            eli_text("Details tab body.");
            eli_end_tab_item();
        }
        if (eli_begin_tab_item("Settings", NULL, 0)) {
            eli_text("Settings tab body.");
            eli_end_tab_item();
        }
        eli_end_tab_bar();
    }
}

/** Popups: a button-opened popup and a modal confirmation dialog. */
static inline void eli_demo__popups(eli_demo_state *s)
{
    static const char *choices[] = {"One", "Two", "Three"};

    if (eli_button("Open popup"))
        eli_open_popup("demo_popup", 0);
    if (eli_begin_popup("demo_popup", 0)) {
        eli_text("Pick one:");
        eli_separator();
        for (int i = 0; i < 3; i++)
            if (eli_selectable(choices[i], s->popup_selected == i, 0, eli_make_vec2(0.0f, 0.0f)))
                s->popup_selected = i;
        eli_end_popup();
    }
    eli_same_line(0.0f, -1.0f);
    eli_text("selected: %s", choices[s->popup_selected]);

    if (eli_button("Open modal"))
        eli_open_popup("demo_modal", 0);
    if (eli_begin_popup_modal("demo_modal", NULL, 0)) {
        eli_text("Confirm this action?");
        eli_separator();
        if (eli_button("OK")) {
            s->modal_confirmed = 1;
            eli_close_current_popup();
        }
        eli_same_line(0.0f, -1.0f);
        if (eli_button("Cancel"))
            eli_close_current_popup();
        eli_end_popup();
    }
}

/** Drag & drop: drag an integer payload from a source onto a target. */
static inline void eli_demo__drag_drop(eli_demo_state *s)
{
    eli_button("Drag source");
    if (eli_begin_drag_drop_source(0)) {
        eli_set_drag_drop_payload("DEMO_INT", &s->dnd_source_value,
                                  sizeof(s->dnd_source_value), 0);
        eli_text("Dragging %d", s->dnd_source_value);
        eli_end_drag_drop_source();
    }
    eli_same_line(0.0f, -1.0f);
    eli_button("Drop target");
    if (eli_begin_drag_drop_target()) {
        const eli_payload *p = eli_accept_drag_drop_payload("DEMO_INT", 0);
        if (p != NULL && p->data != NULL)
            s->dnd_target_value = *(const int *)p->data;
        eli_end_drag_drop_target();
    }
    eli_text("Target received: %d", s->dnd_target_value);
}

/* ---------------------------------------------------------------------------
 * Public demo entry point.
 * ------------------------------------------------------------------------- */

/**
 * Show the elimgui demo window: one large window whose collapsing-header
 * sections exercise the real widget/layout/container API. Intended as both a
 * feature showcase and a smoke test of the whole library.
 *
 * @param p_open  Optional visibility flag; when non-NULL a close button clears
 *                it and the window is skipped while false.
 *
 * Thread-safe: no
 * Reentrant: no
 */
static inline void eli_show_demo_window(bool *p_open)
{
    if (p_open != NULL && !*p_open)
        return;

    eli_demo_state *s = eli_demo__get_state();

    eli_set_next_window_size(eli_make_vec2(550.0f, 680.0f), ELI_COND_FIRST_USE_EVER);
    if (!eli_begin("Dear elimgui Demo", p_open, 0)) {
        eli_end();
        return;
    }

    eli_text("elimgui %s", eli_get_version());
    eli_separator();

    if (eli_collapsing_header("Basic widgets", 0))
        eli_demo__basic_widgets(s);
    if (eli_collapsing_header("Layout", 0))
        eli_demo__layout();
    if (eli_collapsing_header("Input widgets", 0))
        eli_demo__input_widgets(s);
    if (eli_collapsing_header("Sliders & Drags", 0))
        eli_demo__sliders_drags(s);
    if (eli_collapsing_header("Color widgets", 0))
        eli_demo__color_widgets(s);
    if (eli_collapsing_header("Trees & Collapsing", 0))
        eli_demo__trees();
    if (eli_collapsing_header("Tables", 0))
        eli_demo__tables(s);
    if (eli_collapsing_header("Tabs", 0))
        eli_demo__tabs();
    if (eli_collapsing_header("Popups & Modals", 0))
        eli_demo__popups(s);
    if (eli_collapsing_header("Drag & Drop", 0))
        eli_demo__drag_drop(s);
    if (eli_collapsing_header("Style Editor", 0))
        eli_show_style_editor(NULL);

    eli_end();
}

#endif /* ELI_DEMO_ELI_DEMO_H */
