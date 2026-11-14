/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file test_window_floating.c
 * @brief Unit tests for decorated floating windows: the drawn resize grip and
 *        resize cursors, resize from all four edges and corners (4 px band),
 *        clamping so the title bar stays reachable, raise on a body click,
 *        ELI_WINDOW_NO_BRING_TO_FRONT_ON_FOCUS, and the close button sized and
 *        hover-filled from the style.
 *
 * Geometry: no font is bound, so the title bar is 13 + 2 * 3 = 19 px tall. The
 * test window "F" opens at (100,100) sized 200x150 (ELI_COND_FIRST_USE_EVER, so
 * user moves and resizes stick).
 *
 * @status Covers the floating-window behaviours listed above.
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

/* Begin a floating window at (x,y) sized (w,h) on first use only. */
static void begin_at(const char *name, float x, float y, float w, float h, bool *p_open,
                     eli_window_flags flags)
{
    eli_set_next_window_pos(eli_make_vec2(x, y), ELI_COND_FIRST_USE_EVER, eli_make_vec2(0.0f, 0.0f));
    eli_set_next_window_size(eli_make_vec2(w, h), ELI_COND_FIRST_USE_EVER);
    eli_begin(name, p_open, flags);
}

/* One frame of the test window "F"; returns the cursor elimgui asked for. */
static eli_mouse_cursor frame_f(eli_window_flags flags)
{
    frame_begin();
    begin_at("F", 100.0f, 100.0f, 200.0f, 150.0f, NULL, flags);
    eli_mouse_cursor cursor = eli_get_mouse_cursor();
    eli_end();
    frame_end();
    return cursor;
}

/** Vertices in a window's draw list painted with a style colour slot. */
static int count_vtx_col(const eli_window *win, eli_col slot)
{
    eli_col32 col = eli_get_color_u32(slot, 1.0f);
    int n = 0;
    for (uint32_t i = 0; i < win->draw_list.vtx_count; i++)
        if (win->draw_list.vtx[i].col == col)
            n++;
    return n;
}

/**
 * Create "F", hover `from`, press there, drag to `to`, release. Returns the
 * window; *hover_cursor gets the cursor shown while hovering `from`.
 */
static eli_window *drag_f(eli_context *ctx, eli_vec2 from, eli_vec2 to, eli_mouse_cursor *hover_cursor)
{
    eli_io_add_mouse_pos_event(900.0f, 700.0f);
    frame_f(0);
    eli_io_add_mouse_pos_event(from.x, from.y);
    eli_mouse_cursor c = frame_f(0);
    if (hover_cursor != NULL)
        *hover_cursor = c;
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    frame_f(0);
    eli_io_add_mouse_pos_event(to.x, to.y);
    frame_f(0);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, false);
    frame_f(0);
    return eli_find_window_by_name(ctx, "F");
}

