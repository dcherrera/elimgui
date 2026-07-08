/**
 * @file test_layout_item.c
 * @brief Unit tests for the item-layout core: eli_item_size cursor advancement
 *        (by size + item spacing), eli_item_add last-item id/rect recording, the
 *        cursor get/set API, and content-region queries.
 *
 * @status Phase 8 item-layout coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/layout/eli_layout.h>

#define TEST_DT (1.0f / 60.0f)

/* Window geometry with default style (title=19, pad=8):
 *   pos=(100,100) size=(400,300); no scrollbars (no content).
 *   cursor_start_pos = (108, 127); indent = 8; item_width_default = 260. */
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
    eli_begin("LayoutWin", NULL, 0);
}

ELI_TEST(item_size_advances_cursor_by_size_plus_spacing) {
    eli_context *ctx = layout_setup();
    frame_begin();
    open_window();

    /* Cursor seeded at the work-area origin. */
    ELI_ASSERT_FLT_NEAR(eli_get_cursor_screen_pos().x, 108.0f, 0.01f);
    ELI_ASSERT_FLT_NEAR(eli_get_cursor_screen_pos().y, 127.0f, 0.01f);

    float y0 = eli_get_cursor_screen_pos().y;
    eli_item_size(eli_make_vec2(50.0f, 20.0f), -1.0f);
    float y1 = eli_get_cursor_screen_pos().y;

    /* Advanced by item height (20) + item_spacing.y (4). */
    ELI_ASSERT_FLT_NEAR(y1 - y0, 24.0f, 0.01f);
    /* Next line returns to the left origin. */
    ELI_ASSERT_FLT_NEAR(eli_get_cursor_screen_pos().x, 108.0f, 0.01f);

    eli_end();
    eli_destroy_context(ctx);
}

ELI_TEST(item_add_sets_last_item_id_and_rect) {
    eli_context *ctx = layout_setup();
    frame_begin();
    open_window();

    eli_rect bb = eli_make_rect(110.0f, 130.0f, 40.0f, 18.0f);
    bool visible = eli_item_add(0x1234u, bb, 0);
    ELI_ASSERT_TRUE(visible);
    ELI_ASSERT_EQ(eli_get_item_id(), 0x1234u);

    eli_rect got = eli_get_item_rect();
    ELI_ASSERT_FLT_NEAR(got.x, 110.0f, 0.01f);
    ELI_ASSERT_FLT_NEAR(got.y, 130.0f, 0.01f);
    ELI_ASSERT_FLT_NEAR(got.w, 40.0f, 0.01f);
    ELI_ASSERT_FLT_NEAR(got.h, 18.0f, 0.01f);
    ELI_ASSERT_FLT_NEAR(eli_get_item_rect_size().x, 40.0f, 0.01f);

    eli_end();
    eli_destroy_context(ctx);
}

ELI_TEST(cursor_set_get_roundtrip_local) {
    eli_context *ctx = layout_setup();
    frame_begin();
    open_window();

    /* Initial local cursor = start - pos = (8, 27). */
    ELI_ASSERT_FLT_NEAR(eli_get_cursor_pos_x(), 8.0f, 0.01f);
    ELI_ASSERT_FLT_NEAR(eli_get_cursor_pos_y(), 27.0f, 0.01f);

    eli_set_cursor_pos(eli_make_vec2(40.0f, 60.0f));
    ELI_ASSERT_FLT_NEAR(eli_get_cursor_pos().x, 40.0f, 0.01f);
    ELI_ASSERT_FLT_NEAR(eli_get_cursor_pos().y, 60.0f, 0.01f);
    /* Screen pos = pos + local (no scroll). */
    ELI_ASSERT_FLT_NEAR(eli_get_cursor_screen_pos().x, 140.0f, 0.01f);
    ELI_ASSERT_FLT_NEAR(eli_get_cursor_screen_pos().y, 160.0f, 0.01f);

    eli_set_cursor_screen_pos(eli_make_vec2(200.0f, 220.0f));
    ELI_ASSERT_FLT_NEAR(eli_get_cursor_screen_pos().x, 200.0f, 0.01f);
    ELI_ASSERT_FLT_NEAR(eli_get_cursor_pos_x(), 100.0f, 0.01f);

    eli_end();
    eli_destroy_context(ctx);
}

ELI_TEST(cursor_start_pos_is_body_origin) {
    eli_context *ctx = layout_setup();
    frame_begin();
    open_window();

    eli_vec2 start = eli_get_cursor_start_pos();
    ELI_ASSERT_FLT_NEAR(start.x, 8.0f, 0.01f);
    ELI_ASSERT_FLT_NEAR(start.y, 27.0f, 0.01f);

    eli_end();
    eli_destroy_context(ctx);
}

ELI_TEST(content_region_avail_shrinks_as_cursor_advances) {
    eli_context *ctx = layout_setup();
    frame_begin();
    open_window();

    /* avail_w = 400-16 = 384; avail_h = (300-19)-16 = 265. */
    eli_vec2 a0 = eli_get_content_region_avail();
    ELI_ASSERT_FLT_NEAR(a0.x, 384.0f, 0.5f);
    ELI_ASSERT_FLT_NEAR(a0.y, 265.0f, 0.5f);

    eli_item_size(eli_make_vec2(50.0f, 20.0f), -1.0f);
    eli_vec2 a1 = eli_get_content_region_avail();
    ELI_ASSERT_FLT_NEAR(a1.y, 265.0f - 24.0f, 0.5f);
    ELI_ASSERT_LT(a1.y, a0.y);

    /* Window-local content region bounds. */
    eli_vec2 rmin = eli_get_window_content_region_min();
    eli_vec2 rmax = eli_get_window_content_region_max();
    ELI_ASSERT_FLT_NEAR(rmin.x, 8.0f, 0.5f);
    ELI_ASSERT_FLT_NEAR(rmax.x, 392.0f, 0.5f);

    eli_end();
    eli_destroy_context(ctx);
}

ELI_TEST(item_size_rect_matches_item_size) {
    eli_context *ctx = layout_setup();
    frame_begin();
    open_window();

    float y0 = eli_get_cursor_screen_pos().y;
    eli_item_size_rect(eli_make_rect(0.0f, 0.0f, 30.0f, 15.0f), -1.0f);
    float y1 = eli_get_cursor_screen_pos().y;
    ELI_ASSERT_FLT_NEAR(y1 - y0, 15.0f + 4.0f, 0.01f);

    eli_end();
    eli_destroy_context(ctx);
}

ELI_TEST_MAIN()
