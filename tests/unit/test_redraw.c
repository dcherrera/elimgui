/**
 * @file test_redraw.c
 * @brief Idle / redraw API (D6, spec §7): settle frames after input, app requests,
 *        timed requests, and the library's own deadline sources (caret blink,
 *        tooltip delay, key repeat); docked tabs and static UI go idle (-1).
 *
 * @status In use.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/elimgui.h>

#define DT (1.0f / 60.0f)

static eli_font_atlas *g_atlas = NULL;
static char g_buf[64] = "abc";
static bool g_with_input = false;
static bool g_with_tooltip = false;
static bool g_with_dock = false;

static eli_context *setup(void)
{
    eli_context *ctx = eli_create_context();
    ctx->io.display_size = eli_make_vec2(800.0f, 600.0f);
    eli_style_colors_dark(&ctx->style);
    g_atlas = eli_font_atlas_create();
    eli_font *f = eli_font_atlas_add_font_default(g_atlas, NULL);
    eli_font_atlas_build(g_atlas);
    eli_push_font(f);
    g_with_input = g_with_tooltip = g_with_dock = false;
    return ctx;
}

static void teardown(eli_context *ctx)
{
    eli_destroy_context(ctx);
    eli_font_atlas_destroy(g_atlas);
    g_atlas = NULL;
}

/** One full frame of a small static UI; returns the resolved timeout. */
static float frame(float dt)
{
    eli_get_io()->delta_time = dt;
    eli_frame_begin();
    if (g_with_dock) {
        eli_id space = eli_dock_space_over_viewport(eli_get_main_viewport(), 0);
        eli_set_next_window_dock_id(space, ELI_COND_FIRST_USE_EVER);
        if (eli_begin("TabA", NULL, 0))
            eli_text("a");
        eli_end();
        eli_set_next_window_dock_id(space, ELI_COND_FIRST_USE_EVER);
        if (eli_begin("TabB", NULL, 0))
            eli_text("b");
        eli_end();
    }
    eli_set_next_window_pos(eli_make_vec2(10, 10), ELI_COND_ALWAYS, eli_make_vec2(0, 0));
    eli_set_next_window_size(eli_make_vec2(300, 200), ELI_COND_ALWAYS);
    if (eli_begin("Main", NULL, 0)) {
        eli_text("static text");
        if (g_with_input)
            eli_input_text("Field", g_buf, sizeof(g_buf), 0, NULL, NULL);
        eli_button("Hover me");
        if (g_with_tooltip && eli_is_item_hovered_for_tooltip(ELI_HOVERED_FOR_TOOLTIP)) {
            eli_begin_tooltip();
            eli_text("tip");
            eli_end_tooltip();
        }
    }
    eli_end();
    eli_frame_end();
    return eli_get_redraw_timeout();
}

/** Run frames until the timeout is not 0 (at most `max`); returns frames run. */
static int settle(int max)
{
    int n = 0;
    while (n < max && frame(DT) == 0.0f)
        n++;
    return n;
}

ELI_TEST(static_ui_goes_idle) {
    eli_context *ctx = setup();
    int n = settle(20);
    ELI_ASSERT_LT(n, 20);
    ELI_ASSERT_TRUE(eli_get_redraw_timeout() < 0.0f);
    ELI_ASSERT_TRUE(frame(DT) < 0.0f);           /* stays idle */
    teardown(ctx);
}

ELI_TEST(input_owes_two_settle_frames_then_idle) {
    eli_context *ctx = setup();
    settle(20);
    eli_io_add_mouse_pos_event(500.0f, 500.0f);  /* outside the window */
    ELI_ASSERT_TRUE(frame(DT) == 0.0f);          /* the input frame */
    ELI_ASSERT_TRUE(frame(DT) == 0.0f);          /* settle 1 */
    ELI_ASSERT_TRUE(frame(DT) < 0.0f);           /* settle 2 was the frame above */
    teardown(ctx);
}

ELI_TEST(app_request_between_frames_wakes_now) {
    eli_context *ctx = setup();
    settle(20);
    ELI_ASSERT_TRUE(eli_get_redraw_timeout() < 0.0f);
    eli_request_redraw();
    ELI_ASSERT_TRUE(eli_get_redraw_timeout() == 0.0f);
    ELI_ASSERT_EQ(settle(20), 2);
    teardown(ctx);
}

