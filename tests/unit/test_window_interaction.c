/**
 * @file test_window_interaction.c
 * @brief Unit tests for window interaction: hover + focus resolution across a
 *        z-order change, dragging the title bar to move a window, and child
 *        window creation/teardown.
 *
 * @status Phase 7 window interaction coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/window/eli_window.h>

#define TEST_DT (1.0f / 60.0f)

static eli_context *win_setup(void)
{
    eli_context *ctx = eli_create_context();
    ctx->io.delta_time = TEST_DT;
    ctx->io.display_size = eli_make_vec2(1024.0f, 768.0f);
    return ctx;
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

/* Open two overlapping windows: A at (0,0,200,200), B at (100,100,200,200). */
static void open_a(void)
{
    eli_set_next_window_pos(eli_make_vec2(0.0f, 0.0f), 0, eli_make_vec2(0.0f, 0.0f));
    eli_set_next_window_size(eli_make_vec2(200.0f, 200.0f), 0);
    eli_begin("A", NULL, 0);
}

static void open_b(void)
{
    eli_set_next_window_pos(eli_make_vec2(100.0f, 100.0f), 0, eli_make_vec2(0.0f, 0.0f));
    eli_set_next_window_size(eli_make_vec2(200.0f, 200.0f), 0);
    eli_begin("B", NULL, 0);
}

ELI_TEST(window_hover_and_focus_zorder) {
    eli_context *ctx = win_setup();

    /* Frame 1: create both. B is created last => focused/topmost. */
    eli_io_add_mouse_pos_event(150.0f, 150.0f);
    frame_begin();
    open_a(); eli_end();
    open_b(); eli_end();
    frame_end();

    /* Frame 2: mouse in the overlap; topmost (B) is hovered + focused. */
    eli_io_add_mouse_pos_event(150.0f, 150.0f);
    frame_begin();
    open_a();
    ELI_ASSERT_FALSE(eli_is_window_hovered());
    ELI_ASSERT_FALSE(eli_is_window_focused());
    eli_end();
    open_b();
    ELI_ASSERT_TRUE(eli_is_window_hovered());
    ELI_ASSERT_TRUE(eli_is_window_focused());
    eli_end();
    frame_end();

    /* Frame 3: explicitly focus A -> A becomes topmost. */
    eli_io_add_mouse_pos_event(150.0f, 150.0f);
    frame_begin();
    eli_set_next_window_focus();
    open_a();
    ELI_ASSERT_TRUE(eli_is_window_focused());
    eli_end();
    open_b();
    ELI_ASSERT_FALSE(eli_is_window_focused());
    eli_end();
    frame_end();

    /* Frame 4: with A now on top, the overlap hover flips to A. */
    eli_io_add_mouse_pos_event(150.0f, 150.0f);
    frame_begin();
    open_a();
    ELI_ASSERT_TRUE(eli_is_window_hovered());
    eli_end();
    open_b();
    ELI_ASSERT_FALSE(eli_is_window_hovered());
    eli_end();
    frame_end();

    eli_destroy_context(ctx);
}

ELI_TEST(window_title_bar_drag_moves) {
    eli_context *ctx = win_setup();

    /* Frame 1: create the window (establishes its rect for next-frame hover). */
    frame_begin();
    eli_set_next_window_pos(eli_make_vec2(100.0f, 120.0f), 0, eli_make_vec2(0.0f, 0.0f));
    eli_set_next_window_size(eli_make_vec2(200.0f, 150.0f), 0);
    eli_begin("Drag", NULL, 0);
    eli_end();
    frame_end();

    /* Frame 2: press the left button on the title bar -> begin moving. */
    eli_io_add_mouse_pos_event(150.0f, 125.0f); /* inside title bar (y in [120,139]) */
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    frame_begin();
    eli_begin("Drag", NULL, 0);
    eli_end();
    frame_end();
    ELI_ASSERT_NOT_NULL(ctx->moving_window);

    /* Frame 3: move the mouse by (+20,+20) while held -> window follows. */
    eli_io_add_mouse_pos_event(170.0f, 145.0f);
    frame_begin();
    eli_begin("Drag", NULL, 0);
    ELI_ASSERT_FLT_NEAR(eli_get_window_pos().x, 120.0f, 0.01f);
    ELI_ASSERT_FLT_NEAR(eli_get_window_pos().y, 140.0f, 0.01f);
    eli_end();
    frame_end();

    /* Frame 4: release -> movement ends. */
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, false);
    frame_begin();
    eli_begin("Drag", NULL, 0);
    eli_end();
    frame_end();
    ELI_ASSERT_NULL(ctx->moving_window);

    eli_destroy_context(ctx);
}

ELI_TEST(window_child_create_and_end) {
    eli_context *ctx = win_setup();

    frame_begin();
    eli_set_next_window_pos(eli_make_vec2(0.0f, 0.0f), 0, eli_make_vec2(0.0f, 0.0f));
    eli_set_next_window_size(eli_make_vec2(300.0f, 300.0f), 0);
    ELI_ASSERT_TRUE(eli_begin("Parent", NULL, 0));
    eli_window *parent = eli_get_current_window();

    ELI_ASSERT_TRUE(eli_begin_child("kid", eli_make_vec2(100.0f, 80.0f), ELI_CHILD_BORDERS, 0));
    eli_window *child = eli_get_current_window();
    ELI_ASSERT_NE(child, parent);
    ELI_ASSERT_EQ(child->parent_window, parent);
    ELI_ASSERT_EQ(child->root_window, parent);
    ELI_ASSERT_FLT_NEAR(eli_get_window_width(), 100.0f, 0.01f);
    ELI_ASSERT_FLT_NEAR(eli_get_window_height(), 80.0f, 0.01f);
    ELI_ASSERT_EQ(child->title_bar_height, 0.0f); /* children are decoration-free */
    eli_end_child();

    /* Back to the parent. */
    ELI_ASSERT_EQ(eli_get_current_window(), parent);
    eli_end();
    frame_end();

    ELI_ASSERT_EQ(ctx->windows_count, 2); /* parent + child in the pool */
    eli_destroy_context(ctx);
}

ELI_TEST(window_child_id_stable_across_frames) {
    eli_context *ctx = win_setup();

    eli_id child_id = 0;
    for (int frame = 0; frame < 2; frame++) {
        frame_begin();
        eli_set_next_window_size(eli_make_vec2(300.0f, 300.0f), 0);
        eli_begin("P", NULL, 0);
        eli_begin_child("kid", eli_make_vec2(50.0f, 50.0f), 0, 0);
        eli_window *child = eli_get_current_window();
        if (frame == 0)
            child_id = child->id;
        else
            ELI_ASSERT_EQ(child->id, child_id);
        eli_end_child();
        eli_end();
        frame_end();
    }
    ELI_ASSERT_EQ(ctx->windows_count, 2);
    eli_destroy_context(ctx);
}

ELI_TEST_MAIN()
