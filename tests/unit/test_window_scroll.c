/**
 * @file test_window_scroll.c
 * @brief Unit tests for window scrolling: scrollbar presence and scroll-range
 *        computation from content size, scroll clamping, set_scroll_x/y,
 *        set_scroll_from_pos_y, and mouse-wheel scrolling of the hovered window.
 *
 * @status Phase 7 window scrolling coverage.
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

/* Open a 200x150 window at (100,120) with a 400x900 content region. Geometry:
 *   title = 19, pad = 8, inner_h = 131, avail_h = 115, avail_w(pre) = 184.
 *   content.y 900 > 115 => vertical scrollbar; avail_w = 170.
 *   scroll_max.y = 900 - 115 = 785; scroll_max.x = 400 - 170 = 230. */
static void open_scroll_window(void)
{
    eli_set_next_window_pos(eli_make_vec2(100.0f, 120.0f), 0, eli_make_vec2(0.0f, 0.0f));
    eli_set_next_window_size(eli_make_vec2(200.0f, 150.0f), 0);
    eli_set_next_window_content_size(eli_make_vec2(400.0f, 900.0f));
    eli_begin("Scroller", NULL, 0);
}

ELI_TEST(window_scroll_range_from_content) {
    eli_context *ctx = win_setup();

    frame_begin();
    open_scroll_window();
    eli_window *w = eli_get_current_window();
    ELI_ASSERT_TRUE(w->has_scrollbar_y);
    ELI_ASSERT_FALSE(w->has_scrollbar_x); /* no horizontal-scrollbar flag */
    ELI_ASSERT_FLT_NEAR(eli_get_scroll_max_y(), 785.0f, 0.5f);
    ELI_ASSERT_FLT_NEAR(eli_get_scroll_max_x(), 230.0f, 0.5f);
    eli_end();
    frame_end();
    eli_destroy_context(ctx);
}

ELI_TEST(window_scroll_set_and_clamp) {
    eli_context *ctx = win_setup();

    frame_begin();
    open_scroll_window();
    eli_set_scroll_y(10000.0f);
    ELI_ASSERT_FLT_NEAR(eli_get_scroll_y(), 785.0f, 0.5f); /* clamped to max */
    eli_set_scroll_y(-50.0f);
    ELI_ASSERT_FLT_NEAR(eli_get_scroll_y(), 0.0f, 0.5f);   /* clamped to 0 */
    eli_set_scroll_y(300.0f);
    ELI_ASSERT_FLT_NEAR(eli_get_scroll_y(), 300.0f, 0.5f);
    eli_set_scroll_x(100.0f);
    ELI_ASSERT_FLT_NEAR(eli_get_scroll_x(), 100.0f, 0.5f);
    eli_set_scroll_x(5000.0f);
    ELI_ASSERT_FLT_NEAR(eli_get_scroll_x(), 230.0f, 0.5f);
    eli_end();
    frame_end();
    eli_destroy_context(ctx);
}

ELI_TEST(window_scroll_from_pos_y) {
    eli_context *ctx = win_setup();

    frame_begin();
    open_scroll_window();
    /* deco_top = content_region.y - pos.y = (title+pad) = 27. With scroll.y=0 and
     * ratio 0, target = local_y - 27. local_y = 500 => scroll.y = 473. */
    eli_set_scroll_y(0.0f);
    eli_set_scroll_from_pos_y(500.0f, 0.0f);
    ELI_ASSERT_FLT_NEAR(eli_get_scroll_y(), 473.0f, 0.5f);
    eli_end();
    frame_end();
    eli_destroy_context(ctx);
}

ELI_TEST(window_mouse_wheel_scrolls_hovered) {
    eli_context *ctx = win_setup();

    /* Frame 1: create the window and place the mouse inside it. */
    eli_io_add_mouse_pos_event(150.0f, 200.0f);
    frame_begin();
    open_scroll_window();
    eli_end();
    frame_end();

    /* Frame 2: hovered window is resolved from last frame; wheel down scrolls. */
    eli_io_add_mouse_pos_event(150.0f, 200.0f);
    eli_io_add_mouse_wheel_event(0.0f, -1.0f);
    frame_begin();
    ELI_ASSERT_EQ(ctx->hovered_window, eli_find_window_by_name(ctx, "Scroller"));
    open_scroll_window();
    /* step = 5 * 13 = 65; wheel -1 => scroll.y += 65. */
    ELI_ASSERT_FLT_NEAR(eli_get_scroll_y(), 65.0f, 0.5f);
    eli_end();
    frame_end();
    eli_destroy_context(ctx);
}

ELI_TEST(window_horizontal_scrollbar_flag) {
    eli_context *ctx = win_setup();

    frame_begin();
    eli_set_next_window_pos(eli_make_vec2(0.0f, 0.0f), 0, eli_make_vec2(0.0f, 0.0f));
    eli_set_next_window_size(eli_make_vec2(200.0f, 150.0f), 0);
    eli_set_next_window_content_size(eli_make_vec2(400.0f, 50.0f));
    eli_begin("HScroll", NULL, ELI_WINDOW_HORIZONTAL_SCROLLBAR);
    eli_window *w = eli_get_current_window();
    ELI_ASSERT_TRUE(w->has_scrollbar_x);
    ELI_ASSERT_FALSE(w->has_scrollbar_y); /* content.y small */
    ELI_ASSERT_GT(eli_get_scroll_max_x(), 0.0f);
    eli_end();
    frame_end();
    eli_destroy_context(ctx);
}

ELI_TEST_MAIN()
