/**
 * @file test_layout_stack.c
 * @brief Unit tests for the item-width and text-wrap stacks: eli_push_item_width /
 *        eli_pop_item_width, eli_set_next_item_width, eli_calc_item_width (default,
 *        explicit, negative-from-right), and eli_push/pop_text_wrap_pos balance.
 *
 * @status Phase 8 item-width / text-wrap coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/layout/eli_layout.h>

#define TEST_DT (1.0f / 60.0f)

static eli_context *layout_setup(void)
{
    eli_context *ctx = eli_create_context();
    ctx->io.delta_time = TEST_DT;
    ctx->io.display_size = eli_make_vec2(1024.0f, 768.0f);
    ctx->font_size = 13.0f;
    return ctx;
}

static void frame_begin(void)
{
    eli_new_frame();
    eli_input_update_begin_frame();
    eli_window_new_frame();
}

static void open_window(void)
{
    eli_set_next_window_pos(eli_make_vec2(100.0f, 100.0f), 0, eli_make_vec2(0.0f, 0.0f));
    eli_set_next_window_size(eli_make_vec2(400.0f, 300.0f), 0);
    eli_begin("StackWin", NULL, 0);
}

ELI_TEST(calc_item_width_default) {
    eli_context *ctx = layout_setup();
    frame_begin();
    open_window();
    /* Default = trunc(size.x * 0.65) = trunc(400 * 0.65) = 260. */
    ELI_ASSERT_FLT_NEAR(eli_calc_item_width(), 260.0f, 0.01f);
    eli_end();
    eli_destroy_context(ctx);
}

ELI_TEST(push_pop_item_width) {
    eli_context *ctx = layout_setup();
    frame_begin();
    open_window();

    eli_push_item_width(120.0f);
    ELI_ASSERT_FLT_NEAR(eli_calc_item_width(), 120.0f, 0.01f);
    eli_push_item_width(60.0f);
    ELI_ASSERT_FLT_NEAR(eli_calc_item_width(), 60.0f, 0.01f);
    eli_pop_item_width();
    ELI_ASSERT_FLT_NEAR(eli_calc_item_width(), 120.0f, 0.01f);
    eli_pop_item_width();
    ELI_ASSERT_FLT_NEAR(eli_calc_item_width(), 260.0f, 0.01f);

    eli_end();
    eli_destroy_context(ctx);
}

ELI_TEST(set_next_item_width_overrides_once) {
    eli_context *ctx = layout_setup();
    frame_begin();
    open_window();

    eli_push_item_width(120.0f);
    eli_set_next_item_width(55.0f);
    ELI_ASSERT_FLT_NEAR(eli_calc_item_width(), 55.0f, 0.01f);

    /* item_add consumes the one-shot width; the pushed width returns. */
    eli_item_add(0u, eli_make_rect(108.0f, 127.0f, 55.0f, 19.0f), 0);
    ELI_ASSERT_FLT_NEAR(eli_calc_item_width(), 120.0f, 0.01f);

    eli_pop_item_width();
    eli_end();
    eli_destroy_context(ctx);
}

ELI_TEST(calc_item_width_negative_from_right) {
    eli_context *ctx = layout_setup();
    frame_begin();
    open_window();

    /* Negative width measures back from the work-area right edge (x=492).
     * cursor at x=108 => 492 - 108 + (-50) = 334. */
    eli_push_item_width(-50.0f);
    ELI_ASSERT_FLT_NEAR(eli_calc_item_width(), 334.0f, 0.5f);
    eli_pop_item_width();

    eli_end();
    eli_destroy_context(ctx);
}

ELI_TEST(text_wrap_pos_push_pop_balance) {
    eli_context *ctx = layout_setup();
    frame_begin();
    open_window();

    ELI_ASSERT_EQ(ctx->text_wrap_pos_stack_size, 0);
    eli_push_text_wrap_pos(0.0f);
    eli_push_text_wrap_pos(200.0f);
    ELI_ASSERT_EQ(ctx->text_wrap_pos_stack_size, 2);
    ELI_ASSERT_FLT_NEAR(ctx->text_wrap_pos_stack[1], 200.0f, 0.01f);
    eli_pop_text_wrap_pos();
    eli_pop_text_wrap_pos();
    ELI_ASSERT_EQ(ctx->text_wrap_pos_stack_size, 0);

    eli_end();
    eli_destroy_context(ctx);
}

ELI_TEST_MAIN()
