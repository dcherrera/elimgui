/**
 * @file test_window_lifecycle.c
 * @brief Unit tests for window creation/retrieval, next-window pos/size, the
 *        position/size queries, work-region insets, appearing, and collapse.
 *
 * @status Phase 7 window lifecycle coverage.
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
    eli_style_colors_dark(&ctx->style); /* non-zero colors so decorations emit geometry */
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

ELI_TEST(window_create_and_retrieve_same_id) {
    eli_context *ctx = win_setup();

    frame_begin();
    ELI_ASSERT_TRUE(eli_begin("Alpha", NULL, 0));
    eli_window *w1 = eli_get_current_window();
    ELI_ASSERT_NOT_NULL(w1);
    eli_id id1 = w1->id;
    ELI_ASSERT_TRUE(eli_is_window_appearing());
    eli_end();
    frame_end();

    /* Second frame: same name resolves to the same window instance/id. */
    frame_begin();
    ELI_ASSERT_TRUE(eli_begin("Alpha", NULL, 0));
    eli_window *w2 = eli_get_current_window();
    ELI_ASSERT_EQ(w1, w2);
    ELI_ASSERT_EQ(id1, w2->id);
    ELI_ASSERT_FALSE(eli_is_window_appearing());
    eli_end();
    frame_end();

    ELI_ASSERT_EQ(eli_find_window_by_name(ctx, "Alpha"), w1);
    ELI_ASSERT_EQ(ctx->windows_count, 1);
    eli_destroy_context(ctx);
}

ELI_TEST(window_next_pos_size_applied) {
    eli_context *ctx = win_setup();

    frame_begin();
    eli_set_next_window_pos(eli_make_vec2(100.0f, 120.0f), 0, eli_make_vec2(0.0f, 0.0f));
    eli_set_next_window_size(eli_make_vec2(200.0f, 150.0f), 0);
    ELI_ASSERT_TRUE(eli_begin("Sized", NULL, 0));

    ELI_ASSERT_FLT_NEAR(eli_get_window_pos().x, 100.0f, 0.01f);
    ELI_ASSERT_FLT_NEAR(eli_get_window_pos().y, 120.0f, 0.01f);
    ELI_ASSERT_FLT_NEAR(eli_get_window_width(), 200.0f, 0.01f);
    ELI_ASSERT_FLT_NEAR(eli_get_window_height(), 150.0f, 0.01f);
    ELI_ASSERT_FLT_NEAR(eli_get_window_size().x, 200.0f, 0.01f);

    eli_end();
    frame_end();
    eli_destroy_context(ctx);
}

ELI_TEST(window_next_pos_pivot_center) {
    eli_context *ctx = win_setup();

    frame_begin();
    eli_set_next_window_size(eli_make_vec2(200.0f, 100.0f), 0);
    eli_set_next_window_pos(eli_make_vec2(500.0f, 400.0f), 0, eli_make_vec2(0.5f, 0.5f));
    ELI_ASSERT_TRUE(eli_begin("Pivot", NULL, 0));
    /* Center pivot: pos = center - size/2. */
    ELI_ASSERT_FLT_NEAR(eli_get_window_pos().x, 400.0f, 0.01f);
    ELI_ASSERT_FLT_NEAR(eli_get_window_pos().y, 350.0f, 0.01f);
    eli_end();
    frame_end();
    eli_destroy_context(ctx);
}