ELI_TEST(window_resize_grip_drawn_with_state_colours_and_cursor) {
    eli_context *ctx = win_setup();

    eli_io_add_mouse_pos_event(900.0f, 700.0f);
    frame_f(0);
    ELI_ASSERT_EQ(frame_f(0), ELI_MOUSE_CURSOR_ARROW);
    eli_window *win = eli_find_window_by_name(ctx, "F");
    ELI_ASSERT_GT(count_vtx_col(win, ELI_COL_RESIZE_GRIP), 0);
    ELI_ASSERT_EQ(count_vtx_col(win, ELI_COL_RESIZE_GRIP_HOVERED), 0);

    /* Hover the bottom-right grip (window bottom-right is (300,250)). */
    eli_io_add_mouse_pos_event(295.0f, 245.0f);
    ELI_ASSERT_EQ(frame_f(0), ELI_MOUSE_CURSOR_RESIZE_NWSE);
    ELI_ASSERT_GT(count_vtx_col(win, ELI_COL_RESIZE_GRIP_HOVERED), 0);

    /* Hold it: active colour, and dragging still resizes from the corner. */
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    frame_f(0);
    eli_io_add_mouse_pos_event(315.0f, 255.0f);
    ELI_ASSERT_EQ(frame_f(0), ELI_MOUSE_CURSOR_RESIZE_NWSE);
    ELI_ASSERT_GT(count_vtx_col(win, ELI_COL_RESIZE_GRIP_ACTIVE), 0);
    ELI_ASSERT_FLT_NEAR(win->size_full.x, 220.0f, 0.01f);
    ELI_ASSERT_FLT_NEAR(win->size_full.y, 160.0f, 0.01f);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, false);
    frame_f(0);
    eli_destroy_context(ctx);

    /* A NO_RESIZE window draws no grip and never shows a resize cursor. */
    ctx = win_setup();
    eli_io_add_mouse_pos_event(295.0f, 245.0f);
    frame_f(ELI_WINDOW_NO_RESIZE);
    ELI_ASSERT_EQ(frame_f(ELI_WINDOW_NO_RESIZE), ELI_MOUSE_CURSOR_ARROW);
    win = eli_find_window_by_name(ctx, "F");
    ELI_ASSERT_EQ(count_vtx_col(win, ELI_COL_RESIZE_GRIP), 0);
    ELI_ASSERT_EQ(count_vtx_col(win, ELI_COL_RESIZE_GRIP_HOVERED), 0);
    eli_destroy_context(ctx);
}

