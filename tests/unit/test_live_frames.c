/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file test_live_frames.c
 * @brief Live widgets ask for no main-surface frame while they animate (a backend
 *        layer draws them), and draw nothing on the main surface; the fallback
 *        draws them and asks for the slow redraw; setters between frames update
 *        the layer without a frame.
 *
 * @status Live widgets coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"
#include "eli_test_live.h"

static const eli_live_pulse k_pulse = { 1.4f, 0.3f };
static eli_id g_bar = 0, g_label = 0;
static bool g_dummy_only = false;

/** A window with a pulsing dot, a spinner, a progress bar and a text. */
static void ui_all(void)
{
    test_live_begin("Live", eli_make_vec2(20, 20), eli_make_vec2(300, 200));
    if (g_dummy_only) {   /* the same layout without live widgets */
        eli_dummy(eli_make_vec2(8, 8));
        eli_dummy(eli_make_vec2(16, 16));
        eli_dummy(eli_make_vec2(200, eli_get_frame_height()));
        eli_dummy(eli_make_vec2(80, eli_get_font_size()));
    } else {
        eli_live_dot("dot", 4.0f, ELI_COL32(80, 200, 120, 255), &k_pulse);
        eli_live_spinner("spin", 8.0f, ELI_COL32_WHITE);
        g_bar = eli_live_progress("bar", eli_make_vec2(200, 0), 0.25f);
        g_label = eli_live_text("clock", "12:00:01", 80.0f);
    }
    eli_end();
}

ELI_TEST(backend_layers_animate_with_zero_main_surface_frames)
{
    eli_context *ctx = test_ui_setup();
    test_live_install();
    test_ui_frames(ui_all, 5);   /* settle (first frames owe settle frames) */
    ELI_ASSERT_EQ(eli_live_get_stats()->live, 4);
    ELI_ASSERT_FLT_NEAR(eli_get_redraw_timeout(), -1.0f, 0.0f);
    int frames = test_live_loop_frames(ui_all, 30.0);
    printf("  live layers: %d main-surface frames in 30 s\n", frames);
    ELI_ASSERT_EQ(frames, 0);
    eli_live_set_backend(NULL);
    test_ui_teardown(ctx);
}

ELI_TEST(backend_layers_draw_nothing_on_the_main_surface)
{
    eli_context *ctx = test_ui_setup();
    test_live_install();
    g_dummy_only = true;
    test_ui_frames(ui_all, 3);
    eli_window *win = eli_find_window_by_name(ctx, "Live");
    uint32_t baseline = win->draw_list.vtx_count;
    g_dummy_only = false;
    test_ui_frames(ui_all, 3);
    ELI_ASSERT_EQ(win->draw_list.vtx_count, baseline);

    eli_live_set_backend(NULL);   /* fallback: the window draws them */
    test_ui_frames(ui_all, 1);
    ELI_ASSERT_GT(win->draw_list.vtx_count, baseline);
    test_ui_teardown(ctx);
}

ELI_TEST(fallback_draws_with_the_slow_redraw)
{
    eli_context *ctx = test_ui_setup();
    eli_live_set_backend(NULL);
    test_ui_frames(ui_all, 5);
    ELI_ASSERT_EQ(eli_live_get_stats()->live, 0);
    ELI_ASSERT_FLT_NEAR(eli_get_redraw_timeout(), ELI_LIVE_FALLBACK_INTERVAL, 1e-6f);
    int frames = test_live_loop_frames(ui_all, 30.0);
    printf("  fallback: %d main-surface frames in 30 s\n", frames);
    ELI_ASSERT_GE(frames, 118);   /* 30 s / 0.25 s */
    ELI_ASSERT_LE(frames, 121);
    test_ui_teardown(ctx);
}

ELI_TEST(fallback_interval_zero_draws_static)
{
    eli_context *ctx = test_ui_setup();
    eli_live_set_backend(NULL);
    eli_live_set_fallback_interval(0.0f);
    test_ui_frames(ui_all, 5);
    ELI_ASSERT_FLT_NEAR(eli_get_redraw_timeout(), -1.0f, 0.0f);
    ELI_ASSERT_GT(eli_find_window_by_name(ctx, "Live")->draw_list.vtx_count, 0u);
    eli_live_set_fallback_interval(ELI_LIVE_FALLBACK_INTERVAL);
    test_ui_teardown(ctx);
}

ELI_TEST(setters_between_frames_update_the_layer_without_a_frame)
{
    eli_context *ctx = test_ui_setup();
    test_live_install();
    test_ui_frames(ui_all, 5);
    test_live_reset_counts();
    ELI_ASSERT_TRUE(eli_live_set_fraction(g_bar, 0.75f));
    ELI_ASSERT_EQ(g_test_live.updates, 1);
    ELI_ASSERT_EQ(g_test_live.last_changed, (unsigned)ELI_LIVE_CHANGED_VALUE);
    ELI_ASSERT_FLT_NEAR(eli_live_find(g_bar)->fraction, 0.75f, 0.0f);
    ELI_ASSERT_TRUE(eli_live_set_text(g_label, "12:00:02"));
    ELI_ASSERT_EQ(g_test_live.last_changed, (unsigned)ELI_LIVE_CHANGED_TEXT);
    ELI_ASSERT_STR_EQ(eli_live_find(g_label)->text, "12:00:02");
    ELI_ASSERT_TRUE(eli_live_set_text(g_label, "12:00:02"));   /* unchanged: no call */
    ELI_ASSERT_EQ(g_test_live.updates, 2);
    ELI_ASSERT_FLT_NEAR(eli_get_redraw_timeout(), -1.0f, 0.0f);

    eli_live_set_backend(NULL);   /* fallback: a setter asks for a frame */
    test_ui_frames(ui_all, 5);
    eli_live_set_fallback_interval(0.0f);
    test_ui_frames(ui_all, 5);
    ELI_ASSERT_FLT_NEAR(eli_get_redraw_timeout(), -1.0f, 0.0f);
    ELI_ASSERT_TRUE(eli_live_set_fraction(g_bar, 0.5f));
    ELI_ASSERT_FLT_NEAR(eli_get_redraw_timeout(), 0.0f, 0.0f);
    eli_live_set_fallback_interval(ELI_LIVE_FALLBACK_INTERVAL);
    test_ui_teardown(ctx);
}

ELI_TEST(pulse_and_spinner_curves)
{
    eli_live_pulse p = { 1.4f, 0.3f };
    ELI_ASSERT_FLT_NEAR(eli_live_pulse_alpha(p, 0.0), 1.0f, 1e-5f);
    ELI_ASSERT_FLT_NEAR(eli_live_pulse_alpha(p, 0.7), 0.3f, 1e-5f);
    ELI_ASSERT_FLT_NEAR(eli_live_pulse_alpha(p, 1.4), 1.0f, 1e-4f);
    ELI_ASSERT_FLT_NEAR(eli_live_pulse_alpha((eli_live_pulse){ 0.0f, 0.3f }, 0.7), 1.0f, 0.0f);
    ELI_ASSERT_FLT_NEAR(eli_live_spinner_angle(0.25), 0.5f * ELI_PI, 1e-5f);
}

ELI_TEST_MAIN()
