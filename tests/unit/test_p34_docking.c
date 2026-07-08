/**
 * @file test_p34_docking.c
 * @brief Unit tests for Phase 34 docking: dock-space node creation, docking a
 *        window via set_next_window_dock_id, a shared tab bar with click-to-
 *        switch selection, edge-drop splitting, tab tear-out (undock), the
 *        ELI_WINDOW_NO_DOCKING opt-out, and node-pool teardown on destroy.
 *
 * Driven headless across frames with backend mouse events (no browser).
 *
 * @status Phase 34 docking coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/docking/eli_dock.h>

#define TEST_DT (1.0f / 60.0f)

static const eli_id DOCK_ID = 0x00D0C500u;

static eli_context *dock_setup(void)
{
    eli_context *ctx = eli_create_context();
    ctx->io.delta_time = TEST_DT;
    ctx->io.display_size = eli_make_vec2(1024.0f, 768.0f);
    return ctx;
}

static void frame_begin(void)
{
    eli_new_frame();
    eli_util_new_frame();
    eli_input_update_begin_frame();
    eli_window_new_frame();
    eli_dock_new_frame();
}

static void frame_end(void)
{
    eli_window_render();
    eli_render();
    eli_input_update_end_frame();
}

/* Submit the host window and its dock space filling 600x400 at a fixed origin. */
static void submit_dockspace(void)
{
    eli_set_next_window_pos(eli_make_vec2(10.0f, 10.0f), 0, eli_make_vec2(0.0f, 0.0f));
    eli_set_next_window_size(eli_make_vec2(800.0f, 600.0f), 0);
    eli_begin("Host", NULL, ELI_WINDOW_NO_DOCKING);
    eli_dock_space(DOCK_ID, eli_make_vec2(600.0f, 400.0f), 0);
    eli_end();
}

ELI_TEST(dock_space_creates_root_node) {
    eli_context *ctx = dock_setup();

    frame_begin();
    submit_dockspace();
    frame_end();

    eli_dock_node *node = eli_dock_node_find(ctx, DOCK_ID);
    ELI_ASSERT_NOT_NULL(node);
    ELI_ASSERT_FLT_NEAR(node->rect.w, 600.0f, 0.01f);
    ELI_ASSERT_FLT_NEAR(node->rect.h, 400.0f, 0.01f);
    ELI_ASSERT_TRUE((node->flags & ELI_DOCK_NODE_IS_DOCK_SPACE) != 0);
    ELI_ASSERT_TRUE(eli_dock_node_is_leaf(node));

    eli_destroy_context(ctx);
}

ELI_TEST(set_next_dock_id_docks_window) {
    eli_context *ctx = dock_setup();

    /* Frame 1: create the dock space, then dock a window into it. */
    frame_begin();
    submit_dockspace();
    eli_set_next_window_dock_id(DOCK_ID, 0);
    bool visible = eli_begin("Alpha", NULL, 0);
    bool docked = eli_is_window_docked();
    eli_id got_id = eli_get_window_dock_id();
    eli_vec2 wpos = eli_get_window_pos();
    eli_vec2 wsize = eli_get_window_size();
    eli_end();
    frame_end();

    ELI_ASSERT_TRUE(visible);
    ELI_ASSERT_TRUE(docked);
    ELI_ASSERT_EQ(got_id, DOCK_ID);

    eli_dock_node *node = eli_dock_node_find(ctx, DOCK_ID);
    ELI_ASSERT_NOT_NULL(node);
    ELI_ASSERT_EQ(node->window_count, 1);
    eli_rect body = eli_dock_node_body_rect(node);
    ELI_ASSERT_FLT_NEAR(wpos.x, body.x, 0.01f);
    ELI_ASSERT_FLT_NEAR(wpos.y, body.y, 0.01f);
    ELI_ASSERT_FLT_NEAR(wsize.x, body.w, 0.01f);
    ELI_ASSERT_FLT_NEAR(wsize.y, body.h, 0.01f);

    eli_destroy_context(ctx);
}