ELI_TEST(window_resize_from_edges_and_corners) {
    eli_context *ctx = win_setup();
    eli_mouse_cursor cur = ELI_MOUSE_CURSOR_NONE;
    eli_window *win = drag_f(ctx, eli_make_vec2(101.0f, 175.0f), eli_make_vec2(81.0f, 175.0f), &cur);
    ELI_ASSERT_EQ(cur, ELI_MOUSE_CURSOR_RESIZE_EW);
    ELI_ASSERT_FLT_NEAR(win->pos.x, 80.0f, 0.01f);          /* left edge followed */
    ELI_ASSERT_FLT_NEAR(win->size_full.x, 220.0f, 0.01f);   /* right edge stayed at 300 */
    ELI_ASSERT_FLT_NEAR(win->pos.y, 100.0f, 0.01f);
    ELI_ASSERT_FLT_NEAR(win->size_full.y, 150.0f, 0.01f);
    eli_destroy_context(ctx);

    /* Top edge: inside the title bar's top 4 px it resizes, it does not move. */
    ctx = win_setup();
    win = drag_f(ctx, eli_make_vec2(200.0f, 101.0f), eli_make_vec2(200.0f, 81.0f), &cur);
    ELI_ASSERT_EQ(cur, ELI_MOUSE_CURSOR_RESIZE_NS);
    ELI_ASSERT_FLT_NEAR(win->pos.y, 80.0f, 0.01f);
    ELI_ASSERT_FLT_NEAR(win->size_full.y, 170.0f, 0.01f);
    ELI_ASSERT_FLT_NEAR(win->pos.x, 100.0f, 0.01f);
    ELI_ASSERT_NULL(ctx->moving_window);
    eli_destroy_context(ctx);

    ctx = win_setup();
    win = drag_f(ctx, eli_make_vec2(299.0f, 175.0f), eli_make_vec2(319.0f, 175.0f), &cur);
    ELI_ASSERT_EQ(cur, ELI_MOUSE_CURSOR_RESIZE_EW);
    ELI_ASSERT_FLT_NEAR(win->size_full.x, 220.0f, 0.01f);
    ELI_ASSERT_FLT_NEAR(win->pos.x, 100.0f, 0.01f);
    eli_destroy_context(ctx);

    ctx = win_setup();
    win = drag_f(ctx, eli_make_vec2(200.0f, 249.0f), eli_make_vec2(200.0f, 269.0f), &cur);
    ELI_ASSERT_EQ(cur, ELI_MOUSE_CURSOR_RESIZE_NS);
    ELI_ASSERT_FLT_NEAR(win->size_full.y, 170.0f, 0.01f);
    eli_destroy_context(ctx);

    ctx = win_setup();
    win = drag_f(ctx, eli_make_vec2(101.0f, 101.0f), eli_make_vec2(91.0f, 91.0f), &cur);
    ELI_ASSERT_EQ(cur, ELI_MOUSE_CURSOR_RESIZE_NWSE);
    ELI_ASSERT_FLT_NEAR(win->pos.x, 90.0f, 0.01f);
    ELI_ASSERT_FLT_NEAR(win->pos.y, 90.0f, 0.01f);
    ELI_ASSERT_FLT_NEAR(win->size_full.x, 210.0f, 0.01f);
    ELI_ASSERT_FLT_NEAR(win->size_full.y, 160.0f, 0.01f);
    eli_destroy_context(ctx);

    ctx = win_setup();
    win = drag_f(ctx, eli_make_vec2(299.0f, 101.0f), eli_make_vec2(309.0f, 91.0f), &cur);
    ELI_ASSERT_EQ(cur, ELI_MOUSE_CURSOR_RESIZE_NESW);
    ELI_ASSERT_FLT_NEAR(win->pos.y, 90.0f, 0.01f);
    ELI_ASSERT_FLT_NEAR(win->size_full.x, 210.0f, 0.01f);
    ELI_ASSERT_FLT_NEAR(win->size_full.y, 160.0f, 0.01f);
    eli_destroy_context(ctx);

    ctx = win_setup();
    win = drag_f(ctx, eli_make_vec2(101.0f, 249.0f), eli_make_vec2(91.0f, 259.0f), &cur);
    ELI_ASSERT_EQ(cur, ELI_MOUSE_CURSOR_RESIZE_NESW);
    ELI_ASSERT_FLT_NEAR(win->pos.x, 90.0f, 0.01f);
    ELI_ASSERT_FLT_NEAR(win->size_full.x, 210.0f, 0.01f);
    ELI_ASSERT_FLT_NEAR(win->size_full.y, 160.0f, 0.01f);
    eli_destroy_context(ctx);

    /* 5 px in from the left edge is outside the 4 px band: nothing changes. */
    ctx = win_setup();
    win = drag_f(ctx, eli_make_vec2(105.0f, 175.0f), eli_make_vec2(85.0f, 175.0f), &cur);
    ELI_ASSERT_EQ(cur, ELI_MOUSE_CURSOR_ARROW);
    ELI_ASSERT_FLT_NEAR(win->pos.x, 100.0f, 0.01f);
    ELI_ASSERT_FLT_NEAR(win->size_full.x, 200.0f, 0.01f);
    eli_destroy_context(ctx);
}

ELI_TEST(window_resize_left_edge_respects_min_size) {
    eli_context *ctx = win_setup();
    const float xs[] = { 101.0f, 101.0f, 251.0f, 251.0f };
    eli_io_add_mouse_pos_event(900.0f, 700.0f);
    for (int f = -1; f < 4; f++) {
        if (f >= 0)
            eli_io_add_mouse_pos_event(xs[f], 175.0f);
        if (f == 1)
            eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
        frame_begin();
        eli_set_next_window_size_constraints(eli_make_vec2(150.0f, 100.0f), eli_make_vec2(-1.0f, -1.0f));
        begin_at("F", 100.0f, 100.0f, 200.0f, 150.0f, NULL, 0);
        eli_end();
        frame_end();
    }
    eli_window *win = eli_find_window_by_name(ctx, "F");
    ELI_ASSERT_FLT_NEAR(win->size_full.x, 150.0f, 0.01f);   /* stopped at the minimum */
    ELI_ASSERT_FLT_NEAR(win->pos.x, 150.0f, 0.01f);         /* right edge still at 300 */
    eli_destroy_context(ctx);
}

