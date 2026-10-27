/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file test_capture_flags.c
 * @brief io.want_capture_mouse / want_capture_keyboard / want_text_input (D15,
 *        spec §9.3), including the app-owned drag and next-frame overrides.
 *
 * @status In use.
 * @issues None
 * @todo None
 */
#include "eli_test.h"
#include "eli_test_ui.h"

static char g_buf[64] = "abc";
static eli_vec2 g_field_center;

/** One window (0,0)-(300,200) with an input_text. */
static void ui_window(void)
{
    eli_set_next_window_pos(eli_make_vec2(0, 0), ELI_COND_ALWAYS, eli_make_vec2(0, 0));
    eli_set_next_window_size(eli_make_vec2(300, 200), ELI_COND_ALWAYS);
    if (eli_begin("Win", NULL, 0)) {
        eli_input_text("Field", g_buf, sizeof(g_buf), 0, NULL, NULL);
        eli_vec2 mn = eli_get_item_rect_min(), mx = eli_get_item_rect_max();
        g_field_center = eli_make_vec2((mn.x + mx.x) * 0.5f, (mn.y + mx.y) * 0.5f);
    }
    eli_end();
}

ELI_TEST(mouse_capture_follows_hover) {
    eli_context *ctx = test_ui_setup();
    eli_io_add_mouse_pos_event(100, 150);
    test_ui_frames(ui_window, 2);
    ELI_ASSERT_TRUE(ctx->io.want_capture_mouse);
    eli_io_add_mouse_pos_event(600, 500);
    test_ui_frame(ui_window);
    ELI_ASSERT_FALSE(ctx->io.want_capture_mouse);
    test_ui_teardown(ctx);
}

ELI_TEST(drag_started_in_app_stays_with_app) {
    eli_context *ctx = test_ui_setup();
    test_ui_frames(ui_window, 2);
    eli_io_add_mouse_pos_event(600, 500);
    eli_io_add_mouse_button_event(0, true);
    test_ui_frame(ui_window);
    eli_io_add_mouse_pos_event(100, 150);                        /* drag over the window */
    test_ui_frame(ui_window);
    ELI_ASSERT_FALSE(ctx->io.want_capture_mouse);
    eli_io_add_mouse_button_event(0, false);
    test_ui_frame(ui_window);
    ELI_ASSERT_TRUE(ctx->io.want_capture_mouse);                 /* released: hover counts */
    test_ui_teardown(ctx);
}

ELI_TEST(active_field_captures_keyboard_and_text) {
    eli_context *ctx = test_ui_setup();
    test_ui_frames(ui_window, 2);
    ELI_ASSERT_FALSE(ctx->io.want_capture_keyboard);
    ELI_ASSERT_FALSE(ctx->io.want_text_input);
    test_ui_click(ui_window, g_field_center);
    ELI_ASSERT_TRUE(ctx->io.want_capture_keyboard);
    ELI_ASSERT_TRUE(ctx->io.want_text_input);
    eli_io_add_key_event(ELI_KEY_ENTER, true);                   /* commit + deactivate */
    test_ui_frame(ui_window);
    eli_io_add_key_event(ELI_KEY_ENTER, false);
    test_ui_frame(ui_window);
    ELI_ASSERT_FALSE(ctx->io.want_capture_keyboard);
    ELI_ASSERT_FALSE(ctx->io.want_text_input);
    test_ui_teardown(ctx);
}

ELI_TEST(next_frame_override_wins) {
    eli_context *ctx = test_ui_setup();
    eli_io_add_mouse_pos_event(600, 500);
    test_ui_frames(ui_window, 2);
    ELI_ASSERT_FALSE(ctx->io.want_capture_mouse);
    eli_get_io()->delta_time = 1.0f / 60.0f;
    eli_frame_begin();
    ui_window();
    eli_set_next_frame_want_capture_mouse(true);
    eli_frame_end();
    ELI_ASSERT_TRUE(ctx->io.want_capture_mouse);
    test_ui_teardown(ctx);
}

ELI_TEST_MAIN()
