/**
 * @file test_p32_integration.c
 * @brief Phase 32 cross-subsystem integration tests. Verifies invariants that
 *        span all Phase 32 coverage areas — draw data stability, widget layout
 *        ordering, and window/widget bounding-box containment — to confirm that
 *        core types, ID hashing, draw list, layout, input, window, and widget
 *        subsystems compose correctly across a full frame lifecycle.
 *
 * @status Phase 32 integration coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/elimgui.h>

#define TEST_DT (1.0f / 60.0f)

static eli_font_atlas *g_atlas = NULL;

static eli_context *p32_setup(void)
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

static void p32_teardown(eli_context *ctx)
{
    eli_destroy_context(ctx);
    eli_font_atlas_destroy(g_atlas);
    g_atlas = NULL;
}

/* Emit a deterministic window: button + text + progress_bar + separator. */
static void emit_mixed_ui(void)
{
    eli_set_next_window_pos(eli_make_vec2(50.0f, 50.0f), 0, eli_make_vec2(0.0f, 0.0f));
    eli_set_next_window_size(eli_make_vec2(300.0f, 250.0f), 0);
    if (eli_begin("P32Win", NULL, 0)) {
        eli_button("Action");
        eli_text("Status: ok");
        eli_progress_bar(0.75f, eli_make_vec2(200.0f, 0.0f), NULL);
        eli_separator();
    }
    eli_end();
}

static void do_frame(void)
{
    eli_frame_begin();
    emit_mixed_ui();
    eli_frame_end();
}

/* -----------------------------------------------------------------------
 * Test 1: identical frame content produces stable draw data across frames.
 * Bridges: ID hashing -> layout -> draw list -> window management.
 * ----------------------------------------------------------------------- */

ELI_TEST(draw_data_stable_across_identical_frames) {
    eli_context *ctx = p32_setup();

    /* Frame 1 may have the "appearing" window state; run two warm-up frames. */
    do_frame();
    do_frame();

    /* Capture settled-state totals from frame 2. */
    eli_draw_data *dd = eli_get_draw_data();
    ELI_ASSERT_NOT_NULL(dd);
    ELI_ASSERT_TRUE(dd->valid);
    ELI_ASSERT_GT(dd->total_vtx_count, 0);
    ELI_ASSERT_GT(dd->total_idx_count, 0);
    int vtx_ref = dd->total_vtx_count;
    int idx_ref = dd->total_idx_count;

    /* Frames 3-5: same deterministic UI -> same vertex/index totals each time. */
    for (int i = 0; i < 3; i++) {
        do_frame();
        dd = eli_get_draw_data();
        ELI_ASSERT_EQ(dd->total_vtx_count, vtx_ref);
        ELI_ASSERT_EQ(dd->total_idx_count, idx_ref);
    }

    p32_teardown(ctx);
}

/* -----------------------------------------------------------------------
 * Test 2: widget item rects are vertically ordered and non-overlapping.
 * Bridges: layout system -> widget system -> item status queries.
 * ----------------------------------------------------------------------- */

ELI_TEST(widget_item_rects_ordered_vertically) {
    eli_context *ctx = p32_setup();

    eli_rect rects[3];

    eli_frame_begin();
    eli_set_next_window_pos(eli_make_vec2(0.0f, 0.0f), 0, eli_make_vec2(0.0f, 0.0f));
    eli_set_next_window_size(eli_make_vec2(300.0f, 300.0f), 0);
    eli_begin("Order", NULL, 0);

    eli_button("First");
    rects[0] = eli_get_item_rect();

    eli_text("Second");
    rects[1] = eli_get_item_rect();

    eli_dummy(eli_make_vec2(80.0f, 15.0f));
    rects[2] = eli_get_item_rect();

    eli_end();
    eli_frame_end();

    /* Each rect must start at or below the bottom edge of the previous one. */
    ELI_ASSERT_GE(rects[1].y, rects[0].y + rects[0].h);
    ELI_ASSERT_GE(rects[2].y, rects[1].y + rects[1].h);

    /* All rects have positive dimensions. */
    ELI_ASSERT_GT(rects[0].w, 0.0f);
    ELI_ASSERT_GT(rects[0].h, 0.0f);
    ELI_ASSERT_GT(rects[1].w, 0.0f);
    ELI_ASSERT_GT(rects[1].h, 0.0f);
    ELI_ASSERT_GT(rects[2].w, 0.0f);
    ELI_ASSERT_GT(rects[2].h, 0.0f);

    p32_teardown(ctx);
}

/* -----------------------------------------------------------------------
 * Test 3: widget item rects lie within the window's content region.
 * Bridges: window management -> layout calculations -> widget subsystems.
 * ----------------------------------------------------------------------- */

ELI_TEST(widget_rects_contained_in_window_content_region) {
    eli_context *ctx = p32_setup();

    eli_frame_begin();
    eli_set_next_window_pos(eli_make_vec2(100.0f, 100.0f), 0, eli_make_vec2(0.0f, 0.0f));
    eli_set_next_window_size(eli_make_vec2(250.0f, 200.0f), 0);
    eli_begin("Contain", NULL, 0);

    eli_window *w = eli_get_current_window();
    /* Capture content region while the window is open (set during eli_begin). */
    float cr_x = w->content_region_rect.x;
    float cr_y = w->content_region_rect.y;
    float cr_w = w->content_region_rect.w;

    eli_button("In");
    eli_rect br = eli_get_item_rect();

    eli_text("Also In");
    eli_rect tr = eli_get_item_rect();

    eli_end();
    eli_frame_end();

    /* Button left edge must be at or after the content region left edge. */
    ELI_ASSERT_GE(br.x, cr_x - 0.01f);
    /* Button right edge must be at or before the content region right edge. */
    ELI_ASSERT_LE(br.x + br.w, cr_x + cr_w + 0.01f);

    /* Text top must be at or after the content region top. */
    ELI_ASSERT_GE(tr.y, cr_y - 0.01f);
    /* Text left must be at or after the content region left. */
    ELI_ASSERT_GE(tr.x, cr_x - 0.01f);

    p32_teardown(ctx);
}

ELI_TEST_MAIN()
