/**
 * @file test_layout_helpers.c
 * @brief Unit tests for layout-flow helpers: eli_same_line (follow and explicit
 *        offset), eli_spacing, eli_dummy, eli_indent / eli_unindent, eli_separator,
 *        and eli_begin_group / eli_end_group bounding-box sizing.
 *
 * @status Phase 8 layout-helpers coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/layout/eli_layout.h>

#define TEST_DT (1.0f / 60.0f)

static eli_context *layout_setup(void)
{
    eli_context *ctx = eli_create_context();
    ctx->io.delta_time = TEST_DT;
    ctx->io.display_size = eli_make_vec2(1024.0f, 768.0f);
    ctx->font_size = 13.0f;
    return ctx;
}

static void frame_begin(void)
{
    eli_new_frame();
    eli_input_update_begin_frame();
    eli_window_new_frame();
}

static void open_window(void)
{
    eli_set_next_window_pos(eli_make_vec2(100.0f, 100.0f), 0, eli_make_vec2(0.0f, 0.0f));
    eli_set_next_window_size(eli_make_vec2(400.0f, 300.0f), 0);
    eli_begin("HelperWin", NULL, 0);
}

ELI_TEST(same_line_follows_previous_item_by_spacing) {
    eli_context *ctx = layout_setup();
    frame_begin();
    open_window();

    /* item at x=108 width 50 => prev_line.x = 158. */
    eli_item_size(eli_make_vec2(50.0f, 20.0f), -1.0f);
    eli_same_line(0.0f, -1.0f);
    /* x = 158 + item_spacing.x(8) = 166; y returns to the item's line (127). */
    ELI_ASSERT_FLT_NEAR(eli_get_cursor_screen_pos().x, 166.0f, 0.01f);
    ELI_ASSERT_FLT_NEAR(eli_get_cursor_screen_pos().y, 127.0f, 0.01f);

    eli_end();
    eli_destroy_context(ctx);
}

ELI_TEST(same_line_explicit_offset) {
    eli_context *ctx = layout_setup();
    frame_begin();
    open_window();

    eli_item_size(eli_make_vec2(50.0f, 20.0f), -1.0f);
    eli_same_line(100.0f, 0.0f);
    /* x = pos.x(100) + offset(100) = 200. */
    ELI_ASSERT_FLT_NEAR(eli_get_cursor_screen_pos().x, 200.0f, 0.01f);
    ELI_ASSERT_FLT_NEAR(eli_get_cursor_screen_pos().y, 127.0f, 0.01f);

    eli_end();
    eli_destroy_context(ctx);
}

ELI_TEST(spacing_advances_by_item_spacing_y) {
    eli_context *ctx = layout_setup();
    frame_begin();
    open_window();

    float y0 = eli_get_cursor_screen_pos().y;
    eli_spacing();
    float y1 = eli_get_cursor_screen_pos().y;
    ELI_ASSERT_FLT_NEAR(y1 - y0, 4.0f, 0.01f);

    eli_end();
    eli_destroy_context(ctx);
}

ELI_TEST(dummy_advances_and_records_item) {
    eli_context *ctx = layout_setup();
    frame_begin();
    open_window();

    eli_vec2 at = eli_get_cursor_screen_pos();
    eli_dummy(eli_make_vec2(30.0f, 25.0f));
    eli_rect r = eli_get_item_rect();
    ELI_ASSERT_FLT_NEAR(r.x, at.x, 0.01f);
    ELI_ASSERT_FLT_NEAR(r.w, 30.0f, 0.01f);
    ELI_ASSERT_FLT_NEAR(r.h, 25.0f, 0.01f);
    /* Cursor advanced by 25 + spacing(4). */
    ELI_ASSERT_FLT_NEAR(eli_get_cursor_screen_pos().y - at.y, 29.0f, 0.01f);

    eli_end();
    eli_destroy_context(ctx);
}

ELI_TEST(indent_unindent_shift_cursor_x) {
    eli_context *ctx = layout_setup();
    frame_begin();
    open_window();

    float x0 = eli_get_cursor_pos_x();       /* 8 */
    eli_indent(0.0f);                         /* + indent_spacing(21) */
    ELI_ASSERT_FLT_NEAR(eli_get_cursor_pos_x(), x0 + 21.0f, 0.01f);
    /* A fresh item now starts at the indented x. */
    eli_item_size(eli_make_vec2(10.0f, 10.0f), -1.0f);
    ELI_ASSERT_FLT_NEAR(eli_get_cursor_pos_x(), x0 + 21.0f, 0.01f);
    eli_unindent(0.0f);
    ELI_ASSERT_FLT_NEAR(eli_get_cursor_pos_x(), x0, 0.01f);

    eli_end();
    eli_destroy_context(ctx);
}

ELI_TEST(separator_advances_by_thickness) {
    eli_context *ctx = layout_setup();
    frame_begin();
    open_window();

    float y0 = eli_get_cursor_screen_pos().y;
    eli_separator();
    float y1 = eli_get_cursor_screen_pos().y;
    /* thickness(1) + item_spacing.y(4). */
    ELI_ASSERT_FLT_NEAR(y1 - y0, 5.0f, 0.01f);

    eli_end();
    eli_destroy_context(ctx);
}

ELI_TEST(group_bounding_size_equals_enclosed_items) {
    eli_context *ctx = layout_setup();
    frame_begin();
    open_window();

    eli_begin_group();
    eli_dummy(eli_make_vec2(30.0f, 30.0f));   /* line 1: h=30 */
    eli_dummy(eli_make_vec2(50.0f, 10.0f));   /* line 2: w=50, h=10 */
    eli_end_group();

    /* Widest enclosed = 50; total height = 30 + spacing(4) + 10 = 44. */
    eli_vec2 sz = eli_get_item_rect_size();
    ELI_ASSERT_FLT_NEAR(sz.x, 50.0f, 0.01f);
    ELI_ASSERT_FLT_NEAR(sz.y, 44.0f, 0.01f);

    /* Group rect origin is where the group started (108, 127). */
    eli_rect r = eli_get_item_rect();
    ELI_ASSERT_FLT_NEAR(r.x, 108.0f, 0.01f);
    ELI_ASSERT_FLT_NEAR(r.y, 127.0f, 0.01f);

    eli_end();
    eli_destroy_context(ctx);
}

ELI_TEST(group_restores_cursor_and_advances_below) {
    eli_context *ctx = layout_setup();
    frame_begin();
    open_window();

    float y0 = eli_get_cursor_screen_pos().y;   /* 127 */
    eli_begin_group();
    eli_dummy(eli_make_vec2(30.0f, 30.0f));
    eli_dummy(eli_make_vec2(50.0f, 10.0f));
    eli_end_group();
    /* After the group, the cursor sits below it: y0 + group_h(44) + spacing(4). */
    ELI_ASSERT_FLT_NEAR(eli_get_cursor_screen_pos().y - y0, 48.0f, 0.01f);
    /* And back at the left origin. */
    ELI_ASSERT_FLT_NEAR(eli_get_cursor_screen_pos().x, 108.0f, 0.01f);

    eli_end();
    eli_destroy_context(ctx);
}

ELI_TEST_MAIN()
