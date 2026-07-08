/**
 * @file test_p24_value_display.c
 * @brief Unit tests for Phase 24 value-display widgets: each eli_value_* function
 *        advances the cursor to produce an item rect whose size matches the text size
 *        of the formatted "prefix: value" string; bool true/false produce distinct
 *        widths; eli_value_float honors a custom format vs the default "%.3f".
 *
 * @status Phase 24 value-display coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/widgets/eli_value.h>
#include <eli/font/eli_font.h>

#define TEST_DT (1.0f / 60.0f)

static eli_font_atlas *g_atlas = NULL;

static eli_context *value_setup(void)
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

static void value_teardown(eli_context *ctx)
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
    eli_set_next_window_size(eli_make_vec2(600.0f, 400.0f), 0);
    eli_begin("W", NULL, 0);
}

/* ------------------------------------------------------------------------- */

ELI_TEST(value_bool_true_item_rect_matches_text_size) {
    eli_context *ctx = value_setup();
    frame_begin();
    open_win();

    eli_value_bool("Status", true);
    eli_vec2 item_sz = eli_get_item_rect_size();
    eli_vec2 text_sz = eli_calc_text_size("Status: true", NULL);

    /* Item rect must be non-empty. */
    ELI_ASSERT_GT(item_sz.x, 0.0f);
    ELI_ASSERT_GT(item_sz.y, 0.0f);
    /* Item rect size must match the text measurement of the formatted string. */
    ELI_ASSERT_FLT_NEAR(item_sz.x, text_sz.x, 0.5f);
    ELI_ASSERT_FLT_NEAR(item_sz.y, text_sz.y, 0.5f);

    eli_end();
    frame_end();
    value_teardown(ctx);
}

ELI_TEST(value_bool_false_item_rect_matches_text_size) {
    eli_context *ctx = value_setup();
    frame_begin();
    open_win();

    eli_value_bool("Status", false);
    eli_vec2 item_sz = eli_get_item_rect_size();
    eli_vec2 text_sz = eli_calc_text_size("Status: false", NULL);

    ELI_ASSERT_GT(item_sz.x, 0.0f);
    ELI_ASSERT_GT(item_sz.y, 0.0f);
    ELI_ASSERT_FLT_NEAR(item_sz.x, text_sz.x, 0.5f);
    ELI_ASSERT_FLT_NEAR(item_sz.y, text_sz.y, 0.5f);

    eli_end();
    frame_end();
    value_teardown(ctx);
}

ELI_TEST(value_bool_true_and_false_have_distinct_widths) {
    /* "prefix: true" and "prefix: false" differ in length (4 vs 5 chars for the
     * value portion), so their rendered widths must differ. */
    eli_context *ctx = value_setup();

    /* Get text sizes in an idle frame (font is already pushed). */
    frame_begin();
    open_win();
    eli_vec2 true_sz  = eli_calc_text_size("X: true",  NULL);
    eli_vec2 false_sz = eli_calc_text_size("X: false", NULL);
    eli_end();
    frame_end();

    ELI_ASSERT_GT(false_sz.x, true_sz.x);

    /* Verify the actual widgets emit matching widths. */
    frame_begin();
    open_win();

    eli_value_bool("X", true);
    eli_vec2 item_true = eli_get_item_rect_size();

    eli_value_bool("X", false);
    eli_vec2 item_false = eli_get_item_rect_size();

    ELI_ASSERT_FLT_NEAR(item_true.x,  true_sz.x,  0.5f);
    ELI_ASSERT_FLT_NEAR(item_false.x, false_sz.x, 0.5f);
    ELI_ASSERT_GT(item_false.x, item_true.x);

    eli_end();
    frame_end();
    value_teardown(ctx);
}

ELI_TEST(value_int_item_rect_matches_text_size) {
    eli_context *ctx = value_setup();
    frame_begin();
    open_win();

    eli_value_int("Count", 42);
    eli_vec2 item_sz = eli_get_item_rect_size();
    eli_vec2 text_sz = eli_calc_text_size("Count: 42", NULL);

    ELI_ASSERT_GT(item_sz.x, 0.0f);
    ELI_ASSERT_GT(item_sz.y, 0.0f);
    ELI_ASSERT_FLT_NEAR(item_sz.x, text_sz.x, 0.5f);
    ELI_ASSERT_FLT_NEAR(item_sz.y, text_sz.y, 0.5f);

    eli_end();
    frame_end();
    value_teardown(ctx);
}

ELI_TEST(value_uint_item_rect_matches_text_size) {
    eli_context *ctx = value_setup();
    frame_begin();
    open_win();

    eli_value_uint("U", 99u);
    eli_vec2 item_sz = eli_get_item_rect_size();
    eli_vec2 text_sz = eli_calc_text_size("U: 99", NULL);

    ELI_ASSERT_GT(item_sz.x, 0.0f);
    ELI_ASSERT_GT(item_sz.y, 0.0f);
    ELI_ASSERT_FLT_NEAR(item_sz.x, text_sz.x, 0.5f);
    ELI_ASSERT_FLT_NEAR(item_sz.y, text_sz.y, 0.5f);

    eli_end();
    frame_end();
    value_teardown(ctx);
}

ELI_TEST(value_float_default_format_item_rect_matches) {
    /* Default format is "%.3f" → "F: 3.140" for 3.14f. */
    eli_context *ctx = value_setup();
    frame_begin();
    open_win();

    eli_value_float("F", 3.14f, NULL);
    eli_vec2 item_sz = eli_get_item_rect_size();
    eli_vec2 text_sz = eli_calc_text_size("F: 3.140", NULL);

    ELI_ASSERT_GT(item_sz.x, 0.0f);
    ELI_ASSERT_GT(item_sz.y, 0.0f);
    ELI_ASSERT_FLT_NEAR(item_sz.x, text_sz.x, 0.5f);
    ELI_ASSERT_FLT_NEAR(item_sz.y, text_sz.y, 0.5f);

    eli_end();
    frame_end();
    value_teardown(ctx);
}

ELI_TEST(value_float_custom_format_item_rect_matches) {
    /* Custom format "%.1f" → "F: 3.1" for 3.14f. */
    eli_context *ctx = value_setup();
    frame_begin();
    open_win();

    eli_value_float("F", 3.14f, "%.1f");
    eli_vec2 item_sz = eli_get_item_rect_size();
    eli_vec2 text_sz = eli_calc_text_size("F: 3.1", NULL);

    ELI_ASSERT_GT(item_sz.x, 0.0f);
    ELI_ASSERT_GT(item_sz.y, 0.0f);
    ELI_ASSERT_FLT_NEAR(item_sz.x, text_sz.x, 0.5f);
    ELI_ASSERT_FLT_NEAR(item_sz.y, text_sz.y, 0.5f);

    eli_end();
    frame_end();
    value_teardown(ctx);
}

ELI_TEST(value_float_custom_and_default_have_distinct_widths) {
    /* "F: 3.140" (default %.3f, 8 chars) vs "F: 3.1" (%.1f, 6 chars).
     * The default format produces more characters and therefore a wider item. */
    eli_context *ctx = value_setup();
    frame_begin();
    open_win();

    eli_value_float("F", 3.14f, NULL);
    eli_vec2 default_sz = eli_get_item_rect_size();

    eli_value_float("F", 3.14f, "%.1f");
    eli_vec2 custom_sz = eli_get_item_rect_size();

    /* Default ("%.3f") renders more decimal digits → wider output. */
    ELI_ASSERT_GT(default_sz.x, custom_sz.x);

    eli_end();
    frame_end();
    value_teardown(ctx);
}

ELI_TEST_MAIN()
