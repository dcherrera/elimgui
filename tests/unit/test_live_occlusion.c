/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file test_live_occlusion.c
 * @brief Live widget occlusion and clipping: a window or the foreground list drawn
 *        over a widget hides its layer and the main surface draws it static from
 *        the next frame (no timed redraw); uncovered, the static copy goes first and
 *        the layer shows one frame later. Clipping cuts without hiding; a widget
 *        clipped out is hidden without a static copy.
 *
 * @status Live widgets coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"
#include "eli_test_live.h"

static const eli_live_pulse k_pulse = { 1.4f, 0.3f };
static eli_vec2 g_cover_pos = { 500.0f, 400.0f };   /* window B: far away */
static bool g_fg_cover = false;
static eli_rect g_dot_rect = { 60.0f, 80.0f, 10.0f, 10.0f };
static eli_id g_dot = 0;

static void ui(void)
{
    test_live_begin("A", eli_make_vec2(20, 20), eli_make_vec2(300, 200));
    g_dot = eli_get_id("dot");
    eli_live_dot_at(g_dot, g_dot_rect, ELI_COL32(80, 200, 120, 255), &k_pulse);
    eli_end();
    test_live_begin("B", g_cover_pos, eli_make_vec2(120, 80));   /* created later: on top */
    eli_text("cover");
    eli_end();
    if (g_fg_cover)
        eli_draw_list_add_rect_filled(eli_get_foreground_draw_list(), eli_make_vec2(55, 75),
                                      eli_make_vec2(90, 95), ELI_COL32_WHITE, 0.0f,
                                      ELI_DRAW_ROUND_CORNERS_ALL);
}

static eli_context *setup(void)
{
    eli_context *ctx = test_ui_setup();
    test_live_install();
    g_cover_pos = eli_make_vec2(500.0f, 400.0f);
    g_fg_cover = false;
    g_dot_rect = eli_make_rect(60.0f, 80.0f, 10.0f, 10.0f);
    test_ui_frames(ui, 5);
    return ctx;
}

static uint32_t window_a_vertices(eli_context *ctx)
{
    return eli_find_window_by_name(ctx, "A")->draw_list.vtx_count;
}

ELI_TEST(a_window_over_the_widget_hides_it_and_draws_it_static)
{
    eli_context *ctx = setup();
    eli_live_widget *w = eli_live_find(g_dot);
    ELI_ASSERT_TRUE(w->visible);
    uint32_t base = window_a_vertices(ctx);
    test_live_reset_counts();

    g_cover_pos = eli_make_vec2(40.0f, 60.0f);   /* B over the dot */
    test_ui_frame(ui);
    ELI_ASSERT_FALSE(w->visible);
    ELI_ASSERT_EQ(g_test_live.updates, 1);
    ELI_ASSERT_EQ(g_test_live.last_changed, (unsigned)ELI_LIVE_CHANGED_VISIBLE);
    ELI_ASSERT_FALSE(g_test_live.last_visible);
    ELI_ASSERT_TRUE(w->main_static);
    ELI_ASSERT_FLT_NEAR(eli_get_redraw_timeout(), 0.0f, 0.0f);   /* a frame to draw it static */

    test_ui_frame(ui);
    ELI_ASSERT_GT(window_a_vertices(ctx), base);   /* the static dot, under B */
    test_ui_frames(ui, 5);
    ELI_ASSERT_FLT_NEAR(eli_get_redraw_timeout(), -1.0f, 0.0f);   /* static: no slow redraw */
    ELI_ASSERT_EQ(g_test_live.updates, 1);

    test_live_reset_counts();
    g_cover_pos = eli_make_vec2(500.0f, 400.0f);   /* uncovered */
    test_ui_frame(ui);
    ELI_ASSERT_FALSE(w->visible);                  /* this frame still drew the static copy */
    ELI_ASSERT_FALSE(w->main_static);
    ELI_ASSERT_FLT_NEAR(eli_get_redraw_timeout(), 0.0f, 0.0f);
    test_ui_frame(ui);
    ELI_ASSERT_EQ(window_a_vertices(ctx), base);   /* static copy gone ... */
    ELI_ASSERT_TRUE(w->visible);                   /* ... and the layer back */
    ELI_ASSERT_EQ(g_test_live.updates, 1);
    ELI_ASSERT_TRUE(g_test_live.last_visible);
    eli_live_set_backend(NULL);
    test_ui_teardown(ctx);
}

ELI_TEST(a_window_elsewhere_does_not_hide_it)
{
    eli_context *ctx = setup();
    g_cover_pos = eli_make_vec2(200.0f, 60.0f);   /* overlaps window A, not the dot */
    test_ui_frames(ui, 3);
    ELI_ASSERT_TRUE(eli_live_find(g_dot)->visible);
    ELI_ASSERT_FALSE(eli_live_find(g_dot)->main_static);
    eli_live_set_backend(NULL);
    test_ui_teardown(ctx);
}

ELI_TEST(the_foreground_list_hides_it)
{
    eli_context *ctx = setup();
    g_fg_cover = true;
    test_ui_frames(ui, 2);
    ELI_ASSERT_FALSE(eli_live_find(g_dot)->visible);
    ELI_ASSERT_TRUE(eli_live_find(g_dot)->main_static);
    g_fg_cover = false;
    test_ui_frames(ui, 2);
    ELI_ASSERT_TRUE(eli_live_find(g_dot)->visible);
    eli_live_set_backend(NULL);
    test_ui_teardown(ctx);
}

ELI_TEST(clipping_cuts_without_hiding_and_clipped_out_hides_without_static)
{
    eli_context *ctx = setup();
    eli_live_widget *w = eli_live_find(g_dot);
    float right = w->clip.x + w->clip.w;   /* the window's content clip */
    g_dot_rect = eli_make_rect(right - 5.0f, 80.0f, 10.0f, 10.0f);
    test_ui_frames(ui, 2);
    ELI_ASSERT_TRUE(w->visible);   /* half in: the backend cuts it at `clip` */
    ELI_ASSERT_LT(w->clip.x + w->clip.w, w->rect.x + w->rect.w);

    test_live_reset_counts();
    g_dot_rect = eli_make_rect(right + 20.0f, 80.0f, 10.0f, 10.0f);
    test_ui_frame(ui);
    ELI_ASSERT_FALSE(w->visible);
    ELI_ASSERT_FALSE(w->main_static);   /* nothing to draw: no static copy, no redraw */
    ELI_ASSERT_EQ(g_test_live.changed_or,
                  (unsigned)(ELI_LIVE_CHANGED_GEOMETRY | ELI_LIVE_CHANGED_VISIBLE));
    eli_live_set_backend(NULL);
    test_ui_teardown(ctx);
}

ELI_TEST(a_setter_on_an_occluded_widget_redraws_the_static_copy)
{
    eli_context *ctx = setup();
    g_cover_pos = eli_make_vec2(40.0f, 60.0f);
    test_ui_frames(ui, 5);
    ELI_ASSERT_FLT_NEAR(eli_get_redraw_timeout(), -1.0f, 0.0f);
    test_live_reset_counts();
    ELI_ASSERT_TRUE(eli_live_set_color(g_dot, ELI_COL32_BLACK));
    ELI_ASSERT_EQ(g_test_live.updates, 1);                       /* the hidden layer too */
    ELI_ASSERT_FLT_NEAR(eli_get_redraw_timeout(), 0.0f, 0.0f);   /* the main surface shows it */
    eli_live_set_backend(NULL);
    test_ui_teardown(ctx);
}

ELI_TEST_MAIN()
