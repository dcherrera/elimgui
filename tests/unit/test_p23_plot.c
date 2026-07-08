/**
 * @file test_p23_plot.c
 * @brief Unit tests for Phase 23 data-plotting widgets: eli_plot_lines,
 *        eli_plot_lines_fn, eli_plot_histogram, and eli_plot_histogram_fn.
 *
 * Tests verify:
 *   - plot_lines emits the correct vertex/index counts (frame bg + polyline).
 *   - auto-scale maps the data min to the inner-frame bottom and data max to top.
 *   - plot_histogram emits one filled rect per bar with heights proportional to values.
 *   - Explicit scale_min/max clamps the normalised value.
 *   - Overlay text does not crash (no-op when font is absent).
 *   - Item cursor advances by the requested graph_size after each plot call.
 *
 * Geometry assertions reference the draw-list vertex buffer directly. Each
 * test captures vtx_count before and after the plot call so the delta is
 * independent of whatever the window decorations already emitted.
 *
 * @status Phase 23 plotting widget coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/widgets/eli_plot.h>

#define TEST_DT  (1.0f / 60.0f)
/* Default window geometry with window at (0,0), size 400x300:
 *   title bar height = 19px (font_size=13 + 2*frame_padding.y=3),
 *   window_padding = (8,8), frame_padding = (4,3)
 *   cursor starts at screen (8, 27). */
#define WIN_X  0.0f
#define WIN_Y  0.0f
#define WIN_W  400.0f
#define WIN_H  300.0f

/* ---------------------------------------------------------------------------
 * Test fixtures
 * --------------------------------------------------------------------------- */

