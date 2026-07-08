/**
 * @file test_p15_trees.c
 * @brief Unit tests for Phase 15 trees & collapsing headers: default-closed state,
 *        click-to-toggle (with eli_is_item_toggled_open), child indentation by the
 *        tree-node-to-label spacing, ELI_TREE_NODE_DEFAULT_OPEN, leaf nodes (always
 *        open, no push), collapsing-header toggling, and eli_set_next_item_open.
 *        Interaction is driven across frames through the input backend.
 *
 * @status Phase 15 tree coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/widgets/eli_widgets.h>
#include <eli/widgets/eli_tree.h>
#include <eli/font/eli_font.h>

#define TEST_DT (1.0f / 60.0f)

static eli_font_atlas *g_atlas = NULL;

static eli_context *tree_setup(void)
{
    eli_context *ctx = eli_create_context();
    ctx->io.delta_time = TEST_DT;
    ctx->io.display_size = eli_make_vec2(1024.0f, 768.0f);

    g_atlas = eli_font_atlas_create();
    eli_font *f = eli_font_atlas_add_font_default(g_atlas, NULL);
    eli_font_atlas_build(g_atlas);
    eli_push_font(f);
    return ctx;
}

static void tree_teardown(eli_context *ctx)
{
    eli_destroy_context(ctx);
    eli_font_atlas_destroy(g_atlas);
    g_atlas = NULL;
}

static void frame_begin(void)
{
    eli_new_frame();
    eli_input_update_begin_frame();
    eli_window_new_frame();
}

static void frame_end(void)
{
    eli_window_render();
    eli_render();
    eli_input_update_end_frame();
}

static void open_win(void)
{
    eli_set_next_window_pos(eli_make_vec2(0.0f, 0.0f), 0, eli_make_vec2(0.0f, 0.0f));
    eli_set_next_window_size(eli_make_vec2(300.0f, 300.0f), 0);
    eli_begin("W", NULL, 0);
}

/* ------------------------------------------------------------------------- */

ELI_TEST(tree_node_closed_by_default_and_toggles_on_arrow_click) {
    eli_context *ctx = tree_setup();

    /* Frame 1: a fresh tree node is closed. */
    eli_io_add_mouse_pos_event(200.0f, 200.0f);
    frame_begin();
    open_win();
    bool open1 = eli_tree_node("A_Root");
    ELI_ASSERT_FALSE(open1);
    if (open1) eli_tree_pop();
    eli_rect rect = ctx->last_item_rect;
    eli_end();
    frame_end();

    /* Frame 2: press on the arrow column (x just inside the left edge) toggles on
     * mouse-down; the node opens and reports the toggle. */
    float arrow_x = rect.x + 3.0f;
    float mid_y = rect.y + rect.h * 0.5f;
    eli_io_add_mouse_pos_event(arrow_x, mid_y);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    frame_begin();
    open_win();
    bool open2 = eli_tree_node("A_Root");
    ELI_ASSERT_TRUE(open2);
    ELI_ASSERT_TRUE(eli_is_item_toggled_open());
    if (open2) eli_tree_pop();
    eli_end();
    frame_end();

    /* Frame 3: release; the open state persists and no new toggle fires. */
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, false);
    frame_begin();
    open_win();
    bool open3 = eli_tree_node("A_Root");
    ELI_ASSERT_TRUE(open3);
    ELI_ASSERT_FALSE(eli_is_item_toggled_open());
    if (open3) eli_tree_pop();
    eli_end();
    frame_end();

    tree_teardown(ctx);
}

ELI_TEST(default_open_flag_starts_open_and_indents_children) {
    eli_context *ctx = tree_setup();

    frame_begin();
    open_win();
    float x_before = eli_get_cursor_screen_pos().x;
    bool open = eli_tree_node_ex("B_Node", ELI_TREE_NODE_DEFAULT_OPEN);
    ELI_ASSERT_TRUE(open);
    float x_after = eli_get_cursor_screen_pos().x;
    /* Children of an open node are indented by the tree-node-to-label spacing. */
    ELI_ASSERT_FLT_NEAR(x_after - x_before, eli_get_tree_node_to_label_spacing(), 0.6f);
    if (open) eli_tree_pop();
    eli_end();
    frame_end();

    tree_teardown(ctx);
}

