/**
 * @file test_p30_demo.c
 * @brief Unit tests for Phase 30 demo window. A demo is visual, so these verify
 *        it RUNS: full eli_frame_begin / eli_show_demo_window / eli_frame_end
 *        cycles with a font atlas + dark style produce valid, non-empty draw
 *        geometry, closing via *p_open skips it, and clicking a collapsing
 *        header (driven through the input backend) toggles sections without
 *        crashing. Interaction is fed across frames as real mouse events.
 *
 * @status Phase 30 demo-window smoke coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/demo/eli_demo_all.h>
#include <eli/font/eli_font.h>

#define TEST_DT (1.0f / 60.0f)

static eli_font_atlas *g_atlas = NULL;

static eli_context *demo_setup(void)
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

static void demo_teardown(eli_context *ctx)
{
    eli_destroy_context(ctx);
    eli_font_atlas_destroy(g_atlas);
    g_atlas = NULL;
}

/* Run one demo frame with the window pinned at the origin so click coordinates
 * are deterministic. Returns the assembled draw data. */
static eli_draw_data *demo_frame(bool *p_open)
{
    eli_frame_begin();
    eli_set_next_window_pos(eli_make_vec2(0.0f, 0.0f), 0, eli_make_vec2(0.0f, 0.0f));
    eli_show_demo_window(p_open);
    eli_frame_end();
    return eli_get_draw_data();
}

/* ------------------------------------------------------------------------- */

ELI_TEST(demo_runs_and_produces_geometry) {
    eli_context *ctx = demo_setup();
    bool open = true;

    eli_draw_data *dd = NULL;
    for (int frame = 0; frame < 5; frame++)
        dd = demo_frame(&open);

    ELI_ASSERT_NOT_NULL(dd);
    ELI_ASSERT_TRUE(dd->valid);
    ELI_ASSERT_GT(dd->total_vtx_count, 0);
    ELI_ASSERT_GT(dd->total_idx_count, 0);
    ELI_ASSERT_GT(dd->cmd_lists_count, 0);
    ELI_ASSERT_TRUE(open);   /* never closed */

    demo_teardown(ctx);
}

ELI_TEST(demo_null_p_open_still_runs) {
    eli_context *ctx = demo_setup();

    eli_draw_data *dd = NULL;
    for (int frame = 0; frame < 3; frame++)
        dd = demo_frame(NULL);

    ELI_ASSERT_NOT_NULL(dd);
    ELI_ASSERT_GT(dd->total_vtx_count, 0);

    demo_teardown(ctx);
}

ELI_TEST(demo_closed_window_is_skipped) {
    eli_context *ctx = demo_setup();
    bool open = false;   /* already closed */

    eli_draw_data *dd = demo_frame(&open);

    /* With the only window skipped there is no window geometry to emit. */
    ELI_ASSERT_NOT_NULL(dd);
    ELI_ASSERT_EQ(dd->total_vtx_count, 0);
    ELI_ASSERT_FALSE(open);

    demo_teardown(ctx);
}

ELI_TEST(demo_click_headers_no_crash) {
    eli_context *ctx = demo_setup();
    bool open = true;

    /* Settle a few frames, then sweep a press+release down the header column.
     * Each click lands on or near one of the collapsing headers; toggling their
     * open state must never crash and must keep producing valid geometry. */
    for (int frame = 0; frame < 3; frame++)
        demo_frame(&open);

    for (float y = 40.0f; y <= 420.0f; y += 20.0f) {
        eli_io_add_mouse_pos_event(150.0f, y);
        eli_io_add_mouse_button_event(0, true);
        eli_draw_data *dd_press = demo_frame(&open);
        ELI_ASSERT_TRUE(dd_press->valid);

        eli_io_add_mouse_button_event(0, false);
        eli_draw_data *dd_release = demo_frame(&open);
        ELI_ASSERT_TRUE(dd_release->valid);
        ELI_ASSERT_GT(dd_release->total_vtx_count, 0);
    }

    demo_teardown(ctx);
}

ELI_TEST_MAIN()
