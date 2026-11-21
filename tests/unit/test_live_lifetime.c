/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file test_live_lifetime.c
 * @brief Live widget lifetime: created once, removed at the end of a built frame
 *        that does not submit them (hidden, collapsed window), steady frames send
 *        nothing, a move sends geometry only, per-window ids, pool overflow,
 *        a refused create, kind change, context destroy, and text rasterization.
 *
 * @status Live widgets coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"
#include "eli_test_live.h"

static const eli_live_pulse k_pulse = { 1.4f, 0.3f };
static bool g_show = true;
static bool g_collapse = false;
static eli_vec2 g_pos = { 20.0f, 20.0f };
static int g_count = 2;
static eli_id g_dot = 0, g_text = 0;

static void ui(void)
{
    eli_set_next_window_collapsed(g_collapse, ELI_COND_ALWAYS);
    if (test_live_begin("A", g_pos, eli_make_vec2(700, 500))) {
        for (int i = 0; i < g_count && g_show; i++) {
            eli_push_id_int(i);
            eli_id id = eli_live_dot("dot", 4.0f, ELI_COL32_WHITE, &k_pulse);
            eli_same_line(0.0f, -1.0f);
            eli_pop_id();
            if (i == 0)
                g_dot = id;
        }
        if (g_show)
            g_text = eli_live_text("text", "Hi", 0.0f);
    }
    eli_end();
}

static eli_context *setup(void)
{
    eli_context *ctx = test_ui_setup();
    test_live_install();
    g_show = true;
    g_collapse = false;
    g_pos = eli_make_vec2(20.0f, 20.0f);
    g_count = 2;
    return ctx;
}

ELI_TEST(created_once_and_steady_frames_send_nothing)
{
    eli_context *ctx = setup();
    test_ui_frame(ui);
    ELI_ASSERT_EQ(g_test_live.creates, 3);
    ELI_ASSERT_EQ(g_test_live.updates, 3);   /* each one shown at the first frame end */
    ELI_ASSERT_TRUE(g_test_live.last_visible);
    ELI_ASSERT_EQ(g_test_live.handles, 3);
    test_live_reset_counts();
    test_ui_frames(ui, 50);
    ELI_ASSERT_EQ(g_test_live.creates, 0);
    ELI_ASSERT_EQ(g_test_live.updates, 0);
    ELI_ASSERT_EQ(g_test_live.destroys, 0);
    test_ui_teardown(ctx);
    ELI_ASSERT_EQ(g_test_live.handles, 0);   /* context destroy removes them */
    eli_live_set_backend(NULL);
}

ELI_TEST(removed_when_not_submitted)
{
    eli_context *ctx = setup();
    test_ui_frames(ui, 3);
    test_live_reset_counts();
    g_show = false;
    test_ui_frame(ui);
    ELI_ASSERT_EQ(g_test_live.destroys, 3);
    ELI_ASSERT_EQ(eli_live_get_stats()->count, 0);
    ELI_ASSERT_NULL(eli_live_find(g_dot));
    ELI_ASSERT_FALSE(eli_live_set_text(g_text, "x"));   /* gone */

    g_show = true;   /* back: created again */
    test_ui_frame(ui);
    ELI_ASSERT_EQ(g_test_live.creates, 3);
    g_collapse = true;   /* collapsed window: contents skipped */
    test_ui_frames(ui, 2);
    ELI_ASSERT_EQ(eli_live_get_stats()->count, 0);
    ELI_ASSERT_EQ(g_test_live.handles, 0);
    eli_live_set_backend(NULL);
    test_ui_teardown(ctx);
}

ELI_TEST(no_removal_without_a_built_frame)
{
    eli_context *ctx = setup();
    test_ui_frames(ui, 3);
    test_live_reset_counts();
    ELI_ASSERT_TRUE(eli_live_set_text(g_text, "Ho"));   /* between frames, no frame built */
    ELI_ASSERT_EQ(g_test_live.destroys, 0);
    ELI_ASSERT_EQ(eli_live_get_stats()->count, 3);
    eli_live_set_backend(NULL);
    test_ui_teardown(ctx);
}

ELI_TEST(a_move_sends_geometry_only)
{
    eli_context *ctx = setup();
    test_ui_frames(ui, 3);
    eli_rect before = eli_live_find(g_dot)->rect;
    test_live_reset_counts();
    g_pos = eli_make_vec2(60.0f, 30.0f);
    test_ui_frame(ui);
    ELI_ASSERT_EQ(g_test_live.updates, 3);
    ELI_ASSERT_EQ(g_test_live.changed_or, (unsigned)ELI_LIVE_CHANGED_GEOMETRY);
    ELI_ASSERT_FLT_NEAR(eli_live_find(g_dot)->rect.x - before.x, 40.0f, 1e-4f);
    ELI_ASSERT_FLT_NEAR(eli_live_find(g_dot)->rect.y - before.y, 10.0f, 1e-4f);
    test_live_reset_counts();
    test_ui_frames(ui, 5);
    ELI_ASSERT_EQ(g_test_live.updates, 0);
    eli_live_set_backend(NULL);
    test_ui_teardown(ctx);
}

static void ui_two_windows(void)
{
    test_live_begin("One", eli_make_vec2(10, 10), eli_make_vec2(100, 100));
    eli_live_spinner("busy", 6.0f, ELI_COL32_WHITE);
    eli_end();
    test_live_begin("Two", eli_make_vec2(200, 10), eli_make_vec2(100, 100));
    eli_live_spinner("busy", 6.0f, ELI_COL32_WHITE);
    eli_end();
}