/* Open "C" at (x,y) on first use with `cond`, run three frames, return it. */
static eli_window *settle_window_at(eli_context *ctx, float x, float y, eli_cond cond)
{
    for (int f = 0; f < 3; f++) {
        frame_begin();
        eli_set_next_window_pos(eli_make_vec2(x, y), cond, eli_make_vec2(0.0f, 0.0f));
        eli_set_next_window_size(eli_make_vec2(200.0f, 150.0f), ELI_COND_FIRST_USE_EVER);
        eli_begin("C", NULL, 0);
        eli_end();
        frame_end();
    }
    return eli_find_window_by_name(ctx, "C");
}

ELI_TEST(window_clamped_so_title_bar_stays_on_screen) {
    eli_context *ctx = win_setup();
    ELI_ASSERT_FLT_NEAR(ctx->style.display_window_padding.x, 64.0f, 0.01f);

    /* Off the top-left: keeps 64 px of width visible, the top edge on screen. */
    eli_window *win = settle_window_at(ctx, -500.0f, -100.0f, ELI_COND_FIRST_USE_EVER);
    ELI_ASSERT_FLT_NEAR(win->pos.x, 64.0f - 200.0f, 0.01f);
    ELI_ASSERT_FLT_NEAR(win->pos.y, 0.0f, 0.01f);
    eli_destroy_context(ctx);

    /* Off the bottom-right: 64 px of the window stays on screen. */
    ctx = win_setup();
    win = settle_window_at(ctx, 2000.0f, 2000.0f, ELI_COND_FIRST_USE_EVER);
    ELI_ASSERT_FLT_NEAR(win->pos.x, 1024.0f - 64.0f, 0.01f);
    ELI_ASSERT_FLT_NEAR(win->pos.y, 768.0f - 64.0f, 0.01f);
    eli_destroy_context(ctx);

    /* A position the app sets every frame is never clamped (off-screen tear-offs). */
    ctx = win_setup();
    win = settle_window_at(ctx, -500.0f, -100.0f, ELI_COND_ALWAYS);
    ELI_ASSERT_FLT_NEAR(win->pos.x, -500.0f, 0.01f);
    ELI_ASSERT_FLT_NEAR(win->pos.y, -100.0f, 0.01f);
    eli_destroy_context(ctx);

    /* Dragging the title bar far off the left stops at the clamp. */
    ctx = win_setup();
    win = drag_f(ctx, eli_make_vec2(150.0f, 110.0f), eli_make_vec2(-400.0f, 110.0f), NULL);
    ELI_ASSERT_FLT_NEAR(win->pos.x, 64.0f - 200.0f, 0.01f);
    ELI_ASSERT_FLT_NEAR(win->pos.y, 100.0f, 0.01f);
    eli_destroy_context(ctx);
}

/* Two overlapping windows: A at (0,0,200,200), B at (100,100,200,200). */
static void frame_ab(eli_window_flags b_flags, bool focus_b, bool *a_hovered, bool *b_focused)
{
    frame_begin();
    begin_at("A", 0.0f, 0.0f, 200.0f, 200.0f, NULL, 0);
    *a_hovered = eli_is_window_hovered();
    eli_end();
    if (focus_b)
        eli_set_next_window_focus();
    begin_at("B", 100.0f, 100.0f, 200.0f, 200.0f, NULL, b_flags);
    *b_focused = eli_is_window_focused();
    eli_end();
    frame_end();
}

