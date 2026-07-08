/**
 * @file test_p12_input_number.c
 * @brief Phase 12 tests for the numeric inputs: eli_input_int parsing a typed
 *        value, the +/- step buttons stepping the bound value, type-range clamping
 *        via eli_input_scalar, and eli_input_float round-tripping a typed number.
 *        Editing is driven across frames through the input backend.
 *
 * @status Phase 12 numeric-input coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/widgets/eli_input_number.h>
#include <eli/font/eli_font.h>

#define TEST_DT (1.0f / 60.0f)

static eli_font_atlas *g_atlas = NULL;

static eli_context *setup(void)
{
    eli_context *ctx = eli_create_context();
    ctx->io.delta_time = TEST_DT;
    ctx->io.display_size = eli_make_vec2(1024.0f, 768.0f);

    eli_style_colors_dark(&ctx->style);
    g_atlas = eli_font_atlas_create();
    eli_font *f = eli_font_atlas_add_font_default(g_atlas, NULL);
    eli_font_atlas_build(g_atlas);
    eli_push_font(f);
    return ctx;
}

static void teardown(eli_context *ctx)
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
    eli_set_next_window_size(eli_make_vec2(300.0f, 300.0f), 0);
    eli_begin("W", NULL, 0);
}

static eli_vec2 rect_center(eli_rect r)
{
    return eli_make_vec2(r.x + r.w * 0.5f, r.y + r.h * 0.5f);
}

static void type_string(const char *s)
{
    for (const char *p = s; *p != '\0'; p++)
        eli_io_add_input_character((unsigned int)(unsigned char)*p);
}

/* ------------------------------------------------------------------------- */

ELI_TEST(input_int_parses_typed_value) {
    eli_context *ctx = setup();
    int v = 0;

    /* Empty label + no step => the whole widget is the field (last item). */
    eli_io_add_mouse_pos_event(20.0f, 40.0f);
    frame_begin();
    open_win();
    eli_input_int("", &v, 0, 0, 0);
    eli_vec2 c = rect_center(ctx->last_item_rect);
    eli_end();
    frame_end();

    /* Click to activate (auto-selects the "0"). */
    eli_io_add_mouse_pos_event(c.x, c.y);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    frame_begin();
    open_win();
    eli_input_int("", &v, 0, 0, 0);
    eli_end();
    frame_end();

    /* Type "42" replacing the selection; parses back into v. */
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, false);
    frame_begin();
    open_win();
    type_string("42");
    eli_input_int("", &v, 0, 0, 0);
    eli_end();
    frame_end();

    ELI_ASSERT_EQ(v, 42);
    teardown(ctx);
}

ELI_TEST(input_int_step_button_increments) {
    eli_context *ctx = setup();
    int v = 0;

    /* Empty label + step => the last item is the "+" button. */
    eli_io_add_mouse_pos_event(20.0f, 40.0f);
    frame_begin();
    open_win();
    eli_input_int("", &v, 5, 0, 0);
    eli_vec2 plus = rect_center(ctx->last_item_rect);
    eli_end();
    frame_end();

    /* Press the "+" button. */
    eli_io_add_mouse_pos_event(plus.x, plus.y);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    frame_begin();
    open_win();
    eli_input_int("", &v, 5, 0, 0);
    eli_end();
    frame_end();

    /* Release inside -> step applied. */
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, false);
    frame_begin();
    open_win();
    eli_input_int("", &v, 5, 0, 0);
    eli_end();
    frame_end();

    ELI_ASSERT_EQ(v, 5);
    teardown(ctx);
}

ELI_TEST(input_scalar_clamps_to_type_range) {
    eli_context *ctx = setup();
    uint8_t v = 0;

    eli_io_add_mouse_pos_event(20.0f, 40.0f);
    frame_begin();
    open_win();
    eli_input_scalar("", ELI_DATA_TYPE_U8, &v, NULL, NULL, NULL, 0);
    eli_vec2 c = rect_center(ctx->last_item_rect);
    eli_end();
    frame_end();

    eli_io_add_mouse_pos_event(c.x, c.y);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    frame_begin();
    open_win();
    eli_input_scalar("", ELI_DATA_TYPE_U8, &v, NULL, NULL, NULL, 0);
    eli_end();
    frame_end();

    /* Type 300 -> parsed as 300 -> clamped into the uint8 range (255). */
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, false);
    frame_begin();
    open_win();
    type_string("300");
    eli_input_scalar("", ELI_DATA_TYPE_U8, &v, NULL, NULL, NULL, 0);
    eli_end();
    frame_end();

    ELI_ASSERT_EQ((int)v, 255);
    teardown(ctx);
}

ELI_TEST(input_float_round_trips_typed_number) {
    eli_context *ctx = setup();
    float v = 0.0f;

    eli_io_add_mouse_pos_event(20.0f, 40.0f);
    frame_begin();
    open_win();
    eli_input_float("", &v, 0.0f, 0.0f, "%.3f", 0);
    eli_vec2 c = rect_center(ctx->last_item_rect);
    eli_end();
    frame_end();

    eli_io_add_mouse_pos_event(c.x, c.y);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    frame_begin();
    open_win();
    eli_input_float("", &v, 0.0f, 0.0f, "%.3f", 0);
    eli_end();
    frame_end();

    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, false);
    frame_begin();
    open_win();
    type_string("3.14");
    eli_input_float("", &v, 0.0f, 0.0f, "%.3f", 0);
    eli_end();
    frame_end();

    ELI_ASSERT_FLT_NEAR(v, 3.14f, 0.001f);
    teardown(ctx);
}

ELI_TEST(input_number_clamp_helper) {
    ELI_ASSERT_FLT_NEAR((float)eli_input_number__clamp(ELI_DATA_TYPE_U8, 300.0), 255.0f, 0.01f);
    ELI_ASSERT_FLT_NEAR((float)eli_input_number__clamp(ELI_DATA_TYPE_U8, -5.0), 0.0f, 0.01f);
    ELI_ASSERT_FLT_NEAR((float)eli_input_number__clamp(ELI_DATA_TYPE_S8, 200.0), 127.0f, 0.01f);
    ELI_ASSERT_FLT_NEAR((float)eli_input_number__clamp(ELI_DATA_TYPE_S32, 5.0), 5.0f, 0.01f);
}

ELI_TEST_MAIN()
