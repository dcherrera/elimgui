/**
 * @file test_style_stack.c
 * @brief Unit tests for the elimgui style push/pop stacks and the item-flag
 *        stack: color push/pop restores value + stack depth, float and vec2
 *        var push/pop mutate the right field then restore, wrong-kind pushes
 *        are rejected, and item flags nest correctly.
 *
 * @status Phase 6 style-stack coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/style/eli_style.h>

ELI_TEST(push_pop_style_color_restores) {
    eli_context *ctx = eli_create_context();
    eli_style_colors_dark(NULL);

    eli_vec4 original = ctx->style.colors[ELI_COL_TEXT];
    ELI_ASSERT_EQ(ctx->color_stack_size, 0);

    eli_push_style_color(ELI_COL_TEXT, ELI_COL32(10, 20, 30, 40));
    ELI_ASSERT_EQ(ctx->color_stack_size, 1);
    ELI_ASSERT_FLT_NEAR(ctx->style.colors[ELI_COL_TEXT].x, 10.0f / 255.0f, 1e-6);
    ELI_ASSERT_FLT_NEAR(ctx->style.colors[ELI_COL_TEXT].w, 40.0f / 255.0f, 1e-6);

    eli_push_style_color_vec4(ELI_COL_TEXT, eli_make_vec4(0.1f, 0.2f, 0.3f, 0.4f));
    ELI_ASSERT_EQ(ctx->color_stack_size, 2);
    ELI_ASSERT_FLT_NEAR(ctx->style.colors[ELI_COL_TEXT].y, 0.2f, 1e-6);

    eli_pop_style_color(2);
    ELI_ASSERT_EQ(ctx->color_stack_size, 0);
    ELI_ASSERT_FLT_NEAR(ctx->style.colors[ELI_COL_TEXT].x, original.x, 1e-6);
    ELI_ASSERT_FLT_NEAR(ctx->style.colors[ELI_COL_TEXT].w, original.w, 1e-6);

    eli_destroy_context(ctx);
}

ELI_TEST(push_pop_style_var_float) {
    eli_context *ctx = eli_create_context();

    float original = ctx->style.alpha;
    eli_push_style_var(ELI_STYLE_VAR_ALPHA, 0.25f);
    ELI_ASSERT_EQ(ctx->style_var_stack_size, 1);
    ELI_ASSERT_FLT_NEAR(ctx->style.alpha, 0.25f, 1e-6);

    eli_pop_style_var(1);
    ELI_ASSERT_EQ(ctx->style_var_stack_size, 0);
    ELI_ASSERT_FLT_NEAR(ctx->style.alpha, original, 1e-6);

    eli_destroy_context(ctx);
}

ELI_TEST(push_pop_style_var_vec2) {
    eli_context *ctx = eli_create_context();

    eli_vec2 original = ctx->style.window_padding;
    eli_push_style_var_vec2(ELI_STYLE_VAR_WINDOW_PADDING, eli_make_vec2(3.0f, 7.0f));
    ELI_ASSERT_EQ(ctx->style_var_stack_size, 1);
    ELI_ASSERT_FLT_NEAR(ctx->style.window_padding.x, 3.0f, 1e-6);
    ELI_ASSERT_FLT_NEAR(ctx->style.window_padding.y, 7.0f, 1e-6);

    eli_pop_style_var(1);
    ELI_ASSERT_EQ(ctx->style_var_stack_size, 0);
    ELI_ASSERT_FLT_NEAR(ctx->style.window_padding.x, original.x, 1e-6);
    ELI_ASSERT_FLT_NEAR(ctx->style.window_padding.y, original.y, 1e-6);

    eli_destroy_context(ctx);
}

ELI_TEST(push_style_var_wrong_kind_rejected) {
    eli_context *ctx = eli_create_context();

    float original_alpha = ctx->style.alpha;
    /* ALPHA is a float var; pushing it as vec2 must be rejected. */
    eli_push_style_var_vec2(ELI_STYLE_VAR_ALPHA, eli_make_vec2(9.0f, 9.0f));
    ELI_ASSERT_EQ(ctx->style_var_stack_size, 0);
    ELI_ASSERT_FLT_NEAR(ctx->style.alpha, original_alpha, 1e-6);

    eli_vec2 original_pad = ctx->style.window_padding;
    /* WINDOW_PADDING is a vec2 var; pushing it as float must be rejected. */
    eli_push_style_var(ELI_STYLE_VAR_WINDOW_PADDING, 5.0f);
    ELI_ASSERT_EQ(ctx->style_var_stack_size, 0);
    ELI_ASSERT_FLT_NEAR(ctx->style.window_padding.x, original_pad.x, 1e-6);

    eli_destroy_context(ctx);
}

ELI_TEST(item_flag_stack_nests) {
    eli_context *ctx = eli_create_context();

    ELI_ASSERT_EQ(ctx->current_item_flags, 0);

    eli_push_item_flag(ELI_ITEM_DISABLED, true);
    ELI_ASSERT_EQ(ctx->item_flags_stack_size, 1);
    ELI_ASSERT_TRUE((ctx->current_item_flags & ELI_ITEM_DISABLED) != 0);

    eli_push_item_flag(ELI_ITEM_BUTTON_REPEAT, true);
    ELI_ASSERT_TRUE((ctx->current_item_flags & ELI_ITEM_DISABLED) != 0);
    ELI_ASSERT_TRUE((ctx->current_item_flags & ELI_ITEM_BUTTON_REPEAT) != 0);

    /* Disabling clears a bit that was set below. */
    eli_push_item_flag(ELI_ITEM_DISABLED, false);
    ELI_ASSERT_FALSE((ctx->current_item_flags & ELI_ITEM_DISABLED) != 0);
    ELI_ASSERT_TRUE((ctx->current_item_flags & ELI_ITEM_BUTTON_REPEAT) != 0);

    eli_pop_item_flag();
    ELI_ASSERT_TRUE((ctx->current_item_flags & ELI_ITEM_DISABLED) != 0);
    eli_pop_item_flag();
    eli_pop_item_flag();
    ELI_ASSERT_EQ(ctx->item_flags_stack_size, 0);
    ELI_ASSERT_EQ(ctx->current_item_flags, 0);

    eli_destroy_context(ctx);
}

ELI_TEST(pop_more_than_pushed_is_clamped) {
    eli_context *ctx = eli_create_context();

    eli_push_style_color(ELI_COL_TEXT, ELI_COL32_WHITE);
    eli_pop_style_color(5);
    ELI_ASSERT_EQ(ctx->color_stack_size, 0);

    eli_push_style_var(ELI_STYLE_VAR_ALPHA, 0.5f);
    eli_pop_style_var(5);
    ELI_ASSERT_EQ(ctx->style_var_stack_size, 0);

    eli_destroy_context(ctx);
}

ELI_TEST_MAIN()
