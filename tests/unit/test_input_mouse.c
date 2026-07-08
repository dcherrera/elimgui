/**
 * @file test_input_mouse.c
 * @brief Unit tests for elimgui mouse input: click/release detection, click
 *        counts and double-click, drag threshold + delta, hover-rect boundaries,
 *        position validity, cursor get/set, and capture overrides.
 *
 * @status Phase 4 mouse input coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/input/eli_input.h>

/* Standard test frame delta (60 FPS). */
#define TEST_DT (1.0f / 60.0f)

static eli_context *mouse_setup(void)
{
    eli_context *ctx = eli_create_context();
    ctx->io.delta_time = TEST_DT;
    return ctx;
}

ELI_TEST(mouse_click_and_release) {
    eli_context *ctx = mouse_setup();

    /* Frame 1: move on-screen and press the left button. */
    eli_io_add_mouse_pos_event(5.0f, 5.0f);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    eli_input_update_begin_frame();
    ELI_ASSERT_TRUE(eli_is_mouse_down(ELI_MOUSE_BUTTON_LEFT));
    ELI_ASSERT_TRUE(eli_is_mouse_clicked(ELI_MOUSE_BUTTON_LEFT));
    ELI_ASSERT_FALSE(eli_is_mouse_released(ELI_MOUSE_BUTTON_LEFT));
    ELI_ASSERT_EQ(eli_get_mouse_clicked_count(ELI_MOUSE_BUTTON_LEFT), 1);
    ELI_ASSERT_TRUE(eli_is_any_mouse_down());
    eli_input_update_end_frame();

    /* Frame 2: still held -> no fresh click. */
    eli_input_update_begin_frame();
    ELI_ASSERT_TRUE(eli_is_mouse_down(ELI_MOUSE_BUTTON_LEFT));
    ELI_ASSERT_FALSE(eli_is_mouse_clicked(ELI_MOUSE_BUTTON_LEFT));
    eli_input_update_end_frame();

    /* Frame 3: release. */
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, false);
    eli_input_update_begin_frame();
    ELI_ASSERT_FALSE(eli_is_mouse_down(ELI_MOUSE_BUTTON_LEFT));
    ELI_ASSERT_TRUE(eli_is_mouse_released(ELI_MOUSE_BUTTON_LEFT));
    ELI_ASSERT_FALSE(eli_is_any_mouse_down());
    eli_input_update_end_frame();

    eli_destroy_context(ctx);
}

ELI_TEST(mouse_double_click) {
    eli_context *ctx = mouse_setup();

    /* Click 1. */
    eli_io_add_mouse_pos_event(5.0f, 5.0f);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    eli_input_update_begin_frame();
    ELI_ASSERT_EQ(eli_get_mouse_clicked_count(ELI_MOUSE_BUTTON_LEFT), 1);
    ELI_ASSERT_FALSE(eli_is_mouse_double_clicked(ELI_MOUSE_BUTTON_LEFT));
    eli_input_update_end_frame();

    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, false);
    eli_input_update_begin_frame();
    eli_input_update_end_frame();

    /* Click 2 at the same spot, well within the double-click time window. */
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    eli_input_update_begin_frame();
    ELI_ASSERT_EQ(eli_get_mouse_clicked_count(ELI_MOUSE_BUTTON_LEFT), 2);
    ELI_ASSERT_TRUE(eli_is_mouse_double_clicked(ELI_MOUSE_BUTTON_LEFT));
    ELI_ASSERT_TRUE(eli_is_mouse_clicked(ELI_MOUSE_BUTTON_LEFT));
    eli_input_update_end_frame();

    eli_destroy_context(ctx);
}

ELI_TEST(mouse_double_click_reset_when_far) {
    eli_context *ctx = mouse_setup();

    eli_io_add_mouse_pos_event(5.0f, 5.0f);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    eli_input_update_begin_frame();
    eli_input_update_end_frame();

    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, false);
    eli_input_update_begin_frame();
    eli_input_update_end_frame();

    /* Second click far away (> MouseDoubleClickMaxDist) is a fresh single click. */
    eli_io_add_mouse_pos_event(500.0f, 500.0f);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    eli_input_update_begin_frame();
    ELI_ASSERT_EQ(eli_get_mouse_clicked_count(ELI_MOUSE_BUTTON_LEFT), 1);
    ELI_ASSERT_FALSE(eli_is_mouse_double_clicked(ELI_MOUSE_BUTTON_LEFT));
    eli_input_update_end_frame();

    eli_destroy_context(ctx);
}

