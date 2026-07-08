/**
 * @file test_widget_buttons.c
 * @brief Unit tests for Phase 9 widgets: button click semantics, checkbox toggle +
 *        edited flag, radio-button selection, progress-bar clamp/advance, and text
 *        cursor advancement. Interaction is driven across frames by feeding mouse
 *        events through the input backend, mirroring the browser event flow.
 *
 * @status Phase 9 widget coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/widgets/eli_widgets.h>
#include <eli/font/eli_font.h>

#define TEST_DT (1.0f / 60.0f)

static eli_font_atlas *g_atlas = NULL;

static eli_context *widget_setup(void)
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

static void widget_teardown(eli_context *ctx)
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

static eli_vec2 rect_center(eli_rect r)
{
    return eli_make_vec2(r.x + r.w * 0.5f, r.y + r.h * 0.5f);
}

/* ------------------------------------------------------------------------- */

ELI_TEST(button_pressed_on_click_release_inside) {
    eli_context *ctx = widget_setup();

    /* Frame 1: submit the button to establish its rect for next-frame hover. */
    eli_io_add_mouse_pos_event(20.0f, 40.0f);
    frame_begin();
    open_win();
    ELI_ASSERT_FALSE(eli_button("Go"));
    eli_end();
    frame_end();
    eli_vec2 c = rect_center(ctx->last_item_rect);

    /* Frame 2: press inside -> not yet pressed, but becomes active/held. */
    eli_io_add_mouse_pos_event(c.x, c.y);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    frame_begin();
    open_win();
    bool p2 = eli_button("Go");
    ELI_ASSERT_FALSE(p2);
    ELI_ASSERT_TRUE(eli_is_item_active());
    eli_end();
    frame_end();

    /* Frame 3: release inside -> pressed fires exactly this frame. */
    eli_io_add_mouse_pos_event(c.x, c.y);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, false);
    frame_begin();
    open_win();
    bool p3 = eli_button("Go");
    ELI_ASSERT_TRUE(p3);
    eli_end();
    frame_end();

    widget_teardown(ctx);
}

ELI_TEST(button_release_outside_does_not_press) {
    eli_context *ctx = widget_setup();

    eli_io_add_mouse_pos_event(20.0f, 40.0f);
    frame_begin();
    open_win();
    eli_button("Go");
    eli_end();
    frame_end();
    eli_vec2 c = rect_center(ctx->last_item_rect);

    /* Press inside. */
    eli_io_add_mouse_pos_event(c.x, c.y);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    frame_begin();
    open_win();
    eli_button("Go");
    eli_end();
    frame_end();

    /* Move far away, then release -> no press. */
    eli_io_add_mouse_pos_event(500.0f, 500.0f);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, false);
    frame_begin();
    open_win();
    ELI_ASSERT_FALSE(eli_button("Go"));
    eli_end();
    frame_end();

    widget_teardown(ctx);
}

ELI_TEST(button_hover_sets_item_hovered) {
    eli_context *ctx = widget_setup();

    eli_io_add_mouse_pos_event(20.0f, 40.0f);
    frame_begin();
    open_win();
    eli_button("Go");
    eli_end();
    frame_end();
    eli_vec2 c = rect_center(ctx->last_item_rect);

    /* Hover the button on the next frame. */
    eli_io_add_mouse_pos_event(c.x, c.y);
    frame_begin();
    open_win();
    eli_button("Go");
    ELI_ASSERT_TRUE(eli_is_item_hovered(ELI_HOVERED_NONE));
    ELI_ASSERT_TRUE(eli_is_any_item_hovered());
    eli_end();
    frame_end();

    /* Move off the button -> no longer hovered. */
    eli_io_add_mouse_pos_event(250.0f, 250.0f);
    frame_begin();
    open_win();
    eli_button("Go");
    ELI_ASSERT_FALSE(eli_is_item_hovered(ELI_HOVERED_NONE));
    eli_end();
    frame_end();

    widget_teardown(ctx);
}

ELI_TEST(checkbox_toggles_and_marks_edited) {
    eli_context *ctx = widget_setup();
    bool v = false;

    eli_io_add_mouse_pos_event(20.0f, 40.0f);
    frame_begin();
    open_win();
    eli_checkbox("C", &v);
    eli_end();
    frame_end();
    eli_vec2 c = rect_center(ctx->last_item_rect);
    ELI_ASSERT_FALSE(v);

    /* Press. */
    eli_io_add_mouse_pos_event(c.x, c.y);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    frame_begin();
    open_win();
    eli_checkbox("C", &v);
    ELI_ASSERT_FALSE(v);       /* toggles on release, not press */
    eli_end();
    frame_end();

    /* Release inside -> toggles + edited this frame. */
    eli_io_add_mouse_pos_event(c.x, c.y);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, false);
    frame_begin();
    open_win();
    bool pressed = eli_checkbox("C", &v);
    ELI_ASSERT_TRUE(pressed);
    ELI_ASSERT_TRUE(v);
    ELI_ASSERT_TRUE(eli_is_item_edited());
    ELI_ASSERT_TRUE(eli_is_item_deactivated());
    eli_end();
    frame_end();

    widget_teardown(ctx);
}