ELI_TEST(window_work_region_insets_by_padding) {
    eli_context *ctx = win_setup();

    frame_begin();
    eli_set_next_window_pos(eli_make_vec2(0.0f, 0.0f), 0, eli_make_vec2(0.0f, 0.0f));
    eli_set_next_window_size(eli_make_vec2(200.0f, 150.0f), 0);
    ELI_ASSERT_TRUE(eli_begin("Work", NULL, 0));

    eli_window *w = eli_get_current_window();
    float pad = ctx->style.window_padding.x; /* 8 */
    float tbh = w->title_bar_height;          /* 13 + 3*2 = 19 */
    ELI_ASSERT_FLT_NEAR(tbh, 19.0f, 0.01f);
    /* No content => no scrollbars: work width = size.x - 2*pad. */
    ELI_ASSERT_FLT_NEAR(w->content_region_rect.w, 200.0f - 2.0f * pad, 0.01f);
    /* work height = (size.y - title) - 2*pad. */
    ELI_ASSERT_FLT_NEAR(w->content_region_rect.h, (150.0f - tbh) - 2.0f * pad, 0.01f);
    /* work origin sits below the title bar, inset by padding. */
    ELI_ASSERT_FLT_NEAR(w->content_region_rect.x, pad, 0.01f);
    ELI_ASSERT_FLT_NEAR(w->content_region_rect.y, tbh + pad, 0.01f);

    eli_end();
    frame_end();
    eli_destroy_context(ctx);
}

ELI_TEST(window_collapse_skips_body) {
    eli_context *ctx = win_setup();

    frame_begin();
    eli_set_next_window_size(eli_make_vec2(200.0f, 150.0f), 0);
    eli_set_next_window_collapsed(true, 0);
    /* Collapsed window: Begin returns false (skip body) but End is still required. */
    ELI_ASSERT_FALSE(eli_begin("Coll", NULL, 0));
    ELI_ASSERT_TRUE(eli_is_window_collapsed());
    eli_window *w = eli_get_current_window();
    ELI_ASSERT_FLT_NEAR(w->size.y, w->title_bar_height, 0.01f);
    ELI_ASSERT_TRUE(w->skip_items);
    eli_end();
    frame_end();

    /* Un-collapse next frame: body returns. */
    frame_begin();
    eli_set_next_window_collapsed(false, 0);
    ELI_ASSERT_TRUE(eli_begin("Coll", NULL, 0));
    ELI_ASSERT_FALSE(eli_is_window_collapsed());
    ELI_ASSERT_FLT_NEAR(eli_get_window_height(), 150.0f, 0.01f);
    eli_end();
    frame_end();
    eli_destroy_context(ctx);
}

ELI_TEST(window_no_titlebar_has_no_title_height) {
    eli_context *ctx = win_setup();

    frame_begin();
    eli_set_next_window_pos(eli_make_vec2(0.0f, 0.0f), 0, eli_make_vec2(0.0f, 0.0f));
    eli_set_next_window_size(eli_make_vec2(120.0f, 80.0f), 0);
    ELI_ASSERT_TRUE(eli_begin("Bare", NULL, ELI_WINDOW_NO_TITLEBAR));
    eli_window *w = eli_get_current_window();
    ELI_ASSERT_FLT_NEAR(w->title_bar_height, 0.0f, 0.01f);
    /* No title bar: work origin is inset from the window top by padding only. */
    ELI_ASSERT_FLT_NEAR(w->content_region_rect.y, ctx->style.window_padding.y, 0.01f);
    eli_end();
    frame_end();
    eli_destroy_context(ctx);
}

ELI_TEST(window_draw_data_gathers_active_windows) {
    eli_context *ctx = win_setup();

    frame_begin();
    eli_set_next_window_size(eli_make_vec2(100.0f, 100.0f), 0);
    eli_begin("One", NULL, 0);
    eli_end();
    eli_set_next_window_size(eli_make_vec2(100.0f, 100.0f), 0);
    eli_begin("Two", NULL, 0);
    eli_end();
    frame_end();

    eli_draw_data *dd = eli_get_draw_data();
    ELI_ASSERT_NOT_NULL(dd);
    ELI_ASSERT_TRUE(dd->valid);
    ELI_ASSERT_EQ(dd->cmd_lists_count, 2);
    ELI_ASSERT_GT(dd->total_vtx_count, 0);
    ELI_ASSERT_FLT_NEAR(dd->display_size.x, 1024.0f, 0.01f);
    eli_destroy_context(ctx);
}

ELI_TEST_MAIN()