ELI_TEST(same_label_in_two_windows_is_two_widgets)
{
    eli_context *ctx = setup();
    test_ui_frames(ui_two_windows, 2);
    ELI_ASSERT_EQ(eli_live_get_stats()->count, 2);
    ELI_ASSERT_EQ(g_test_live.creates, 2);
    eli_live_set_backend(NULL);
    test_ui_teardown(ctx);
}

ELI_TEST(pool_overflow_falls_back)
{
    eli_context *ctx = setup();
    g_count = ELI_LIVE_MAX + 3;   /* + the text */
    test_ui_frames(ui, 2);
    const eli_live_stats *st = eli_live_get_stats();
    ELI_ASSERT_EQ(st->count, ELI_LIVE_MAX);
    ELI_ASSERT_EQ(st->live, ELI_LIVE_MAX);
    ELI_ASSERT_EQ(st->overflow, 8u);   /* 4 extra submits x 2 frames */
    /* The extra pulsing dots are drawn and animated by the main surface. */
    ELI_ASSERT_GT(eli_find_window_by_name(ctx, "A")->draw_list.vtx_count, 0u);
    test_ui_frames(ui, 3);
    ELI_ASSERT_FLT_NEAR(eli_get_redraw_timeout(), ELI_LIVE_FALLBACK_INTERVAL, 1e-6f);
    eli_live_set_backend(NULL);
    test_ui_teardown(ctx);
}

ELI_TEST(refused_create_falls_back_once)
{
    eli_context *ctx = setup();
    g_test_live.refuse = true;
    test_ui_frames(ui, 10);
    ELI_ASSERT_EQ(g_test_live.creates, 3);   /* not retried every frame */
    ELI_ASSERT_EQ(eli_live_get_stats()->live, 0);
    ELI_ASSERT_EQ(g_test_live.updates, 0);
    ELI_ASSERT_FLT_NEAR(eli_get_redraw_timeout(), ELI_LIVE_FALLBACK_INTERVAL, 1e-6f);
    eli_live_set_backend(NULL);
    test_ui_teardown(ctx);
}

static bool g_as_spinner = false;

static void ui_kind(void)
{
    test_live_begin("K", eli_make_vec2(10, 10), eli_make_vec2(100, 100));
    eli_id id = eli_get_id("w");
    eli_rect r = eli_make_rect(20, 40, 12, 12);
    if (g_as_spinner)
        eli_live_spinner_at(id, r, ELI_COL32_WHITE);
    else
        eli_live_dot_at(id, r, ELI_COL32_WHITE, NULL);
    eli_end();
}

ELI_TEST(kind_change_replaces_and_setters_check_kind)
{
    eli_context *ctx = setup();
    test_ui_frames(ui_kind, 2);
    eli_id id = g_eli_live[0].id;
    ELI_ASSERT_EQ(eli_live_find(id)->kind, ELI_LIVE_DOT);
    ELI_ASSERT_FALSE(eli_live_set_fraction(id, 0.5f));   /* not a progress bar */
    ELI_ASSERT_FALSE(eli_live_set_text(id, "x"));
    ELI_ASSERT_TRUE(eli_live_set_color(id, ELI_COL32_BLACK));
    ELI_ASSERT_FALSE(eli_live_set_color(id + 1u, ELI_COL32_BLACK));
    g_as_spinner = true;
    test_ui_frame(ui_kind);
    ELI_ASSERT_EQ(eli_live_find(id)->kind, ELI_LIVE_SPINNER);
    ELI_ASSERT_EQ(g_test_live.destroys, 1);
    ELI_ASSERT_EQ(g_test_live.creates, 2);
    g_as_spinner = false;
    eli_live_set_backend(NULL);
    test_ui_teardown(ctx);
}

ELI_TEST(text_rasterizes_from_the_atlas_or_falls_back)
{
    eli_context *ctx = setup();
    test_ui_frame(ui);
    ELI_ASSERT_TRUE(eli_live_find(g_text)->live);
    ELI_ASSERT_TRUE(g_test_live.raster_ok);
    int covered = 0;
    for (size_t i = 0; i < sizeof(g_test_live.raster); i++)
        covered += g_test_live.raster[i] > 0;
    ELI_ASSERT_GT(covered, 10);   /* "Hi" left some coverage */
    eli_live_widget *w = eli_live_find(g_text);
    ELI_ASSERT_EQ(eli_live__copy_text(w->text, "\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9"
                                               "\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9"
                                               "\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9"
                                               "\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9"
                                               "\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9"
                                               "\xc3\xa9\xc3\xa9"), true);
    ELI_ASSERT_EQ(strlen(w->text), (size_t)62);   /* cut at a UTF-8 boundary under 63 */

    /* A baked atlas without CPU pixels cannot raster: live text falls back. */
    g_show = false;
    test_ui_frame(ui);
    eli_font_atlas_release_cpu_pixels(g_test_ui_atlas);
    ELI_ASSERT_FALSE(eli_live_text_can_raster(eli_get_font()));
    g_show = true;
    test_ui_frame(ui);
    ELI_ASSERT_FALSE(eli_live_find(g_text)->live);
    ELI_ASSERT_TRUE(eli_live_find(g_dot)->live);
    eli_live_set_backend(NULL);
    test_ui_teardown(ctx);
}

ELI_TEST_MAIN()
