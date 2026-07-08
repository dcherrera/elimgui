/**
 * @file render_windows_dock.h
 * @brief Live-widget renderers for the "Child Windows", "Disabling & Clipping",
 *        "Docking", and "List Clipper" cheatsheet entries.
 *
 * Each function renders exactly one concept and owns its function-static state
 * so it can be called once per frame without side-effects.
 *
 * Include this file before rows_windows_dock.h so the cheat_render_* symbols
 * are already declared when the entry initializers reference them.
 *
 * @status Cheatsheet content. Not part of the library.
 * @issues None
 * @todo None
 */
#ifndef CHEAT_RENDER_WINDOWS_DOCK_H
#define CHEAT_RENDER_WINDOWS_DOCK_H

#include <eli/elimgui.h>

/* ---------------------------------------------------------------------------
 * Child Windows
 * ------------------------------------------------------------------------- */

/**
 * A small bordered, scrollable child window with several text lines so the
 * vertical scrollbar becomes visible.
 */
static void cheat_render_child_window(void)
{
    eli_vec2 size = eli_make_vec2(180.0f, 80.0f);
    if (eli_begin_child("##wd_child", size, ELI_CHILD_BORDERS, ELI_WINDOW_NONE)) {
        eli_text("Line 1");
        eli_text("Line 2");
        eli_text("Line 3");
        eli_text("Line 4");
        eli_text("Line 5  (scroll me)");
        eli_text("Line 6");
    }
    eli_end_child();
}

/* ---------------------------------------------------------------------------
 * Disabling & Clipping
 * ------------------------------------------------------------------------- */

/**
 * Wraps a button and checkbox in eli_begin_disabled(true) so both render at
 * reduced alpha and accept no mouse/keyboard input.
 */
static void cheat_render_disabled_scope(void)
{
    static bool flag = true;
    eli_begin_disabled(true);
    eli_button("Disabled Button");
    eli_same_line(0.0f, -1.0f);
    eli_checkbox("Disabled Check", &flag);
    eli_end_disabled();
    eli_text_disabled("(widgets above are non-interactive)");
}

/**
 * Pushes a clip rect 90 px wide and 46 px tall, draws three text rows that
 * overflow it to show the scissoring effect, then pops and shows a note.
 */
static void cheat_render_clip_rect(void)
{
    eli_vec2 p  = eli_get_cursor_screen_pos();
    eli_vec2 mn = p;
    eli_vec2 mx = eli_make_vec2(p.x + 90.0f, p.y + 46.0f);
    eli_push_clip_rect(mn, mx, true);
    eli_text("Clipped at 90 px wide");
    eli_text("This line is also clipped");
    eli_text("And this one too");
    eli_pop_clip_rect();
    eli_text_disabled("(above text clipped to 90 x 46 px)");
}

/* ---------------------------------------------------------------------------
 * Docking
 * ------------------------------------------------------------------------- */

/**
 * Creates a fixed-size dock space region. eli_get_id derives a stable id from
 * the current window's id stack. Drag any floating window over the shaded area
 * to dock it.
 *
 * Simplification: the live render shows only the empty dock-space region
 * inside the cheatsheet card. The full workflow (eli_dock_space +
 * eli_set_next_window_dock_id) is shown in the snippet.
 */
static void cheat_render_dock_space(void)
{
    eli_id   ds_id = eli_get_id("wd_dockspace");
    eli_vec2 size  = eli_make_vec2(180.0f, 70.0f);
    eli_dock_space(ds_id, size, 0);
    eli_text_disabled("(dock space region above)");
}

/**
 * Queries the current window's dock state and displays it as text. Inside the
 * cheatsheet card the window is always a plain child (dock_id == 0, not
 * docked), illustrating the default floating state.
 */
static void cheat_render_set_next_dock_id(void)
{
    eli_id dock_id = eli_get_window_dock_id();
    bool   docked  = eli_is_window_docked();
    eli_text("dock_id: %u  (0 = floating)", (unsigned)dock_id);
    eli_text("is_docked: %s", docked ? "yes" : "no");
    eli_text_disabled("Call eli_set_next_window_dock_id");
    eli_text_disabled("before eli_begin to dock a window.");
}

/* ---------------------------------------------------------------------------
 * List Clipper
 * ------------------------------------------------------------------------- */

/**
 * Renders 1000 items inside a small scrollable child window using the list
 * clipper so only visible rows are submitted each frame. The scroll bar tracks
 * the full 1000-item height because the clipper seeks the cursor past clipped
 * ranges.
 */
static void cheat_render_list_clipper(void)
{
    eli_vec2 size = eli_make_vec2(180.0f, 100.0f);
    if (eli_begin_child("##wd_clipper", size, ELI_CHILD_BORDERS, ELI_WINDOW_NONE)) {
        eli_list_clipper clipper;
        eli_list_clipper_begin(&clipper, 1000, -1.0f);
        while (eli_list_clipper_step(&clipper)) {
            for (int i = clipper.display_start; i < clipper.display_end; i++)
                eli_text("Item %04d", i);
        }
        eli_list_clipper_end(&clipper);
    }
    eli_end_child();
}

#endif /* CHEAT_RENDER_WINDOWS_DOCK_H */