ELI_TEST(mouse_drag_threshold_and_delta) {
    eli_context *ctx = mouse_setup();

    /* Press at origin. */
    eli_io_add_mouse_pos_event(10.0f, 10.0f);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    eli_input_update_begin_frame();
    /* Not dragging yet (no movement past threshold). */
    ELI_ASSERT_FALSE(eli_is_mouse_dragging(ELI_MOUSE_BUTTON_LEFT, -1.0f));
    eli_input_update_end_frame();

    /* Move 25px right while held -> past the 6px threshold. */
    eli_io_add_mouse_pos_event(35.0f, 10.0f);
    eli_input_update_begin_frame();
    ELI_ASSERT_TRUE(eli_is_mouse_dragging(ELI_MOUSE_BUTTON_LEFT, -1.0f));
    eli_vec2 d = eli_get_mouse_drag_delta(ELI_MOUSE_BUTTON_LEFT, -1.0f);
    ELI_ASSERT_FLT_NEAR(d.x, 25.0f, 1e-4);
    ELI_ASSERT_FLT_NEAR(d.y, 0.0f, 1e-4);
    /* Mouse delta between frames reflects the move. */
    ELI_ASSERT_FLT_NEAR(ctx->io.mouse_delta.x, 25.0f, 1e-4);

    /* Reset the drag origin to the current position -> delta collapses to 0. */
    eli_reset_mouse_drag_delta(ELI_MOUSE_BUTTON_LEFT);
    d = eli_get_mouse_drag_delta(ELI_MOUSE_BUTTON_LEFT, -1.0f);
    ELI_ASSERT_FLT_NEAR(d.x, 0.0f, 1e-4);
    ELI_ASSERT_FALSE(eli_is_mouse_dragging(ELI_MOUSE_BUTTON_LEFT, -1.0f));
    eli_input_update_end_frame();

    eli_destroy_context(ctx);
}

ELI_TEST(mouse_hovering_rect_boundaries) {
    eli_context *ctx = mouse_setup();

    eli_vec2 rmin = eli_make_vec2(10.0f, 10.0f);
    eli_vec2 rmax = eli_make_vec2(20.0f, 20.0f);

    /* Inside. */
    eli_io_add_mouse_pos_event(15.0f, 15.0f);
    eli_input_update_begin_frame();
    ELI_ASSERT_TRUE(eli_is_mouse_hovering_rect(rmin, rmax, false));
    eli_input_update_end_frame();

    /* Min edge is inclusive. */
    eli_io_add_mouse_pos_event(10.0f, 10.0f);
    eli_input_update_begin_frame();
    ELI_ASSERT_TRUE(eli_is_mouse_hovering_rect(rmin, rmax, false));
    eli_input_update_end_frame();

    /* Max edge is exclusive. */
    eli_io_add_mouse_pos_event(20.0f, 20.0f);
    eli_input_update_begin_frame();
    ELI_ASSERT_FALSE(eli_is_mouse_hovering_rect(rmin, rmax, false));
    eli_input_update_end_frame();

    /* Outside. */
    eli_io_add_mouse_pos_event(5.0f, 5.0f);
    eli_input_update_begin_frame();
    ELI_ASSERT_FALSE(eli_is_mouse_hovering_rect(rmin, rmax, false));
    eli_input_update_end_frame();

    eli_destroy_context(ctx);
}

ELI_TEST(mouse_pos_validity) {
    eli_context *ctx = mouse_setup();

    /* Fresh context: mouse position is the invalid sentinel. */
    ELI_ASSERT_FALSE(eli_is_mouse_pos_valid(NULL));

    eli_io_add_mouse_pos_event(1.0f, 2.0f);
    eli_input_update_begin_frame();
    ELI_ASSERT_TRUE(eli_is_mouse_pos_valid(NULL));
    eli_vec2 p = eli_get_mouse_pos();
    ELI_ASSERT_FLT_NEAR(p.x, 1.0f, 1e-6);
    ELI_ASSERT_FLT_NEAR(p.y, 2.0f, 1e-6);
    eli_input_update_end_frame();

    eli_destroy_context(ctx);
}

ELI_TEST(mouse_cursor_and_capture_override) {
    eli_context *ctx = mouse_setup();

    eli_input_update_begin_frame();
    /* Cursor resets to arrow at begin-frame. */
    ELI_ASSERT_EQ(eli_get_mouse_cursor(), ELI_MOUSE_CURSOR_ARROW);
    eli_set_mouse_cursor(ELI_MOUSE_CURSOR_HAND);
    ELI_ASSERT_EQ(eli_get_mouse_cursor(), ELI_MOUSE_CURSOR_HAND);
    eli_input_update_end_frame();

    /* Next frame resets it again. */
    eli_input_update_begin_frame();
    ELI_ASSERT_EQ(eli_get_mouse_cursor(), ELI_MOUSE_CURSOR_ARROW);
    eli_input_update_end_frame();

    /* Capture override applies on the following frame. */
    eli_set_next_frame_want_capture_mouse(true);
    eli_input_update_begin_frame();
    ELI_ASSERT_TRUE(ctx->io.want_capture_mouse);
    eli_input_update_end_frame();

    eli_destroy_context(ctx);
}

ELI_TEST(mouse_wheel_accumulates_and_clears) {
    eli_context *ctx = mouse_setup();

    eli_io_add_mouse_wheel_event(0.0f, 1.0f);
    eli_io_add_mouse_wheel_event(0.0f, 0.5f);
    eli_input_update_begin_frame();
    ELI_ASSERT_FLT_NEAR(ctx->io.mouse_wheel, 1.5f, 1e-5);
    eli_input_update_end_frame();
    /* Wheel is cleared for the next frame. */
    ELI_ASSERT_FLT_NEAR(ctx->io.mouse_wheel, 0.0f, 1e-6);

    eli_destroy_context(ctx);
}

ELI_TEST_MAIN()