ELI_TEST(two_windows_share_tab_bar_and_switch) {
    eli_context *ctx = dock_setup();

    /* Frame 1: dock two windows into the same node. First becomes selected. */
    frame_begin();
    submit_dockspace();
    eli_set_next_window_dock_id(DOCK_ID, 0);
    ELI_ASSERT_TRUE(eli_begin("Alpha", NULL, 0));   /* selected + visible */
    eli_end();
    eli_set_next_window_dock_id(DOCK_ID, 0);
    ELI_ASSERT_FALSE(eli_begin("Beta", NULL, 0));   /* hidden (not selected) */
    eli_end();
    frame_end();

    eli_dock_node *node = eli_dock_node_find(ctx, DOCK_ID);
    ELI_ASSERT_NOT_NULL(node);
    ELI_ASSERT_EQ(node->window_count, 2);
    ELI_ASSERT_EQ(node->selected_window_id, eli_hash_str("Alpha", 0));

    /* Frame 2: click on Beta's tab -> Beta becomes selected. Beta's tab is the
     * second tab, starting after Alpha's tab width along the strip. */
    float fs = 13.0f;
    float char_w = fs * ELI_DOCK_APPROX_CHAR_W_MULT;
    float tab_w = ELI_DOCK_TAB_PADDING_X * 2.0f + 4.0f * char_w; /* "Alpha"/"Beta" ~ len 4-5 */
    float beta_x = node->rect.x + (ELI_DOCK_TAB_PADDING_X * 2.0f + 5.0f * char_w) + tab_w * 0.5f;
    float strip_y = node->rect.y + node->tab_bar_height * 0.5f;
    eli_io_add_mouse_pos_event(beta_x, strip_y);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    frame_begin();
    submit_dockspace();
    eli_set_next_window_dock_id(DOCK_ID, 0);
    eli_begin("Alpha", NULL, 0);
    eli_end();
    eli_set_next_window_dock_id(DOCK_ID, 0);
    eli_begin("Beta", NULL, 0);
    eli_end();
    frame_end();

    ELI_ASSERT_EQ(node->selected_window_id, eli_hash_str("Beta", 0));

    eli_destroy_context(ctx);
}

ELI_TEST(edge_drop_splits_node) {
    eli_context *ctx = dock_setup();

    /* Frame 1: dock Alpha into the space; submit Beta as a floating window so it
     * exists in the pool for the drop request to reference. */
    frame_begin();
    submit_dockspace();
    eli_set_next_window_dock_id(DOCK_ID, 0);
    eli_begin("Alpha", NULL, 0);
    eli_end();
    eli_begin("Beta", NULL, 0);
    eli_end();
    frame_end();

    eli_dock_node *root = eli_dock_node_find(ctx, DOCK_ID);
    ELI_ASSERT_NOT_NULL(root);
    float parent_w = root->rect.w;

    /* Frame 2: simulate a drop of Beta on the RIGHT edge (drag path drop) and let
     * eli_dock_new_frame resolve it into a split. */
    ctx->has_dock_request = true;
    ctx->dock_request_target = DOCK_ID;
    ctx->dock_request_window = eli_hash_str("Beta", 0);
    ctx->dock_request_dir = ELI_DOCK_DIR_RIGHT;
    frame_begin();
    submit_dockspace();
    frame_end();

    root = eli_dock_node_find(ctx, DOCK_ID);
    ELI_ASSERT_NOT_NULL(root);
    ELI_ASSERT_FALSE(eli_dock_node_is_leaf(root));
    ELI_ASSERT_EQ(root->split_axis, ELI_DOCK_AXIS_X);
    ELI_ASSERT_NOT_NULL(root->child[0]);
    ELI_ASSERT_NOT_NULL(root->child[1]);

    float w0 = root->child[0]->rect.w;
    float w1 = root->child[1]->rect.w;
    ELI_ASSERT_FLT_NEAR(w0, w1, 1.5f);                          /* each roughly half */
    ELI_ASSERT_FLT_NEAR(w0 + w1 + ELI_DOCK_SEPARATOR_SIZE, parent_w, 0.01f);

    eli_destroy_context(ctx);
}

/* Regression: a split must keep the PRE-EXISTING window docked. The split moved
 * the window list into a new "keep" child but left the window's dock_id naming
 * the (now non-leaf) parent, so it floated away one frame later. */
