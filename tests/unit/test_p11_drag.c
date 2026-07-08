/**
 * @file test_p11_drag.c
 * @brief Phase 11 drag interaction tests driven across frames with backend mouse
 *        events inside a window: dragging changes the value proportionally to the
 *        pixel delta times v_speed, clamps to both bounds, and only reports a
 *        change / edited flag on a frame the value actually moves.
 *
 * @status Phase 11 drag interaction coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/widgets/eli_drag.h>
#include <eli/widgets/eli_item_status.h>
#include <eli/font/eli_font.h>

#define TEST_DT (1.0f / 60.0f)

static eli_font_atlas *g_atlas = NULL;

static eli_context *drag_setup(void)
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

static void drag_teardown(eli_context *ctx)
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
    eli_set_next_window_size(eli_make_vec2(320.0f, 300.0f), 0);
    eli_begin("W", NULL, 0);
}

/* ------------------------------------------------------------------------- */

ELI_TEST(drag_float_moves_proportionally_and_clamps) {
    eli_context *ctx = drag_setup();
    float v = 2.0f;

    /* Frame 1: submit to establish the item rect. */
    eli_io_add_mouse_pos_event(300.0f, 260.0f);
    frame_begin();
    open_win();
    eli_drag_float("##D", &v, 0.1f, 0.0f, 10.0f, "%.3f", 0);
    eli_end();
    frame_end();
    eli_rect bb = ctx->last_item_rect;
    float cx = bb.x + bb.w * 0.5f;
    float cy = bb.y + bb.h * 0.5f;

    /* Frame 2: press to activate (no movement yet -> value unchanged). */
    eli_io_add_mouse_pos_event(cx, cy);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    frame_begin();
    open_win();
    bool c2 = eli_drag_float("##D", &v, 0.1f, 0.0f, 10.0f, "%.3f", 0);
    eli_end();
    frame_end();
    ELI_ASSERT_FALSE(c2);
    ELI_ASSERT_FLT_NEAR(2.0f, v, 0.0001f);

    /* Frame 3: drag right by 30px -> +30 * 0.1 = +3.0 -> 5.0. */
    eli_io_add_mouse_pos_event(cx + 30.0f, cy);
    frame_begin();
    open_win();
    bool c3 = eli_drag_float("##D", &v, 0.1f, 0.0f, 10.0f, "%.3f", 0);
    bool e3 = eli_is_item_edited();
    eli_end();
    frame_end();
    ELI_ASSERT_TRUE(c3);
    ELI_ASSERT_TRUE(e3);
    ELI_ASSERT_FLT_NEAR(5.0f, v, 0.0001f);

    /* Frame 4: hold still -> no change reported. */
    eli_io_add_mouse_pos_event(cx + 30.0f, cy);
    frame_begin();
    open_win();
    bool c4 = eli_drag_float("##D", &v, 0.1f, 0.0f, 10.0f, "%.3f", 0);
    bool e4 = eli_is_item_edited();
    eli_end();
    frame_end();
    ELI_ASSERT_FALSE(c4);
    ELI_ASSERT_FALSE(e4);
    ELI_ASSERT_FLT_NEAR(5.0f, v, 0.0001f);

    /* Frame 5: big drag right -> clamps to max. */
    eli_io_add_mouse_pos_event(cx + 330.0f, cy);
    frame_begin();
    open_win();
    eli_drag_float("##D", &v, 0.1f, 0.0f, 10.0f, "%.3f", 0);
    eli_end();
    frame_end();
    ELI_ASSERT_FLT_NEAR(10.0f, v, 0.0001f);

    /* Frame 6: big drag left -> clamps to min. */
    eli_io_add_mouse_pos_event(cx + 30.0f, cy);
    frame_begin();
    open_win();
    eli_drag_float("##D", &v, 0.1f, 0.0f, 10.0f, "%.3f", 0);
    eli_end();
    frame_end();
    ELI_ASSERT_FLT_NEAR(0.0f, v, 0.0001f);

    drag_teardown(ctx);
}

ELI_TEST(drag_int_steps_by_pixel_delta) {
    eli_context *ctx = drag_setup();
    int v = 0;

    eli_io_add_mouse_pos_event(300.0f, 260.0f);
    frame_begin();
    open_win();
    eli_drag_int("##DI", &v, 1.0f, -100, 100, NULL, 0);
    eli_end();
    frame_end();
    eli_rect bb = ctx->last_item_rect;
    float cx = bb.x + bb.w * 0.5f;
    float cy = bb.y + bb.h * 0.5f;

    /* Activate. */
    eli_io_add_mouse_pos_event(cx, cy);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    frame_begin();
    open_win();
    eli_drag_int("##DI", &v, 1.0f, -100, 100, NULL, 0);
    eli_end();
    frame_end();
    ELI_ASSERT_EQ(0, v);

    /* Drag +12px at speed 1 -> +12 integer steps. */
    eli_io_add_mouse_pos_event(cx + 12.0f, cy);
    frame_begin();
    open_win();
    bool changed = eli_drag_int("##DI", &v, 1.0f, -100, 100, NULL, 0);
    eli_end();
    frame_end();
    ELI_ASSERT_TRUE(changed);
    ELI_ASSERT_EQ(12, v);

    drag_teardown(ctx);
}

ELI_TEST_MAIN()