ELI_TEST(request_after_sets_earliest_deadline) {
    eli_context *ctx = setup();
    settle(20);
    eli_request_redraw_after(0.5f);              /* between frames: shortens now */
    ELI_ASSERT_FLT_NEAR(eli_get_redraw_timeout(), 0.5f, 1e-6);
    eli_request_redraw_after(0.2f);
    ELI_ASSERT_FLT_NEAR(eli_get_redraw_timeout(), 0.2f, 1e-6);
    ELI_ASSERT_TRUE(frame(0.2f) < 0.0f);          /* woke, nothing timed left: idle */
    teardown(ctx);
}

ELI_TEST(active_input_text_wakes_at_caret_blink_edges) {
    eli_context *ctx = setup();
    g_with_input = true;
    settle(20);
    eli_window *w = NULL;
    for (int i = 0; i < ctx->windows_count; i++)
        if (strcmp(ctx->windows[i]->name, "Main") == 0)
            w = ctx->windows[i];
    ELI_ASSERT_NOT_NULL(w);
    /* Click the field: it is the second item, one line below "static text". */
    eli_vec2 p = eli_make_vec2(w->pos.x + 60.0f, w->pos.y + 50.0f);
    eli_io_add_mouse_pos_event(p.x, p.y);
    eli_io_add_mouse_button_event(0, true);
    frame(DT);
    eli_io_add_mouse_button_event(0, false);
    settle(20);
    ELI_ASSERT_NE(ctx->active_id, 0u);
    float t = eli_get_redraw_timeout();
    ELI_ASSERT_GT(t, 0.0f);
    ELI_ASSERT_LE(t, 0.81f);
    int blinks = 0;                              /* ~1.2 s of blinking: 2 edges per cycle */
    float clock = 0.0f;
    while (clock < 2.4f && blinks < 10) {
        clock += t;
        t = frame(t);
        blinks++;
        if (t == 0.0f)
            t = frame(DT);
    }
    ELI_ASSERT_LE(blinks, 6);
    ELI_ASSERT_GE(blinks, 3);
    teardown(ctx);
}

ELI_TEST(tooltip_delay_requests_a_timed_frame) {
    eli_context *ctx = setup();
    g_with_tooltip = true;
    settle(20);
    eli_window *w = ctx->windows[0];
    eli_io_add_mouse_pos_event(w->pos.x + 40.0f, w->pos.y + 50.0f);
    frame(DT);
    settle(5);
    float t = eli_get_redraw_timeout();
    ELI_ASSERT_GT(t, 0.0f);                      /* waiting for the hover delay */
    ELI_ASSERT_LE(t, ctx->style.hover_delay_normal + ctx->style.hover_stationary_delay);
    teardown(ctx);
}

ELI_TEST(held_key_wakes_at_repeat_edges) {
    eli_context *ctx = setup();
    settle(20);
    eli_io_add_key_event(ELI_KEY_LEFT_ARROW, true);
    frame(DT);
    settle(5);
    float t = eli_get_redraw_timeout();
    ELI_ASSERT_GT(t, 0.0f);
    ELI_ASSERT_LE(t, ctx->io.key_repeat_delay);
    eli_io_add_key_event(ELI_KEY_LEFT_ARROW, false);
    settle(5);
    ELI_ASSERT_TRUE(eli_get_redraw_timeout() < 0.0f);
    teardown(ctx);
}

ELI_TEST(held_modifier_alone_stays_idle) {
    eli_context *ctx = setup();
    settle(20);
    eli_io_add_key_event(ELI_KEY_LEFT_SHIFT, true);
    frame(DT);
    settle(5);
    ELI_ASSERT_TRUE(eli_get_redraw_timeout() < 0.0f);
    teardown(ctx);
}

ELI_TEST(docked_tabs_go_idle) {
    eli_context *ctx = setup();
    g_with_dock = true;
    int n = settle(30);
    ELI_ASSERT_LT(n, 30);
    ELI_ASSERT_TRUE(eli_get_redraw_timeout() < 0.0f);
    teardown(ctx);
}

ELI_TEST_MAIN()