ELI_TEST(radio_button_selects_value) {
    eli_context *ctx = widget_setup();
    int sel = 0;

    /* Two radios stacked; establish rects. */
    eli_io_add_mouse_pos_event(20.0f, 40.0f);
    frame_begin();
    open_win();
    eli_radio_button_int("A", &sel, 0);
    eli_radio_button_int("B", &sel, 1);
    eli_rect b_rect = ctx->last_item_rect;
    eli_end();
    frame_end();
    eli_vec2 c = rect_center(b_rect);
    ELI_ASSERT_EQ(sel, 0);

    /* Click radio B (press then release). */
    eli_io_add_mouse_pos_event(c.x, c.y);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    frame_begin();
    open_win();
    eli_radio_button_int("A", &sel, 0);
    eli_radio_button_int("B", &sel, 1);
    eli_end();
    frame_end();

    eli_io_add_mouse_pos_event(c.x, c.y);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, false);
    frame_begin();
    open_win();
    eli_radio_button_int("A", &sel, 0);
    eli_radio_button_int("B", &sel, 1);
    eli_end();
    frame_end();

    ELI_ASSERT_EQ(sel, 1);
    widget_teardown(ctx);
}

ELI_TEST(progress_bar_clamps_and_advances) {
    eli_context *ctx = widget_setup();

    frame_begin();
    open_win();
    float y0 = eli_get_cursor_pos_y();
    eli_progress_bar(2.0f, eli_make_vec2(120.0f, 0.0f), NULL); /* fraction clamps to 1 */
    eli_vec2 size = eli_get_item_rect_size();
    float y1 = eli_get_cursor_pos_y();
    eli_end();
    frame_end();

    ELI_ASSERT_FLT_NEAR(size.x, 120.0f, 0.01f);
    ELI_ASSERT_FLT_NEAR(size.y, eli_get_frame_height(), 0.01f);
    ELI_ASSERT_GT(y1, y0);   /* cursor advanced past the bar */

    /* A negative fraction is also accepted (clamped) without changing geometry. */
    frame_begin();
    open_win();
    eli_progress_bar(-1.0f, eli_make_vec2(120.0f, 0.0f), "x");
    ELI_ASSERT_FLT_NEAR(eli_get_item_rect_size().x, 120.0f, 0.01f);
    eli_end();
    frame_end();

    widget_teardown(ctx);
}

ELI_TEST(text_advances_cursor_by_text_size) {
    eli_context *ctx = widget_setup();

    frame_begin();
    open_win();
    eli_vec2 expect = eli_calc_text_size("Hello", NULL);
    float y0 = eli_get_cursor_pos_y();
    eli_text("Hello");
    eli_vec2 size = eli_get_item_rect_size();
    float y1 = eli_get_cursor_pos_y();
    eli_end();
    frame_end();

    ELI_ASSERT_FLT_NEAR(size.x, expect.x, 0.01f);
    ELI_ASSERT_FLT_NEAR(size.y, expect.y, 0.01f);
    ELI_ASSERT_FLT_NEAR(y1 - y0, expect.y + ctx->style.item_spacing.y, 0.01f);

    widget_teardown(ctx);
}

ELI_TEST(checkbox_flags_set_and_clear_bits) {
    eli_context *ctx = widget_setup();
    int flags = 0;
    const int BIT = 1 << 2;

    eli_io_add_mouse_pos_event(20.0f, 40.0f);
    frame_begin();
    open_win();
    eli_checkbox_flags_int("F", &flags, BIT);
    eli_end();
    frame_end();
    eli_vec2 c = rect_center(ctx->last_item_rect);

    eli_io_add_mouse_pos_event(c.x, c.y);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    frame_begin();
    open_win();
    eli_checkbox_flags_int("F", &flags, BIT);
    eli_end();
    frame_end();

    eli_io_add_mouse_pos_event(c.x, c.y);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, false);
    frame_begin();
    open_win();
    eli_checkbox_flags_int("F", &flags, BIT);
    eli_end();
    frame_end();

    ELI_ASSERT_EQ(flags & BIT, BIT);
    widget_teardown(ctx);
}

ELI_TEST_MAIN()