ELI_TEST(split_keeps_existing_window_docked) {
    eli_context *ctx = dock_setup();

    frame_begin();
    submit_dockspace();
    eli_set_next_window_dock_id(DOCK_ID, 0);
    eli_begin("Alpha", NULL, 0);
    eli_end();
    eli_begin("Beta", NULL, 0);
    eli_end();
    frame_end();

    ctx->has_dock_request = true;
    ctx->dock_request_target = DOCK_ID;
    ctx->dock_request_window = eli_hash_str("Beta", 0);
    ctx->dock_request_dir = ELI_DOCK_DIR_RIGHT;
    frame_begin();
    submit_dockspace();
    frame_end();

    /* Alpha (the pre-existing panel) must still be docked into a LEAF node, not
     * left pointing at the now non-leaf root. */
    eli_id alpha_id = eli_hash_str("Alpha", 0);
    eli_window *alpha = NULL;
    for (int i = 0; i < ctx->windows_count; i++)
        if (ctx->windows[i]->id == alpha_id) alpha = ctx->windows[i];
    ELI_ASSERT_NOT_NULL(alpha);
    ELI_ASSERT_NE(alpha->dock_id, 0u);                         /* not floating */

    eli_dock_node *an = eli_dock_node_find(ctx, alpha->dock_id);
    ELI_ASSERT_NOT_NULL(an);
    ELI_ASSERT_TRUE(eli_dock_node_is_leaf(an));                /* docked in a leaf */

    eli_destroy_context(ctx);
}

/* Regression: re-docking a window must detach it from its old node, or the old
 * node keeps a stale entry, never empties, never gets GC'd, and the dock-node
 * pool grows without bound (which froze the app under heavy docking). */
ELI_TEST(redock_does_not_leak_nodes) {
    eli_context *ctx = dock_setup();

    frame_begin();
    submit_dockspace();
    eli_set_next_window_dock_id(DOCK_ID, 0);
    eli_begin("Alpha", NULL, 0);
    eli_end();
    eli_set_next_window_dock_id(DOCK_ID, 0);
    eli_begin("Beta", NULL, 0);
    eli_end();
    frame_end();

    /* Churn: repeatedly split then re-tab the two windows around the space. */
    for (int i = 0; i < 40; i++) {
        ctx->has_dock_request = true;
        ctx->dock_request_target = DOCK_ID;
        ctx->dock_request_window = eli_hash_str((i & 1) ? "Alpha" : "Beta", 0);
        ctx->dock_request_dir = (i % 2) ? ELI_DOCK_DIR_RIGHT : ELI_DOCK_DIR_CENTER;
        ctx->dock_drag_window_id = ctx->dock_request_window;
        frame_begin();
        submit_dockspace();
        frame_end();
    }

    /* Two windows + one dock space can never need more than a handful of nodes.
     * Before the fix this grew unboundedly. */
    ELI_ASSERT_LT(ctx->dock_nodes_count, 8);

    eli_destroy_context(ctx);
}

