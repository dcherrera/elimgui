/**
 * @file test_p27_util.c
 * @brief Phase 27 misc-utility coverage: time/frame accessors, the main
 *        viewport, rectangle-visibility queries, and the background/foreground
 *        draw lists (lazy allocation, per-frame clearing, draw-data ordering,
 *        and clean shutdown).
 *
 * @status Phase 27 unit coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/elimgui.h>

#define TEST_DT (1.0f / 60.0f)

static eli_font_atlas *g_atlas = NULL;

static eli_context *p27_setup(void)
{
    eli_context *ctx = eli_create_context();
    ctx->io.delta_time = TEST_DT;
    ctx->io.display_size = eli_make_vec2(1024.0f, 768.0f);
    eli_style_colors_dark(&ctx->style);

    g_atlas = eli_font_atlas_create();
    eli_font *f = eli_font_atlas_add_font_default(g_atlas, NULL);
    eli_font_atlas_build(g_atlas);
    eli_push_font(f);
    return ctx;
}

static void p27_teardown(eli_context *ctx)
{
    eli_destroy_context(ctx);
    eli_font_atlas_destroy(g_atlas);
    g_atlas = NULL;
}

/* ---------------------------------------------------------------------------
 * Time & frame
 * ------------------------------------------------------------------------- */

ELI_TEST(time_and_frame_count_advance) {
    eli_context *ctx = p27_setup();

    ELI_ASSERT_EQ(eli_get_frame_count(), 0);
    ELI_ASSERT_FLT_NEAR(eli_get_time(), 0.0, 1e-9);

    for (int i = 1; i <= 4; i++) {
        eli_frame_begin();
        eli_frame_end();
        ELI_ASSERT_EQ(eli_get_frame_count(), i);
        ELI_ASSERT_FLT_NEAR(eli_get_time(), (double)i * (double)TEST_DT, 1e-4);
    }

    p27_teardown(ctx);
}

/* ---------------------------------------------------------------------------
 * Viewport
 * ------------------------------------------------------------------------- */

ELI_TEST(main_viewport_matches_display_size) {
    eli_context *ctx = p27_setup();

    eli_viewport *vp = eli_get_main_viewport();
    ELI_ASSERT_NOT_NULL(vp);
    ELI_ASSERT_FLT_NEAR(vp->pos.x, 0.0f, 1e-6f);
    ELI_ASSERT_FLT_NEAR(vp->pos.y, 0.0f, 1e-6f);
    ELI_ASSERT_FLT_NEAR(vp->size.x, 1024.0f, 1e-6f);
    ELI_ASSERT_FLT_NEAR(vp->size.y, 768.0f, 1e-6f);
    /* Work area equals the full viewport for now. */
    ELI_ASSERT_FLT_NEAR(vp->work_pos.x, 0.0f, 1e-6f);
    ELI_ASSERT_FLT_NEAR(vp->work_size.x, 1024.0f, 1e-6f);
    ELI_ASSERT_FLT_NEAR(vp->work_size.y, 768.0f, 1e-6f);

    /* Refreshes on display resize. */
    ctx->io.display_size = eli_make_vec2(640.0f, 480.0f);
    vp = eli_get_main_viewport();
    ELI_ASSERT_FLT_NEAR(vp->size.x, 640.0f, 1e-6f);
    ELI_ASSERT_FLT_NEAR(vp->size.y, 480.0f, 1e-6f);

    p27_teardown(ctx);
}

/* ---------------------------------------------------------------------------
 * Visibility
 * ------------------------------------------------------------------------- */

ELI_TEST(is_rect_visible_inside_and_outside_window) {
    eli_context *ctx = p27_setup();

    eli_frame_begin();
    eli_set_next_window_pos(eli_make_vec2(100.0f, 100.0f), 0, eli_make_vec2(0.0f, 0.0f));
    eli_set_next_window_size(eli_make_vec2(300.0f, 200.0f), 0);
    bool visible_inside = false;
    bool visible_far = true;
    if (eli_begin("Vis", NULL, 0)) {
        /* A small rect at the cursor sits inside the window's clip rect. */
        visible_inside = eli_is_rect_visible(eli_make_vec2(20.0f, 20.0f));
        /* A rect far below/right of the window is fully outside its clip rect. */
        visible_far = eli_is_rect_visible_vec2(eli_make_vec2(5000.0f, 5000.0f),
                                               eli_make_vec2(5020.0f, 5020.0f));
    }
    eli_end();
    eli_frame_end();

    ELI_ASSERT_TRUE(visible_inside);
    ELI_ASSERT_FALSE(visible_far);

    /* No current window -> not visible. */
    ELI_ASSERT_FALSE(eli_is_rect_visible(eli_make_vec2(10.0f, 10.0f)));

    p27_teardown(ctx);
}