static eli_context *plot_setup(void)
{
    eli_context *ctx = eli_create_context();
    ctx->io.delta_time  = TEST_DT;
    ctx->io.display_size = eli_make_vec2(1024.0f, 768.0f);
    /* font_size drives default_h and item sizing; ctx->font stays NULL so
     * text-render calls are no-ops and don't add vertices. */
    ctx->font_size = 13.0f;
    /* Set non-zero style colors so draw calls (add_rect_filled, add_polyline)
     * actually emit geometry. Without this, eli_get_color_u32 returns 0 and
     * all draw primitives early-return on zero alpha. */
    eli_style_colors_dark(&ctx->style);
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

static void open_win(void)
{
    eli_set_next_window_pos(eli_make_vec2(WIN_X, WIN_Y), 0, eli_make_vec2(0.0f, 0.0f));
    eli_set_next_window_size(eli_make_vec2(WIN_W, WIN_H), 0);
    eli_begin("PlotWin", NULL, 0);
}

/* Tiny callback getter: returns (float)idx. */
static float idx_getter(void *data, int idx)
{
    (void)data;
    return (float)idx;
}

/* ---------------------------------------------------------------------------
 * plot_lines vertex/index count
 *
 * With N=5 samples (no border, no hover, no font):
 *   frame bg  : add_rect_filled → 4 vtx, 6 idx
 *   polyline  : N-1 = 4 segs → 4*4 = 16 vtx, 4*6 = 24 idx
 *   Total delta: 20 vtx, 30 idx
 * --------------------------------------------------------------------------- */

ELI_TEST(plot_lines_emits_polyline_geometry) {
    eli_context *ctx = plot_setup();
    frame_begin();
    open_win();

    eli_draw_list *dl = eli_get_window_draw_list();
    ELI_ASSERT_NOT_NULL(dl);
    uint32_t vtx_before = dl->vtx_count;
    uint32_t idx_before = dl->idx_count;

    float vals[5] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
    eli_plot_lines("##plines", vals, 5, 0, NULL,
                   ELI_PLOT_SCALE_AUTO, ELI_PLOT_SCALE_AUTO,
                   eli_make_vec2(200.0f, 100.0f), 0);

    /* frame bg: 4 vtx / 6 idx  +  polyline 4 segs: 16 vtx / 24 idx */
    ELI_ASSERT_EQ(dl->vtx_count - vtx_before, 20u);
    ELI_ASSERT_EQ(dl->idx_count - idx_before, 30u);

    eli_end();
    frame_end();
    eli_destroy_context(ctx);
}

/* ---------------------------------------------------------------------------
 * plot_lines_fn vertex/index count (same geometry via callback path)
 * --------------------------------------------------------------------------- */

ELI_TEST(plot_lines_fn_emits_same_geometry_as_array_variant) {
    eli_context *ctx = plot_setup();
    frame_begin();
    open_win();

    eli_draw_list *dl = eli_get_window_draw_list();
    ELI_ASSERT_NOT_NULL(dl);
    uint32_t vtx_before = dl->vtx_count;
    uint32_t idx_before = dl->idx_count;

    /* 5 samples via callback: same expected geometry as the array variant. */
    eli_plot_lines_fn("##plfn", idx_getter, NULL, 5, 0, NULL,
                      0.0f, 4.0f, eli_make_vec2(200.0f, 100.0f));

    ELI_ASSERT_EQ(dl->vtx_count - vtx_before, 20u);
    ELI_ASSERT_EQ(dl->idx_count - idx_before, 30u);

    eli_end();
    frame_end();
    eli_destroy_context(ctx);
}

/* ---------------------------------------------------------------------------
 * Auto-scale: min value maps to inner-frame bottom, max value maps to top.
 *
 * Frame bb = rect(8, 27, 200, 100) → fmin=(8,27), fmax=(208,127)
 * inner_min = (9, 28), inner_max = (207, 126), inner_h = 98
 *
 * 2 samples: [0.0, 1.0] with auto-scale → scale [0,1]
 *   pts[0] = (9,  126)  → y = inner_max.y  (value 0 → bottom)
 *   pts[1] = (207, 28)  → y = inner_min.y  (value 1 → top)
 *
 * After the frame-bg rect (4 vtx at baseline), the polyline emits 4 vtx:
 *   vtx[baseline+4].pos.y ≈ inner_max.y (near pt0)
 *   vtx[baseline+7].pos.y ≈ inner_max.y (near pt0, other side of strip)
 *   vtx[baseline+5].pos.y ≈ inner_min.y (near pt1)
 *   vtx[baseline+6].pos.y ≈ inner_min.y (near pt1, other side of strip)
 *
 * Tolerance is 1.5 px to account for the 1 px thickness normal expansion.
 * --------------------------------------------------------------------------- */

ELI_TEST(plot_lines_auto_scale_maps_min_to_bottom_max_to_top) {
    eli_context *ctx = plot_setup();
    frame_begin();
    open_win();

    eli_draw_list *dl = eli_get_window_draw_list();
    ELI_ASSERT_NOT_NULL(dl);
    uint32_t base = dl->vtx_count;

    float vals[2] = {0.0f, 1.0f};
    eli_plot_lines("##ascale", vals, 2, 0, NULL,
                   ELI_PLOT_SCALE_AUTO, ELI_PLOT_SCALE_AUTO,
                   eli_make_vec2(200.0f, 100.0f), 0);

    /* Expected inner geometry: inner_max.y = 126, inner_min.y = 28 */
    float inner_max_y = 126.0f;
    float inner_min_y = 28.0f;
    float tol = 1.5f;

    /* vtx[base+4] and [base+7] are the two vertices near pt0 (value=0, bottom). */
    ELI_ASSERT_FLT_NEAR(dl->vtx[base + 4].y, inner_max_y, tol);
    ELI_ASSERT_FLT_NEAR(dl->vtx[base + 7].y, inner_max_y, tol);

    /* vtx[base+5] and [base+6] are the two vertices near pt1 (value=1, top). */
    ELI_ASSERT_FLT_NEAR(dl->vtx[base + 5].y, inner_min_y, tol);
    ELI_ASSERT_FLT_NEAR(dl->vtx[base + 6].y, inner_min_y, tol);

    eli_end();
    frame_end();
    eli_destroy_context(ctx);
}

/* ---------------------------------------------------------------------------
 * plot_histogram vertex/index count
 *
 * With N=4 samples (no border, no hover, no font):
 *   frame bg  : 4 vtx, 6 idx
 *   N bars    : 4 * (4 vtx + 6 idx) = 16 vtx, 24 idx
 *   Total delta: 20 vtx, 30 idx
 *
 * With scale [0,4] and values [1,2,3,4], all bars have positive height so
 * none are skipped.
 * --------------------------------------------------------------------------- */

ELI_TEST(plot_histogram_emits_one_bar_per_sample) {
    eli_context *ctx = plot_setup();
    frame_begin();
    open_win();

    eli_draw_list *dl = eli_get_window_draw_list();
    ELI_ASSERT_NOT_NULL(dl);
    uint32_t vtx_before = dl->vtx_count;
    uint32_t idx_before = dl->idx_count;

    float vals[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    eli_plot_histogram("##phisto", vals, 4, 0, NULL,
                       0.0f, 4.0f,
                       eli_make_vec2(200.0f, 100.0f), 0);

    /* frame bg: 4 vtx / 6 idx  +  4 bars × 4 vtx / 6 idx each */
    ELI_ASSERT_EQ(dl->vtx_count - vtx_before, 20u);
    ELI_ASSERT_EQ(dl->idx_count - idx_before, 30u);

    eli_end();
    frame_end();
    eli_destroy_context(ctx);
}

/* ---------------------------------------------------------------------------
 * Histogram bar heights are proportional to values.
 *
 * With 2 bars, values [2, 4], scale [0, 4], graph (200,100):
 *   inner_min=(9,28), inner_max=(207,126), inner_h=98
 *   bar_w = 198/2 = 99
 *   bar 0 (v=2): norm=0.5, y_top = 126 - 0.5*98 = 77  → height = 49
 *   bar 1 (v=4): norm=1.0, y_top = 126 - 1.0*98 = 28  → height = 98
 *
 * From add_rect_filled vertex layout (TL, TR, BR, BL):
 *   After frame bg (4 vtx), bar 0 starts at base+4:
 *     vtx[base+4].pos.y  = y_top[0] ≈ 77  (TL)
 *     vtx[base+6].pos.y  = inner_max.y ≈ 126 (BR)
 *   bar 1 starts at base+8:
 *     vtx[base+8].pos.y  = y_top[1] ≈ 28  (TL)
 *     vtx[base+10].pos.y = inner_max.y ≈ 126 (BR)
 * --------------------------------------------------------------------------- */

ELI_TEST(plot_histogram_bar_heights_proportional_to_values) {
    eli_context *ctx = plot_setup();
    frame_begin();
    open_win();

    eli_draw_list *dl = eli_get_window_draw_list();
    ELI_ASSERT_NOT_NULL(dl);
    uint32_t base = dl->vtx_count;

    float vals[2] = {2.0f, 4.0f};
    eli_plot_histogram("##heights", vals, 2, 0, NULL,
                       0.0f, 4.0f,
                       eli_make_vec2(200.0f, 100.0f), 0);

    float inner_max_y = 126.0f;
    float tol = 1.0f;

    /* Bar 0 (v=2, half-height): TL y ≈ 77, BR y ≈ 126 */
    ELI_ASSERT_FLT_NEAR(dl->vtx[base + 4].y, 77.0f, tol);
    ELI_ASSERT_FLT_NEAR(dl->vtx[base + 6].y, inner_max_y, tol);

    /* Bar 1 (v=4, full height): TL y ≈ 28, BR y ≈ 126 */
    ELI_ASSERT_FLT_NEAR(dl->vtx[base + 8].y, 28.0f, tol);
    ELI_ASSERT_FLT_NEAR(dl->vtx[base + 10].y, inner_max_y, tol);

    /* Bar 1 must be taller (lower TL y-value) than bar 0. */
    ELI_ASSERT_LT(dl->vtx[base + 8].y, dl->vtx[base + 4].y);

    eli_end();
    frame_end();
    eli_destroy_context(ctx);
}

/* ---------------------------------------------------------------------------
 * Explicit scale_min/max clamps out-of-range values.
 *
 * With scale [0, 2] and values [4, 4] (both above scale_max):
 *   norm = (4-0)/2 = 2.0 → clamped to 1.0 → y_top = inner_min.y (top).
 * Both bars should render at the top of the inner frame.
 * --------------------------------------------------------------------------- */

ELI_TEST(plot_histogram_explicit_scale_clamps_above_max) {
    eli_context *ctx = plot_setup();
    frame_begin();
    open_win();

    eli_draw_list *dl = eli_get_window_draw_list();
    ELI_ASSERT_NOT_NULL(dl);
    uint32_t base = dl->vtx_count;

    float vals[2] = {4.0f, 4.0f};
    eli_plot_histogram("##clamp", vals, 2, 0, NULL,
                       0.0f, 2.0f,        /* explicit scale: max=2, values=4 → clamped */
                       eli_make_vec2(200.0f, 100.0f), 0);

    float inner_min_y = 28.0f;
    float tol = 1.0f;

    /* Both bars at full height → TL y ≈ inner_min.y. */
    ELI_ASSERT_FLT_NEAR(dl->vtx[base + 4].y, inner_min_y, tol);
    ELI_ASSERT_FLT_NEAR(dl->vtx[base + 8].y, inner_min_y, tol);

    eli_end();
    frame_end();
    eli_destroy_context(ctx);
}

/* ---------------------------------------------------------------------------
 * Overlay text: passing a non-NULL overlay_text must not crash and must not
 * add unexpected geometry (font is NULL so text is a no-op here).
 * The delta vertex count is still the same as without overlay text.
 * --------------------------------------------------------------------------- */

ELI_TEST(plot_lines_overlay_text_does_not_crash) {
    eli_context *ctx = plot_setup();
    frame_begin();
    open_win();

    eli_draw_list *dl = eli_get_window_draw_list();
    ELI_ASSERT_NOT_NULL(dl);
    uint32_t vtx_before = dl->vtx_count;

    float vals[3] = {1.0f, 2.0f, 3.0f};
    /* Pass a non-NULL overlay_text; font is NULL so render is a no-op. */
    eli_plot_lines("##overlay", vals, 3, 0, "peak",
                   ELI_PLOT_SCALE_AUTO, ELI_PLOT_SCALE_AUTO,
                   eli_make_vec2(200.0f, 100.0f), 0);

    /* frame bg: 4 vtx  +  polyline 2 segs: 8 vtx = 12 total */
    ELI_ASSERT_EQ(dl->vtx_count - vtx_before, 12u);

    eli_end();
    frame_end();
    eli_destroy_context(ctx);
}

/* ---------------------------------------------------------------------------
 * Item cursor advance: after eli_plot_lines the cursor must have moved down
 * by graph_size.y + item_spacing.y relative to its position before the call.
 * --------------------------------------------------------------------------- */

ELI_TEST(plot_lines_advances_cursor_by_graph_size) {
    eli_context *ctx = plot_setup();
    frame_begin();
    open_win();

    float y_before = eli_get_cursor_screen_pos().y;

    float vals[5] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
    eli_plot_lines("##advance", vals, 5, 0, NULL,
                   ELI_PLOT_SCALE_AUTO, ELI_PLOT_SCALE_AUTO,
                   eli_make_vec2(200.0f, 100.0f), 0);

    float y_after = eli_get_cursor_screen_pos().y;
    /* cursor advance = graph_size.y + item_spacing.y (4 by default) */
    ELI_ASSERT_FLT_NEAR(y_after - y_before, 100.0f + 4.0f, 0.5f);

    eli_end();
    frame_end();
    eli_destroy_context(ctx);
}

/* ---------------------------------------------------------------------------
 * Item cursor advance for plot_histogram.
 * --------------------------------------------------------------------------- */

ELI_TEST(plot_histogram_advances_cursor_by_graph_size) {
    eli_context *ctx = plot_setup();
    frame_begin();
    open_win();

    float y_before = eli_get_cursor_screen_pos().y;

    float vals[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    eli_plot_histogram("##hadvance", vals, 4, 0, NULL,
                       ELI_PLOT_SCALE_AUTO, ELI_PLOT_SCALE_AUTO,
                       eli_make_vec2(200.0f, 80.0f), 0);

    float y_after = eli_get_cursor_screen_pos().y;
    ELI_ASSERT_FLT_NEAR(y_after - y_before, 80.0f + 4.0f, 0.5f);

    eli_end();
    frame_end();
    eli_destroy_context(ctx);
}

/* ---------------------------------------------------------------------------
 * Callback (fn) variant of plot_histogram emits the same bar count.
 * --------------------------------------------------------------------------- */

ELI_TEST(plot_histogram_fn_emits_same_geometry_as_array_variant) {
    eli_context *ctx = plot_setup();
    frame_begin();
    open_win();

    eli_draw_list *dl = eli_get_window_draw_list();
    ELI_ASSERT_NOT_NULL(dl);
    uint32_t vtx_before = dl->vtx_count;
    uint32_t idx_before = dl->idx_count;

    /* 4 samples via callback: values 0,1,2,3; scale [0,3].
     * Value 0 → norm=0 → zero-height bar → skipped.
     * Values 1,2,3 → 3 bars. */
    eli_plot_histogram_fn("##hfn", idx_getter, NULL, 4, 0, NULL,
                          0.0f, 3.0f, eli_make_vec2(200.0f, 100.0f));

    /* frame bg: 4 vtx / 6 idx  +  3 visible bars × 4 vtx / 6 idx = 16 vtx / 24 idx
     * (bar at idx=0 has value=0 → zero-height → skipped) */
    ELI_ASSERT_EQ(dl->vtx_count - vtx_before, 16u);
    ELI_ASSERT_EQ(dl->idx_count - idx_before, 24u);

    eli_end();
    frame_end();
    eli_destroy_context(ctx);
}

/* ---------------------------------------------------------------------------
 * Null / zero-count guards: passing NULL values or values_count <= 0 must not
 * crash and must leave the draw list and cursor unchanged.
 * --------------------------------------------------------------------------- */

ELI_TEST(plot_lines_null_values_is_safe) {
    eli_context *ctx = plot_setup();
    frame_begin();
    open_win();

    eli_draw_list *dl = eli_get_window_draw_list();
    uint32_t vtx_before = dl->vtx_count;
    float y_before = eli_get_cursor_screen_pos().y;

    /* NULL values: should no-op. */
    eli_plot_lines("##null", NULL, 5, 0, NULL,
                   ELI_PLOT_SCALE_AUTO, ELI_PLOT_SCALE_AUTO,
                   eli_make_vec2(200.0f, 100.0f), 0);
    /* values_count == 0: should no-op. */
    float dummy = 1.0f;
    eli_plot_lines("##zero", &dummy, 0, 0, NULL,
                   ELI_PLOT_SCALE_AUTO, ELI_PLOT_SCALE_AUTO,
                   eli_make_vec2(200.0f, 100.0f), 0);

    ELI_ASSERT_EQ(dl->vtx_count, vtx_before);
    ELI_ASSERT_FLT_NEAR(eli_get_cursor_screen_pos().y, y_before, 0.01f);

    eli_end();
    frame_end();
    eli_destroy_context(ctx);
}

ELI_TEST_MAIN()