ELI_TEST(tab_drag_out_undocks) {
    eli_context *ctx = dock_setup();

    /* Frame 1: dock Alpha and Beta. */
    frame_begin();
    submit_dockspace();
    eli_set_next_window_dock_id(DOCK_ID, 0);
    eli_begin("Alpha", NULL, 0);
    eli_end();
    eli_set_next_window_dock_id(DOCK_ID, 0);
    eli_begin("Beta", NULL, 0);
    eli_end();
    frame_end();

    eli_dock_node *node = eli_dock_node_find(ctx, DOCK_ID);
    ELI_ASSERT_EQ(node->window_count, 2);
    float fs = 13.0f;
    float char_w = fs * ELI_DOCK_APPROX_CHAR_W_MULT;
    float alpha_cx = node->rect.x + (ELI_DOCK_TAB_PADDING_X * 2.0f + 5.0f * char_w) * 0.5f;
    float strip_y = node->rect.y + node->tab_bar_height * 0.5f;

    /* Frame 2: press on Alpha's tab (arms the potential tear-out). */
    eli_io_add_mouse_pos_event(alpha_cx, strip_y);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    frame_begin();
    submit_dockspace();
    eli_set_next_window_dock_id(DOCK_ID, 0);
    eli_begin("Alpha", NULL, 0);
    eli_end();
    eli_set_next_window_dock_id(DOCK_ID, 0);
    eli_begin("Beta", NULL, 0);
    eli_end();
    frame_end();

    /* Frame 3: drag far past the undock threshold while held -> Alpha tears off. */
    eli_io_add_mouse_pos_event(alpha_cx + 60.0f, strip_y + 60.0f);
    frame_begin();
    submit_dockspace();
    eli_set_next_window_dock_id(DOCK_ID, 0);
    eli_begin("Alpha", NULL, 0);
    eli_end();
    eli_set_next_window_dock_id(DOCK_ID, 0);
    eli_begin("Beta", NULL, 0);
    eli_end();
    frame_end();

    eli_window *alpha = eli_find_window_by_id(ctx, eli_hash_str("Alpha", 0));
    ELI_ASSERT_NOT_NULL(alpha);
    ELI_ASSERT_EQ(alpha->dock_id, 0u);
    ELI_ASSERT_NULL(alpha->dock_node);
    ELI_ASSERT_EQ(node->window_count, 1);
    ELI_ASSERT_EQ(node->selected_window_id, eli_hash_str("Beta", 0));

    eli_destroy_context(ctx);
}

/* The central area AND the tab-bar strip both resolve to CENTER (tab), while the
 * clear outer edge bands resolve to a split. This is the tab-vs-split rule that
 * keeps a floated panel dragged back over the dock re-joining its tab group. */
ELI_TEST(hit_zone_center_and_strip_tab_edges_split) {
    eli_context *ctx = dock_setup();

    frame_begin();
    submit_dockspace();
    eli_set_next_window_dock_id(DOCK_ID, 0);
    eli_begin("Alpha", NULL, 0);
    eli_end();
    frame_end();

    eli_dock_node *node = eli_dock_node_find(ctx, DOCK_ID);
    ELI_ASSERT_NOT_NULL(node);
    eli_rect r = node->rect;
    eli_vec2 center = eli_rect_center(r);

    /* Dead center -> tab. */
    ELI_ASSERT_EQ(eli_dock__hit_zone(node, center), ELI_DOCK_DIR_CENTER);
    /* On the tab-bar / header strip -> tab (the user's pain point). */
    eli_vec2 on_strip = eli_make_vec2(r.x + r.w * 0.5f, r.y + 2.0f);
    ELI_ASSERT_EQ(eli_dock__hit_zone(node, on_strip), ELI_DOCK_DIR_CENTER);

    /* Clear outer edge bands -> split toward that edge. */
    ELI_ASSERT_EQ(eli_dock__hit_zone(node, eli_make_vec2(r.x + 4.0f, center.y)),
                  ELI_DOCK_DIR_LEFT);
    ELI_ASSERT_EQ(eli_dock__hit_zone(node, eli_make_vec2(r.x + r.w - 4.0f, center.y)),
                  ELI_DOCK_DIR_RIGHT);
    ELI_ASSERT_EQ(eli_dock__hit_zone(node, eli_make_vec2(center.x, r.y + r.h - 4.0f)),
                  ELI_DOCK_DIR_DOWN);

    /* Outside the node -> no target. */
    ELI_ASSERT_EQ(eli_dock__hit_zone(node, eli_make_vec2(r.x - 20.0f, center.y)),
                  ELI_DOCK_DIR_NONE);

    eli_destroy_context(ctx);
}

/* A drop resolved to the center TABS the window into the existing node (its
 * window_count grows and the node stays a leaf), rather than splitting it. */
