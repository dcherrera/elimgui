/**
 * @file test_p13_color.c
 * @brief Phase 13 color widget tests driven across frames with backend mouse events
 *        inside a window: color_button click reporting, color_edit4 component drag
 *        editing (updates the float[4] + marks edited), rgb<->hsv display-toggle value
 *        preservation, and interactive picker SV-square / hue-bar clicks moving the
 *        saturation-value / hue toward the click.
 *
 * @status Phase 13 color widget coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/widgets/eli_color.h>
#include <eli/widgets/eli_item_status.h>
#include <eli/font/eli_font.h>

#define TEST_DT (1.0f / 60.0f)

static eli_font_atlas *g_atlas = NULL;

static eli_context *color_setup(void)
{
    eli_context *ctx = eli_create_context();
    ctx->io.delta_time = TEST_DT;
    ctx->io.display_size = eli_make_vec2(1024.0f, 768.0f);
    g_atlas = eli_font_atlas_create();
    eli_font *f = eli_font_atlas_add_font_default(g_atlas, NULL);
    eli_font_atlas_build(g_atlas);
    eli_push_font(f);
    return ctx;
}

static void color_teardown(eli_context *ctx)
{
    eli_destroy_context(ctx);
    eli_font_atlas_destroy(g_atlas);
    g_atlas = NULL;
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
    eli_set_next_window_pos(eli_make_vec2(0.0f, 0.0f), 0, eli_make_vec2(0.0f, 0.0f));
    eli_set_next_window_size(eli_make_vec2(360.0f, 420.0f), 0);
    eli_begin("W", NULL, 0);
}

/* ------------------------------------------------------------------------- */

ELI_TEST(color_button_returns_true_on_click) {
    eli_context *ctx = color_setup();
    eli_vec4 col = eli_make_vec4(0.3f, 0.6f, 0.9f, 1.0f);

    /* Frame 1: submit to establish the swatch rect. */
    eli_io_add_mouse_pos_event(300.0f, 300.0f);
    frame_begin();
    open_win();
    eli_color_button("btn", col, 0, eli_make_vec2(40.0f, 40.0f));
    eli_end();
    frame_end();
    eli_rect bb = ctx->last_item_rect;
    float cx = bb.x + bb.w * 0.5f;
    float cy = bb.y + bb.h * 0.5f;

    /* Frame 2: press down over the swatch (activate, not yet pressed). */
    eli_io_add_mouse_pos_event(cx, cy);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    frame_begin();
    open_win();
    bool p2 = eli_color_button("btn", col, 0, eli_make_vec2(40.0f, 40.0f));
    eli_end();
    frame_end();
    ELI_ASSERT_FALSE(p2);

    /* Frame 3: release inside -> press fires. */
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, false);
    frame_begin();
    open_win();
    bool p3 = eli_color_button("btn", col, 0, eli_make_vec2(40.0f, 40.0f));
    eli_end();
    frame_end();
    ELI_ASSERT_TRUE(p3);

    color_teardown(ctx);
}

ELI_TEST(color_edit4_exposes_rgba_and_edits_component) {
    eli_context *ctx = color_setup();
    float col[4] = { 0.4f, 0.5f, 0.6f, 0.7f };

    /* Frame 1: submit (default UINT8 RGB display) to establish the group rect. */
    eli_io_add_mouse_pos_event(10.0f, 10.0f);
    frame_begin();
    open_win();
    eli_color_edit4("##C", col, 0);
    eli_end();
    frame_end();
    eli_rect grp = ctx->last_item_rect;
    /* The first (red) component drag sits at the group's left edge. */
    float cx = grp.x + 8.0f;
    float cy = grp.y + grp.h * 0.5f;

    /* Frame 2: press to activate the red component drag (no movement yet). */
    eli_io_add_mouse_pos_event(cx, cy);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    frame_begin();
    open_win();
    bool c2 = eli_color_edit4("##C", col, 0);
    eli_end();
    frame_end();
    ELI_ASSERT_FALSE(c2);
    ELI_ASSERT_FLT_NEAR(0.4f, col[0], 0.0001f);
    /* Untouched components are intact. */
    ELI_ASSERT_FLT_NEAR(0.5f, col[1], 0.0001f);
    ELI_ASSERT_FLT_NEAR(0.7f, col[3], 0.0001f);

    /* Frame 3: drag right 20px at UINT8 speed 1 -> red 102 -> 122. */
    eli_io_add_mouse_pos_event(cx + 20.0f, cy);
    frame_begin();
    open_win();
    bool c3 = eli_color_edit4("##C", col, 0);
    bool e3 = eli_is_item_edited();
    eli_end();
    frame_end();
    ELI_ASSERT_TRUE(c3);
    ELI_ASSERT_TRUE(e3);
    ELI_ASSERT_FLT_NEAR(122.0f / 255.0f, col[0], 0.002f);
    /* UINT8 display round-trips every component through /255 on an edit, so the
     * untouched channels land on the nearest 8-bit step (within 1/255). */
    ELI_ASSERT_FLT_NEAR(0.5f, col[1], 0.004f);
    ELI_ASSERT_FLT_NEAR(0.7f, col[3], 0.004f);

    color_teardown(ctx);
}

