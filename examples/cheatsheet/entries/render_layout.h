/**
 * @file render_layout.h
 * @brief Live-widget renderers for the "Layout" cheatsheet entries.
 *
 * Each function demonstrates one layout primitive visually. Functions are
 * `static` so this header can be included from any translation unit without
 * symbol collisions. Retained widget state lives in function-static variables,
 * matching elimgui's immediate-mode model.
 *
 * Include this header before rows_layout.h (which references these symbols).
 *
 * @status Cheatsheet layout entries.
 * @issues None
 * @todo None
 */
#ifndef CHEAT_RENDER_LAYOUT_H
#define CHEAT_RENDER_LAYOUT_H

#include <eli/elimgui.h>

/* ---------------------------------------------------------------------------
 * eli_separator
 * ------------------------------------------------------------------------- */

/** Draws a horizontal rule between two text rows. */
static void cheat_render_separator(void)
{
    eli_text("Above the line");
    eli_separator();
    eli_text("Below the line");
}

/* ---------------------------------------------------------------------------
 * eli_same_line
 * ------------------------------------------------------------------------- */

/** Places three buttons on a single row to show horizontal flow. */
static void cheat_render_same_line(void)
{
    eli_button("Alpha");
    eli_same_line(0.0f, -1.0f);
    eli_button("Beta");
    eli_same_line(0.0f, -1.0f);
    eli_button("Gamma");
}

/* ---------------------------------------------------------------------------
 * eli_begin_group / eli_end_group
 * ------------------------------------------------------------------------- */

/**
 * Groups two buttons so they act as one bounding box, then places a label
 * same-line to the right of the whole group.
 */
static void cheat_render_group(void)
{
    eli_begin_group();
    eli_button("Save");
    eli_button("Cancel");
    eli_end_group();
    eli_same_line(0.0f, 16.0f);
    eli_text("<-- one group");
}

/* ---------------------------------------------------------------------------
 * eli_indent / eli_unindent
 * ------------------------------------------------------------------------- */

/** Steps through two levels of indent then returns to the baseline. */
static void cheat_render_indent(void)
{
    eli_text("No indent");
    eli_indent(0.0f);
    eli_text("Indented once");
    eli_indent(0.0f);
    eli_text("Indented twice");
    eli_unindent(0.0f);
    eli_unindent(0.0f);
    eli_text("Back to baseline");
}

/* ---------------------------------------------------------------------------
 * eli_spacing
 * ------------------------------------------------------------------------- */

/** Inserts two extra blank rows of vertical spacing between text items. */
static void cheat_render_spacing(void)
{
    eli_text("Row A");
    eli_spacing();
    eli_spacing();
    eli_text("Row B  (two spacings gap above)");
}

/* ---------------------------------------------------------------------------
 * eli_dummy
 * ------------------------------------------------------------------------- */

/**
 * Uses a dummy item as a fixed-width gap between two buttons on the same row.
 */
static void cheat_render_dummy(void)
{
    eli_button("Left");
    eli_same_line(0.0f, -1.0f);
    eli_dummy(eli_make_vec2(40.0f, 20.0f));
    eli_same_line(0.0f, -1.0f);
    eli_button("Right");
}

/* ---------------------------------------------------------------------------
 * eli_new_line
 * ------------------------------------------------------------------------- */

/**
 * Calls eli_same_line then immediately eli_new_line to show that new_line
 * cancels the pending same-line and forces the next item to a fresh row.
 */
static void cheat_render_new_line(void)
{
    eli_button("A");
    eli_same_line(0.0f, -1.0f);
    eli_new_line();
    eli_button("B  (new line after A)");
}

/* ---------------------------------------------------------------------------
 * eli_align_text_to_frame_padding
 * ------------------------------------------------------------------------- */

/**
 * Aligns a plain text label with the text inside the framed button next to it
 * so both baselines sit at the same height.
 */
static void cheat_render_align_text_to_frame_padding(void)
{
    eli_align_text_to_frame_padding();
    eli_text("Label:");
    eli_same_line(0.0f, -1.0f);
    eli_button("Action");
}

/* ---------------------------------------------------------------------------
 * eli_push_item_width / eli_pop_item_width
 * ------------------------------------------------------------------------- */

/** Narrows a slider to 120 px with push/pop, then shows the default width. */
static void cheat_render_push_item_width(void)
{
    static float v = 0.5f;
    eli_push_item_width(120.0f);
    eli_slider_float("Narrow (120 px)", &v, 0.0f, 1.0f, "%.2f", 0);
    eli_pop_item_width();
    eli_slider_float("Default width", &v, 0.0f, 1.0f, "%.2f", 0);
}

/* ---------------------------------------------------------------------------
 * eli_get_content_region_avail
 * ------------------------------------------------------------------------- */

/**
 * Queries the available content width, shows it as a label, then stretches a
 * slider to fill the remaining row width using the -FLT_MIN convention.
 */
static void cheat_render_content_region_avail(void)
{
    static float v = 0.5f;
    eli_vec2 avail = eli_get_content_region_avail();
    eli_text("avail.x = %.0f px", avail.x);
    eli_push_item_width(-FLT_MIN);
    eli_slider_float("##full", &v, 0.0f, 1.0f, "%.2f", 0);
    eli_pop_item_width();
}

#endif /* CHEAT_RENDER_LAYOUT_H */