ELI_TEST(center_drop_tabs_into_node) {
    eli_context *ctx = dock_setup();

    frame_begin();
    submit_dockspace();
    eli_set_next_window_dock_id(DOCK_ID, 0);
    eli_begin("Alpha", NULL, 0);
    eli_end();
    eli_begin("Beta", NULL, 0);   /* floating, exists in the pool */
    eli_end();
    frame_end();

    eli_dock_node *root = eli_dock_node_find(ctx, DOCK_ID);
    ELI_ASSERT_NOT_NULL(root);
    ELI_ASSERT_EQ(root->window_count, 1);

    /* Frame 2: drop Beta in the CENTER -> it tabs in. */
    ctx->has_dock_request = true;
    ctx->dock_request_target = DOCK_ID;
    ctx->dock_request_window = eli_hash_str("Beta", 0);
    ctx->dock_request_dir = ELI_DOCK_DIR_CENTER;
    frame_begin();
    submit_dockspace();
    frame_end();

    root = eli_dock_node_find(ctx, DOCK_ID);
    ELI_ASSERT_NOT_NULL(root);
    ELI_ASSERT_TRUE(eli_dock_node_is_leaf(root));              /* no split occurred */
    ELI_ASSERT_EQ(root->window_count, 2);                      /* both windows share tabs */
    ELI_ASSERT_EQ(root->selected_window_id, eli_hash_str("Beta", 0));

    eli_destroy_context(ctx);
}

/* A node too small to yield two >= min-pane panes can only be tabbed into: every
 * point (including the far edges) resolves to CENTER, never a split. */
ELI_TEST(tiny_node_only_tabs) {
    eli_context *ctx = dock_setup();

    eli_dock_node *node = eli_dock_node_get_or_create(ctx, 0xBEEFu, 0);
    ELI_ASSERT_NOT_NULL(node);
    node->rect = eli_make_rect(100.0f, 100.0f, 120.0f, 80.0f);  /* < 2*96 on both axes */
    node->tab_bar_height = 20.0f;

    ELI_ASSERT_TRUE(eli_dock__can_split_axis(node, ELI_DOCK_AXIS_X) == false);
    ELI_ASSERT_TRUE(eli_dock__can_split_axis(node, ELI_DOCK_AXIS_Y) == false);

    /* Body points near each edge still tab (only tabbing is offered). */
    ELI_ASSERT_EQ(eli_dock__hit_zone(node, eli_make_vec2(101.0f, 150.0f)), ELI_DOCK_DIR_CENTER);
    ELI_ASSERT_EQ(eli_dock__hit_zone(node, eli_make_vec2(219.0f, 150.0f)), ELI_DOCK_DIR_CENTER);
    ELI_ASSERT_EQ(eli_dock__hit_zone(node, eli_make_vec2(160.0f, 178.0f)), ELI_DOCK_DIR_CENTER);

    eli_destroy_context(ctx);
}

ELI_TEST(no_docking_flag_refuses_dock) {
    eli_context *ctx = dock_setup();

    frame_begin();
    submit_dockspace();
    eli_set_next_window_dock_id(DOCK_ID, 0);
    eli_begin("NoDock", NULL, ELI_WINDOW_NO_DOCKING);
    bool docked = eli_is_window_docked();
    eli_id id = eli_get_window_dock_id();
    eli_end();
    frame_end();

    ELI_ASSERT_FALSE(docked);
    ELI_ASSERT_EQ(id, 0u);

    eli_dock_node *node = eli_dock_node_find(ctx, DOCK_ID);
    ELI_ASSERT_NOT_NULL(node);
    ELI_ASSERT_EQ(node->window_count, 0);

    eli_destroy_context(ctx);
}

ELI_TEST(node_pool_freed_on_destroy) {
    eli_context *ctx = dock_setup();

    frame_begin();
    submit_dockspace();
    eli_set_next_window_dock_id(DOCK_ID, 0);
    eli_begin("Alpha", NULL, 0);
    eli_end();
    eli_begin("Beta", NULL, 0);
    eli_end();
    frame_end();

    /* Split the root so the pool holds several individually-allocated nodes. */
    eli_dock_node *root = eli_dock_node_find(ctx, DOCK_ID);
    ELI_ASSERT_NOT_NULL(root);
    ELI_ASSERT_NOT_NULL(eli_dock_node_split(ctx, root, ELI_DOCK_DIR_DOWN));
    ELI_ASSERT_GE(ctx->dock_nodes_count, 3);
    ELI_ASSERT_NOT_NULL(ctx->dock_shutdown_fn);

    /* Destroy must free the whole pool without leaking or crashing. */
    eli_destroy_context(ctx);
}

ELI_TEST_MAIN()