/* ---------------------------------------------------------------------------
 * Background / foreground draw lists
 * ------------------------------------------------------------------------- */

/* Find the index of a draw list inside the assembled draw data, or -1. */
static int p27_index_of(const eli_draw_data *dd, const eli_draw_list *dl)
{
    for (int i = 0; i < dd->cmd_lists_count; i++)
        if (dd->cmd_lists[i] == dl)
            return i;
    return -1;
}

ELI_TEST(background_first_foreground_last_in_draw_data) {
    eli_context *ctx = p27_setup();

    eli_frame_begin();

    eli_draw_list *bg = eli_get_background_draw_list();
    eli_draw_list *fg = eli_get_foreground_draw_list();
    ELI_ASSERT_NOT_NULL(bg);
    ELI_ASSERT_NOT_NULL(fg);
    /* Lazy allocation also registered the shutdown hook. */
    ELI_ASSERT_NOT_NULL(ctx->util_shutdown_fn);

    eli_draw_list_add_rect_filled(bg, eli_make_vec2(0, 0), eli_make_vec2(50, 50),
                                  0xFF0000FFu, 0.0f, 0);
    eli_draw_list_add_rect_filled(fg, eli_make_vec2(0, 0), eli_make_vec2(50, 50),
                                  0xFF00FF00u, 0.0f, 0);

    /* A window in between so ordering is meaningful. */
    eli_set_next_window_pos(eli_make_vec2(10.0f, 10.0f), 0, eli_make_vec2(0.0f, 0.0f));
    eli_set_next_window_size(eli_make_vec2(200.0f, 150.0f), 0);
    if (eli_begin("Mid", NULL, 0))
        eli_button("Click");
    eli_end();

    eli_frame_end();

    eli_draw_data *dd = eli_get_draw_data();
    ELI_ASSERT_NOT_NULL(dd);
    ELI_ASSERT_TRUE(dd->valid);
    ELI_ASSERT_GE(dd->cmd_lists_count, 3);

    int bg_idx = p27_index_of(dd, bg);
    int fg_idx = p27_index_of(dd, fg);
    ELI_ASSERT_EQ(bg_idx, 0);
    ELI_ASSERT_EQ(fg_idx, dd->cmd_lists_count - 1);
    /* The window sits strictly between background and foreground. */
    ELI_ASSERT_LT(bg_idx, fg_idx);

    p27_teardown(ctx);
}

ELI_TEST(util_draw_lists_cleared_between_frames) {
    eli_context *ctx = p27_setup();

    eli_frame_begin();
    eli_draw_list *bg = eli_get_background_draw_list();
    eli_draw_list_add_rect_filled(bg, eli_make_vec2(0, 0), eli_make_vec2(50, 50),
                                  0xFFFFFFFFu, 0.0f, 0);
    ELI_ASSERT_GT((int)bg->idx_count, 0);
    eli_frame_end();

    /* Next frame begins by clearing the persistent lists. */
    eli_frame_begin();
    ELI_ASSERT_EQ((int)bg->idx_count, 0);
    ELI_ASSERT_EQ((int)bg->vtx_count, 0);
    eli_frame_end();

    /* An empty background list contributes nothing to draw data. */
    eli_draw_data *dd = eli_get_draw_data();
    ELI_ASSERT_EQ(p27_index_of(dd, bg), -1);

    p27_teardown(ctx);
}

ELI_TEST(util_shutdown_frees_draw_lists) {
    eli_context *ctx = p27_setup();

    eli_frame_begin();
    (void)eli_get_background_draw_list();
    (void)eli_get_foreground_draw_list();
    eli_frame_end();

    ELI_ASSERT_NOT_NULL(ctx->background_draw_list);
    ELI_ASSERT_NOT_NULL(ctx->foreground_draw_list);

    /* Destroy runs util_shutdown_fn; the lists are freed and nulled. No leak or
     * crash under the runner's clean-exit accounting. */
    p27_teardown(ctx);
}

ELI_TEST_MAIN()
