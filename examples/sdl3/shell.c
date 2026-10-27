/**
 * @file shell.c
 * @brief SDL3 example, second TU: a viewport dockspace split into a left and a
 *        right pane, with an input_text, a multiline input and a table.
 *
 * Compiled without ELI_IMPLEMENTATION: every eli_* call here uses the context,
 * pools and clipboard defined in main.c (ELI_SHARED_STATE).
 *
 * @status In use.
 * @issues None
 * @todo None
 */
#include "example_app.h"

#include <eli/elimgui.h>

#include <stdio.h>

static char g_shell_name[8192] = "type here";
static int g_shell_app_copy_count = 0;   /* app-level Shortcut+C (GLOBAL route) */
static char g_shell_notes[4096] = "Multiline notes.\nSecond line.";
static eli_vec2 g_shell_name_center;
static bool g_shell_name_known = false;
static eli_id g_shell_left_id = 0;   /* dock node for "Files" */
static eli_id g_shell_right_id = 0;  /* dock node for "Editor" (top right) */
static eli_id g_shell_bottom_id = 0; /* dock node for the perf fixture (bottom right) */

/** Split the dockspace once into three panes: Files | Editor / Perf fixture. */
static void shell_build_layout(eli_id space_id)
{
    eli_context *ctx = eli_get_current_context();
    eli_dock_node *root = eli_dock_node_find(ctx, space_id);
    if (!root || !eli_dock_node_is_leaf(root))
        return;
    eli_dock_node *left = eli_dock_node_split(ctx, root, ELI_DOCK_DIR_LEFT);
    if (!left)
        return;
    root->split_ratio = 0.3f;
    g_shell_left_id = left->id;
    eli_dock_node *right = (root->child[0] == left) ? root->child[1] : root->child[0];
    eli_dock_node *bottom = eli_dock_node_split(ctx, right, ELI_DOCK_DIR_DOWN);
    if (!bottom) {
        g_shell_right_id = right->id;
        return;
    }
    right->split_ratio = 0.35f;
    g_shell_bottom_id = bottom->id;
    g_shell_right_id = (right->child[0] == bottom) ? right->child[1]->id : right->child[0]->id;
}

/** Dock the next window into `node_id` on first use (0 = leave floating). */
static void shell_dock_next(eli_id node_id)
{
    if (node_id != 0)
        eli_set_next_window_dock_id(node_id, ELI_COND_FIRST_USE_EVER);
}

/** "Files": a table of fake rows. */
static void shell_draw_files(void)
{
    shell_dock_next(g_shell_left_id);
    if (eli_begin("Files", NULL, 0)) {
        if (eli_begin_table("files", 3, ELI_TABLE_BORDERS | ELI_TABLE_ROW_BG)) {
            eli_table_setup_column("Name", 0, 0.0f, 0);
            eli_table_setup_column("Size", 0, 0.0f, 0);
            eli_table_setup_column("Kind", 0, 0.0f, 0);
            eli_table_headers_row();
            for (int i = 0; i < 40; i++) {
                eli_table_next_row();
                eli_table_next_column();
                eli_text("file_%02d.c", i);
                eli_table_next_column();
                eli_text("%d KB", (i * 37) % 200 + 1);
                eli_table_next_column();
                eli_text("%s", (i % 3) ? "source" : "header");
            }
            eli_end_table();
        }
    }
    eli_end();
}

/** "Editor": single-line and multiline input. */
static void shell_draw_editor(void)
{
    shell_dock_next(g_shell_right_id);
    if (eli_begin("Editor", NULL, 0)) {
        eli_input_text("Name", g_shell_name, sizeof(g_shell_name), 0, NULL, NULL);
        eli_vec2 mn = eli_get_item_rect_min(), mx = eli_get_item_rect_max();
        g_shell_name_center = eli_make_vec2((mn.x + mx.x) * 0.5f, (mn.y + mx.y) * 0.5f);
        g_shell_name_known = true;
        eli_input_text_multiline("Notes", g_shell_notes, sizeof(g_shell_notes),
                                 eli_make_vec2(-1.0f, 160.0f), 0, NULL, NULL);
        eli_text("Clipboard: Ctrl/Cmd+C/V in the fields above.");
        example_tasks_draw();
    }
    eli_end();
}

bool example_shell_name_field_center(float *x, float *y)
{
    *x = g_shell_name_center.x;
    *y = g_shell_name_center.y;
    return g_shell_name_known;
}

unsigned int example_shell_fixture_dock_id(void)
{
    return (unsigned int)(g_shell_bottom_id ? g_shell_bottom_id : g_shell_right_id);
}

const char *example_shell_name_text(void)
{
    return g_shell_name;
}

int example_shell_app_copy_count(void)
{
    return g_shell_app_copy_count;
}

void example_shell_draw(void)
{
    /* App-level copy shortcut: yields to an active text field, which owns Shortcut+C. */
    if (eli_shortcut_ex(ELI_SHORTCUT_COPY, ELI_INPUT_ROUTE_GLOBAL))
        g_shell_app_copy_count++;
    eli_id space = eli_dock_space_over_viewport(eli_get_main_viewport(), 0);
    if (g_shell_left_id == 0 && space != 0)
        shell_build_layout(space);
    shell_draw_files();
    shell_draw_editor();
}