ELI_TEST(leaf_node_is_always_open_and_does_not_push) {
    eli_context *ctx = tree_setup();

    frame_begin();
    open_win();
    float x_before = eli_get_cursor_screen_pos().x;
    bool open = eli_tree_node_ex("C_Leaf", ELI_TREE_NODE_LEAF);
    ELI_ASSERT_TRUE(open);                 /* a leaf always reports "open" */
    float x_after = eli_get_cursor_screen_pos().x;
    ELI_ASSERT_FLT_NEAR(x_after - x_before, 0.0f, 0.01f);  /* no indentation pushed */
    ELI_ASSERT_FALSE(eli_is_item_toggled_open());
    eli_end();                             /* no eli_tree_pop needed */
    frame_end();

    tree_teardown(ctx);
}

ELI_TEST(get_tree_node_to_label_spacing_matches_style) {
    eli_context *ctx = tree_setup();

    frame_begin();
    open_win();
    float expect = ctx->font_size + ctx->style.frame_padding.x * 2.0f;
    ELI_ASSERT_FLT_NEAR(eli_get_tree_node_to_label_spacing(), expect, 0.01f);
    eli_end();
    frame_end();

    tree_teardown(ctx);
}

ELI_TEST(collapsing_header_toggles_on_click) {
    eli_context *ctx = tree_setup();

    /* Frame 1: closed by default. */
    eli_io_add_mouse_pos_event(200.0f, 200.0f);
    frame_begin();
    open_win();
    ELI_ASSERT_FALSE(eli_collapsing_header("D_Head", ELI_TREE_NODE_NONE));
    eli_rect rect = ctx->last_item_rect;
    eli_end();
    frame_end();

    eli_vec2 center = eli_make_vec2(rect.x + rect.w * 0.5f, rect.y + rect.h * 0.5f);

    /* Frame 2: press in the label area -> not yet toggled (fires on release). */
    eli_io_add_mouse_pos_event(center.x, center.y);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    frame_begin();
    open_win();
    ELI_ASSERT_FALSE(eli_collapsing_header("D_Head", ELI_TREE_NODE_NONE));
    eli_end();
    frame_end();

    /* Frame 3: release inside -> toggles open. */
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, false);
    frame_begin();
    open_win();
    bool opened = eli_collapsing_header("D_Head", ELI_TREE_NODE_NONE);
    ELI_ASSERT_TRUE(opened);
    ELI_ASSERT_TRUE(eli_is_item_toggled_open());
    eli_end();
    frame_end();

    /* Frame 4: state persists (no indentation/push for a collapsing header). */
    frame_begin();
    open_win();
    ELI_ASSERT_TRUE(eli_collapsing_header("D_Head", ELI_TREE_NODE_NONE));
    eli_end();
    frame_end();

    tree_teardown(ctx);
}

ELI_TEST(set_next_item_open_forces_state) {
    eli_context *ctx = tree_setup();

    /* Force open. */
    frame_begin();
    open_win();
    eli_set_next_item_open(true, ELI_COND_ALWAYS);
    bool open = eli_tree_node("E_Node");
    ELI_ASSERT_TRUE(open);
    if (open) eli_tree_pop();
    eli_end();
    frame_end();

    /* Force closed the next frame, overriding the stored value. */
    frame_begin();
    open_win();
    eli_set_next_item_open(false, ELI_COND_ALWAYS);
    bool closed = eli_tree_node("E_Node");
    ELI_ASSERT_FALSE(closed);
    if (closed) eli_tree_pop();
    eli_end();
    frame_end();

    tree_teardown(ctx);
}

ELI_TEST(collapsing_header_bool_hidden_when_not_visible) {
    eli_context *ctx = tree_setup();
    bool visible = false;

    frame_begin();
    open_win();
    /* *p_visible == false: the header is not shown and reports closed. */
    ELI_ASSERT_FALSE(eli_collapsing_header_bool("F_Head", &visible, ELI_TREE_NODE_NONE));
    eli_end();
    frame_end();

    tree_teardown(ctx);
}

ELI_TEST_MAIN()