ELI_TEST(window_body_click_raises) {
    eli_context *ctx = win_setup();
    bool a_hov = false, b_foc = false;

    eli_io_add_mouse_pos_event(150.0f, 150.0f);
    frame_ab(0, false, &a_hov, &b_foc);
    frame_ab(0, false, &a_hov, &b_foc);
    ELI_ASSERT_FALSE(a_hov);                     /* B (created last) is on top */

    /* Click A's body away from the overlap and from its title bar. */
    eli_io_add_mouse_pos_event(50.0f, 150.0f);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    frame_ab(0, false, &a_hov, &b_foc);
    ELI_ASSERT_FALSE(b_foc);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, false);
    eli_io_add_mouse_pos_event(150.0f, 150.0f);
    frame_ab(0, false, &a_hov, &b_foc);
    ELI_ASSERT_TRUE(a_hov);                      /* A is now on top in the overlap */
    eli_destroy_context(ctx);
}

ELI_TEST(window_no_bring_to_front_on_focus) {
    eli_context *ctx = win_setup();
    bool a_hov = false, b_foc = false;
    const eli_window_flags nb = ELI_WINDOW_NO_BRING_TO_FRONT_ON_FOCUS;

    /* Created after A, but a no-bring-to-front window joins at the back. */
    eli_io_add_mouse_pos_event(150.0f, 150.0f);
    frame_ab(nb, false, &a_hov, &b_foc);
    frame_ab(nb, false, &a_hov, &b_foc);
    ELI_ASSERT_TRUE(a_hov);

    /* A body click focuses B but leaves it behind A. */
    eli_io_add_mouse_pos_event(250.0f, 250.0f);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    frame_ab(nb, false, &a_hov, &b_foc);
    ELI_ASSERT_TRUE(b_foc);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, false);
    eli_io_add_mouse_pos_event(150.0f, 150.0f);
    frame_ab(nb, false, &a_hov, &b_foc);
    ELI_ASSERT_TRUE(a_hov);

    /* Programmatic focus does not raise it either. */
    frame_ab(nb, true, &a_hov, &b_foc);
    ELI_ASSERT_TRUE(b_foc);
    frame_ab(nb, true, &a_hov, &b_foc);
    ELI_ASSERT_TRUE(a_hov);
    eli_destroy_context(ctx);
}

/* One frame of the closable window "##close" (no visible title text). */
static void frame_close(bool *open)
{
    frame_begin();
    begin_at("##close", 100.0f, 100.0f, 200.0f, 150.0f, open, 0);
    eli_end();
    frame_end();
}

ELI_TEST(window_close_button_sized_and_filled_from_style) {
    eli_context *ctx = win_setup();
    ELI_ASSERT_FLT_NEAR(ctx->style.close_button_size, 14.0f, 0.01f);
    ctx->style.close_button_size = 24.0f;
    bool open = true;

    eli_io_add_mouse_pos_event(900.0f, 700.0f);
    frame_close(&open);
    frame_close(&open);
    eli_window *win = eli_find_window_by_name(ctx, "##close");
    ELI_ASSERT_FLT_NEAR(win->title_bar_height, 24.0f, 0.01f);   /* grows to fit the button */
    ELI_ASSERT_EQ(count_vtx_col(win, ELI_COL_BUTTON_HOVERED), 0);

    /* Box: x 272..296 (frame_padding.x in from the right), y 100..124. Hovered,
     * it gets the hovered fill and the cross keeps the text colour. */
    eli_io_add_mouse_pos_event(284.0f, 112.0f);
    frame_close(&open);
    ELI_ASSERT_TRUE(open);
    ELI_ASSERT_GT(count_vtx_col(win, ELI_COL_BUTTON_HOVERED), 0);
    ELI_ASSERT_GT(count_vtx_col(win, ELI_COL_TEXT), 0);

    /* A click inside the 24 px box but outside a 14 px one closes the window. */
    eli_io_add_mouse_pos_event(275.0f, 121.0f);
    frame_close(&open);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    frame_close(&open);
    ELI_ASSERT_FALSE(open);
    ELI_ASSERT_NULL(ctx->moving_window);                         /* the click did not grab the title */
    eli_destroy_context(ctx);
}

ELI_TEST_MAIN()