ELI_TEST(color_edit_rgb_hsv_toggle_preserves_value) {
    eli_context *ctx = color_setup();
    float col[4] = { 0.7f, 0.35f, 0.1f, 1.0f };
    float before[4] = { col[0], col[1], col[2], col[3] };

    /* No mouse interaction: a display-mode toggle must not mutate the color. */
    eli_io_add_mouse_pos_event(5.0f, 5.0f);
    frame_begin();
    open_win();
    eli_color_edit4("##RGB", col, ELI_COLOR_EDIT_DISPLAY_RGB);
    eli_color_edit4("##HSV", col, ELI_COLOR_EDIT_DISPLAY_HSV);
    eli_end();
    frame_end();

    ELI_ASSERT_FLT_NEAR(before[0], col[0], 0.0001f);
    ELI_ASSERT_FLT_NEAR(before[1], col[1], 0.0001f);
    ELI_ASSERT_FLT_NEAR(before[2], col[2], 0.0001f);

    /* And the HSV round-trip itself preserves the RGB value. */
    float h, s, v, r, g, b;
    eli_color_convert_rgb_to_hsv(before[0], before[1], before[2], &h, &s, &v);
    eli_color_convert_hsv_to_rgb(h, s, v, &r, &g, &b);
    ELI_ASSERT_FLT_NEAR(before[0], r, 0.0005f);
    ELI_ASSERT_FLT_NEAR(before[1], g, 0.0005f);
    ELI_ASSERT_FLT_NEAR(before[2], b, 0.0005f);

    color_teardown(ctx);
}

/* Picker config that shows only the SV square + hue bar, so the layout math is
 * deterministic: group width = sv_size + inner_spacing.x + one frame height. */
#define PICKER_FLAGS \
    (ELI_COLOR_EDIT_NO_SIDE_PREVIEW | ELI_COLOR_EDIT_NO_ALPHA | ELI_COLOR_EDIT_NO_INPUTS)

ELI_TEST(picker_sv_click_changes_saturation_value) {
    eli_context *ctx = color_setup();
    /* Pure red: hue 0, saturation 1, value 1. */
    float col[4] = { 1.0f, 0.0f, 0.0f, 1.0f };

    eli_io_add_mouse_pos_event(5.0f, 5.0f);
    frame_begin();
    open_win();
    eli_color_picker4("##P", col, PICKER_FLAGS, NULL);
    eli_end();
    frame_end();
    eli_rect grp = ctx->last_item_rect;
    float square = eli_get_frame_height();
    float spacing = ctx->style.item_inner_spacing.x;
    float sv_size = grp.w - spacing - square;
    ELI_ASSERT_GT(sv_size, 20.0f);

    /* Click the center of the SV square -> S ~ 0.5, V ~ 0.5. */
    float mx = grp.x + sv_size * 0.5f;
    float my = grp.y + sv_size * 0.5f;
    eli_io_add_mouse_pos_event(mx, my);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    frame_begin();
    open_win();
    bool changed = eli_color_picker4("##P", col, PICKER_FLAGS, NULL);
    eli_end();
    frame_end();

    ELI_ASSERT_TRUE(changed);
    float h, s, v;
    eli_color_convert_rgb_to_hsv(col[0], col[1], col[2], &h, &s, &v);
    /* Saturation and value both moved down toward the click (0.5). */
    ELI_ASSERT_LT(s, 0.9f);
    ELI_ASSERT_LT(v, 0.9f);
    ELI_ASSERT_FLT_NEAR(0.5f, s, 0.15f);
    ELI_ASSERT_FLT_NEAR(0.5f, v, 0.15f);
    /* Hue unchanged (still red). */
    ELI_ASSERT_FLT_NEAR(0.0f, h, 0.02f);

    color_teardown(ctx);
}

ELI_TEST(picker_hue_click_changes_hue) {
    eli_context *ctx = color_setup();
    /* Pure red: hue 0. */
    float col[4] = { 1.0f, 0.0f, 0.0f, 1.0f };

    eli_io_add_mouse_pos_event(5.0f, 5.0f);
    frame_begin();
    open_win();
    eli_color_picker4("##P", col, PICKER_FLAGS, NULL);
    eli_end();
    frame_end();
    eli_rect grp = ctx->last_item_rect;
    float square = eli_get_frame_height();
    float spacing = ctx->style.item_inner_spacing.x;
    float sv_size = grp.w - spacing - square;

    /* Hue bar spans [grp.x + sv_size + spacing, +square] horizontally, full sv_size
     * vertically. Click one third down -> hue ~ 1/3 -> green. */
    float bar_x = grp.x + sv_size + spacing + square * 0.5f;
    float bar_y = grp.y + sv_size * (1.0f / 3.0f);
    eli_io_add_mouse_pos_event(bar_x, bar_y);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    frame_begin();
    open_win();
    bool changed = eli_color_picker4("##P", col, PICKER_FLAGS, NULL);
    eli_end();
    frame_end();

    ELI_ASSERT_TRUE(changed);
    float h, s, v;
    eli_color_convert_rgb_to_hsv(col[0], col[1], col[2], &h, &s, &v);
    /* Hue moved off red toward green (~0.33); S and V stay maxed. */
    ELI_ASSERT_FLT_NEAR(1.0f / 3.0f, h, 0.03f);
    ELI_ASSERT_FLT_NEAR(1.0f, s, 0.02f);
    ELI_ASSERT_FLT_NEAR(1.0f, v, 0.02f);
    /* Green dominates the resulting RGB. */
    ELI_ASSERT_GT(col[1], col[0]);
    ELI_ASSERT_GT(col[1], col[2]);

    color_teardown(ctx);
}

ELI_TEST_MAIN()
