/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file eli_test_ui.h
 * @brief Shared harness for UI-level unit tests: a context with the default font,
 *        and one-call frames that run eli_frame_begin/end around a UI callback.
 *
 * @status In use by the Phase E input tests.
 * @issues None
 * @todo None
 */
#ifndef ELI_TEST_UI_H
#define ELI_TEST_UI_H

#include <eli/elimgui.h>

static eli_font_atlas *g_test_ui_atlas = NULL;

/** Create a context (dark style, default font, 800x600, mac behaviors off). */
static inline eli_context *test_ui_setup(void)
{
    eli_context *ctx = eli_create_context();
    ctx->io.display_size = eli_make_vec2(800.0f, 600.0f);
    ctx->io.config_mac_os_behaviors = false;
    eli_style_colors_dark(&ctx->style);
    g_test_ui_atlas = eli_font_atlas_create();
    eli_font *f = eli_font_atlas_add_font_default(g_test_ui_atlas, NULL);
    eli_font_atlas_build(g_test_ui_atlas);
    eli_push_font(f);
    return ctx;
}

/** Destroy the context and font atlas from test_ui_setup. */
static inline void test_ui_teardown(eli_context *ctx)
{
    eli_destroy_context(ctx);
    eli_font_atlas_destroy(g_test_ui_atlas);
    g_test_ui_atlas = NULL;
}

/** Run one full frame around `ui` (may be NULL). */
static inline void test_ui_frame(void (*ui)(void))
{
    eli_get_io()->delta_time = 1.0f / 60.0f;
    eli_frame_begin();
    if (ui)
        ui();
    eli_frame_end();
}

/** Run `n` frames around `ui`. */
static inline void test_ui_frames(void (*ui)(void), int n)
{
    for (int i = 0; i < n; i++)
        test_ui_frame(ui);
}

/** Press (and release next frame) a chord: modifiers + key. */
static inline void test_ui_chord(void (*ui)(void), int mods_keys_count, const eli_key *keys)
{
    for (int i = 0; i < mods_keys_count; i++)
        eli_io_add_key_event(keys[i], true);
    test_ui_frame(ui);
    for (int i = mods_keys_count - 1; i >= 0; i--)
        eli_io_add_key_event(keys[i], false);
    test_ui_frame(ui);
}

/** Click at a point (press frame, release frame). */
static inline void test_ui_click(void (*ui)(void), eli_vec2 p)
{
    eli_io_add_mouse_pos_event(p.x, p.y);
    eli_io_add_mouse_button_event(0, true);
    test_ui_frame(ui);
    eli_io_add_mouse_button_event(0, false);
    test_ui_frame(ui);
}

#endif /* ELI_TEST_UI_H */
