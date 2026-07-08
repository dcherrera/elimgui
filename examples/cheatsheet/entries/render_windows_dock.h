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
 * The Docking entry is a full interactive playground: it hosts a live
 * eli_dock_space inside the card and docks three real windows into it, so the
 * user can drag panel tabs to undock, re-dock, split and tab them together and
 * SEE docking behave. See cheat_render_dock_playground.
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
 * Docking Playground
 *
 * A live, usable dock area hosted inside the card. eli_dock_space reserves a
 * region; three real windows ("Files", "Editor", "Output") are docked into it
 * via eli_set_next_window_dock_id on first appearance. From there the user can
 * drag a panel's tab to undock it, re-dock it, split the space, or tab panels
 * back together — the full docking interaction, live.
 * ------------------------------------------------------------------------- */

/* Generous dock area so all five drop zones and split previews are usable. */
#define CHEAT_DOCK_PG_W 560.0f
#define CHEAT_DOCK_PG_H 340.0f

/* The three docked panels. "##dockpg" keeps their ids unique to this card while
 * the tab/title shows only the text before "##". */
static const char *const cheat_dock_pg_wins[3] = {
    "Files##dockpg", "Editor##dockpg", "Output##dockpg"
};

/* Shared, function-static playground state (immediate-mode friendly). */
static int cheat_dock_pg_selected_file = 0; /* row selected in the Files panel */
static int cheat_dock_pg_log_lines = 3;     /* lines shown in the Output panel */

/**
 * Restore the initial layout: strip every playground window out of all dock
 * nodes and point its dock_id back at the root space. Emptied split children get
 * garbage-collected next frame (the root collapses back to a single leaf), then
 * the windows re-dock into it as tabs. Called from the "Reset layout" button.
 *
 * @param ctx   Current context (non-NULL).
 * @param root  Root dock-space id to re-home the panels onto.
 */
static void cheat_dock_pg_reset(eli_context *ctx, eli_id root)
{
    for (int i = 0; i < 3; i++) {
        eli_id wid = eli_hash_str(cheat_dock_pg_wins[i], 0);
        eli_window *win = eli_find_window_by_id(ctx, wid);
        if (win == NULL)
            continue;
        for (int k = 0; k < ctx->dock_nodes_count; k++)
            eli_dock_node_remove_window(ctx->dock_nodes[k], wid);
        win->dock_id = root;
    }
}

/** Files panel body: a selectable file list driving the shared selection. */
static void cheat_dock_pg_body_files(void)
{
    static const char *files[] = { "main.c", "eli_dock.h", "README.md", "build.sh" };
    for (int i = 0; i < (int)(sizeof(files) / sizeof(files[0])); i++) {
        if (eli_selectable(files[i], cheat_dock_pg_selected_file == i,
                           ELI_SELECTABLE_NONE, eli_make_vec2(0, 0)))
            cheat_dock_pg_selected_file = i;
    }
}

/** Editor panel body: shows the selected file and appends to the Output log. */
static void cheat_dock_pg_body_editor(void)
{
    static const char *files[] = { "main.c", "eli_dock.h", "README.md", "build.sh" };
    eli_text("Editing: %s", files[cheat_dock_pg_selected_file]);
    eli_text_wrapped("int main(void) { return 0; }");
    if (eli_button("Run") && cheat_dock_pg_log_lines < 99)
        cheat_dock_pg_log_lines++;
}

/** Output panel body: a small log whose length the Editor's Run button grows. */
static void cheat_dock_pg_body_output(void)
{
    for (int i = 0; i < cheat_dock_pg_log_lines; i++)
        eli_text("[%02d] build step ok", i);
    if (eli_button("Clear"))
        cheat_dock_pg_log_lines = 0;
}

/** Submit one docked panel window; docks into `root` on first appearance. */
static void cheat_dock_pg_panel(const char *name, eli_id root, void (*body)(void))
{
    eli_set_next_window_dock_id(root, ELI_COND_FIRST_USE_EVER);
    if (eli_begin(name, NULL, ELI_WINDOW_NONE))
        body();
    eli_end();
}

/**
 * The interactive docking playground. Hosts a dock space in the card and docks
 * three real windows into it. Drag a panel's tab to undock / re-dock / split /
 * tab; "Reset layout" restores the initial tabbed layout.
 */
static void cheat_render_dock_playground(void)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx == NULL)
        return;

    eli_id root = eli_get_id("cheat_dock_playground_space");

    eli_text_disabled("Drag a tab out to float it, then drag it back over the dock.");
    eli_text_disabled("Drop zones activate near the edges and center - aim for them:");
    eli_bullet_text("Center or tab bar   =  add as a TAB");
    eli_bullet_text("Left / right / bottom edge  =  SPLIT there");
    if (eli_button("Reset layout"))
        cheat_dock_pg_reset(ctx, root);

    eli_dock_space(root, eli_make_vec2(CHEAT_DOCK_PG_W, CHEAT_DOCK_PG_H), 0);

    cheat_dock_pg_panel(cheat_dock_pg_wins[0], root, cheat_dock_pg_body_files);
    cheat_dock_pg_panel(cheat_dock_pg_wins[1], root, cheat_dock_pg_body_editor);
    cheat_dock_pg_panel(cheat_dock_pg_wins[2], root, cheat_dock_pg_body_output);
}

/**
 * Supporting reference: queries the current window's dock state. Inside the
 * cheatsheet card the host is a plain child (dock_id == 0), illustrating the
 * floating default and the query API used to inspect docking at runtime.
 */
static void cheat_render_dock_query(void)
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
