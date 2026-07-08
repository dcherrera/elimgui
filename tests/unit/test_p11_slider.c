/**
 * @file test_p11_slider.c
 * @brief Phase 11 slider interaction tests driven across frames with backend mouse
 *        events inside a window: mouse-x maps across the track (endpoints + centre),
 *        the edited flag and return value fire only on an actual change, and a
 *        multi-component slider edits exactly one component.
 *
 * @status Phase 11 slider interaction coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/widgets/eli_slider.h>
#include <eli/widgets/eli_item_status.h>
#include <eli/font/eli_font.h>

#define TEST_DT (1.0f / 60.0f)

static eli_font_atlas *g_atlas = NULL;

static eli_context *slider_setup(void)
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

static void slider_teardown(eli_context *ctx)
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
    eli_set_next_window_size(eli_make_vec2(320.0f, 300.0f), 0);
    eli_begin("W", NULL, 0);
}

/* ------------------------------------------------------------------------- */

ELI_TEST(slider_int_maps_mouse_x_across_track) {
    eli_context *ctx = slider_setup();
    int v = 7;

    /* Frame 1: submit with a hidden label so the item rect equals the track. */
    eli_io_add_mouse_pos_event(300.0f, 260.0f);
    frame_begin();
    open_win();
    eli_slider_int("##S", &v, 0, 10, NULL, 0);
    eli_end();
    frame_end();
    eli_rect track = ctx->last_item_rect;
    float cy = track.y + track.h * 0.5f;

    /* Click hard-left -> minimum. */
    eli_io_add_mouse_pos_event(track.x + 1.0f, cy);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    frame_begin();
    open_win();
    bool changed = eli_slider_int("##S", &v, 0, 10, NULL, 0);
    eli_end();
    frame_end();
    ELI_ASSERT_TRUE(changed);
    ELI_ASSERT_EQ(0, v);

    /* Release, then click hard-right -> maximum. */
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, false);
    frame_begin(); open_win(); eli_slider_int("##S", &v, 0, 10, NULL, 0); eli_end(); frame_end();

    eli_io_add_mouse_pos_event(track.x + track.w - 1.0f, cy);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    frame_begin();
    open_win();
    eli_slider_int("##S", &v, 0, 10, NULL, 0);
    eli_end();
    frame_end();
    ELI_ASSERT_EQ(10, v);

    /* Release, then click dead-centre -> midpoint. */
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, false);
    frame_begin(); open_win(); eli_slider_int("##S", &v, 0, 10, NULL, 0); eli_end(); frame_end();

    eli_io_add_mouse_pos_event(track.x + track.w * 0.5f, cy);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    frame_begin();
    open_win();
    eli_slider_int("##S", &v, 0, 10, NULL, 0);
    eli_end();
    frame_end();
    ELI_ASSERT_EQ(5, v);

    slider_teardown(ctx);
}

ELI_TEST(slider_edited_and_return_only_on_change) {
    eli_context *ctx = slider_setup();
    float v = 0.0f;

    eli_io_add_mouse_pos_event(300.0f, 260.0f);
    frame_begin();
    open_win();
    eli_slider_float("##F", &v, 0.0f, 1.0f, "%.3f", 0);
    eli_end();
    frame_end();
    eli_rect track = ctx->last_item_rect;
    float cy = track.y + track.h * 0.5f;

    /* Click centre -> value moves to ~0.5, edited + return true. */
    eli_io_add_mouse_pos_event(track.x + track.w * 0.5f, cy);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    frame_begin();
    open_win();
    bool c1 = eli_slider_float("##F", &v, 0.0f, 1.0f, "%.3f", 0);
    bool e1 = eli_is_item_edited();
    eli_end();
    frame_end();
    ELI_ASSERT_TRUE(c1);
    ELI_ASSERT_TRUE(e1);
    ELI_ASSERT_FLT_NEAR(0.5f, v, 0.03f);

    /* Hold at the same spot -> no change, so no return / edited. */
    eli_io_add_mouse_pos_event(track.x + track.w * 0.5f, cy);
    frame_begin();
    open_win();
    bool c2 = eli_slider_float("##F", &v, 0.0f, 1.0f, "%.3f", 0);
    bool e2 = eli_is_item_edited();
    eli_end();
    frame_end();
    ELI_ASSERT_FALSE(c2);
    ELI_ASSERT_FALSE(e2);

    slider_teardown(ctx);
}

ELI_TEST(slider_float3_edits_one_component) {
    eli_context *ctx = slider_setup();
    float v[3] = {2.0f, 4.0f, 6.0f};

    eli_io_add_mouse_pos_event(300.0f, 260.0f);
    frame_begin();
    open_win();
    eli_slider_float3("V3", v, 0.0f, 10.0f, "%.2f", 0);
    eli_end();
    frame_end();
    eli_rect grp = ctx->last_item_rect;
    float cy = grp.y + grp.h * 0.5f;

    /* Click hard-left of the first sub-slider -> only component 0 -> min. */
    eli_io_add_mouse_pos_event(grp.x + 2.0f, cy);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    frame_begin();
    open_win();
    bool changed = eli_slider_float3("V3", v, 0.0f, 10.0f, "%.2f", 0);
    eli_end();
    frame_end();
    ELI_ASSERT_TRUE(changed);
    ELI_ASSERT_FLT_NEAR(0.0f, v[0], 0.0001f);
    ELI_ASSERT_FLT_NEAR(4.0f, v[1], 0.0001f);
    ELI_ASSERT_FLT_NEAR(6.0f, v[2], 0.0001f);

    slider_teardown(ctx);
}

ELI_TEST_MAIN()
