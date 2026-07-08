/**
 * @file test_p25_disabled.c
 * @brief Phase 25 disabling tests: eli_begin_disabled sets ELI_ITEM_DISABLED and
 *        dims style.alpha by disabled_alpha; eli_end_disabled restores both; nested
 *        disabled scopes stay disabled and dim alpha only once.
 *
 * @status Phase 25 disabling coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/interaction/eli_interaction.h>
#include <eli/layout/eli_layout.h>

#define TEST_DT (1.0f / 60.0f)

static eli_context *setup(void)
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
    eli_begin("DisableWin", NULL, 0);
}

ELI_TEST(begin_disabled_sets_flag_and_dims_alpha) {
    eli_context *ctx = setup();
    frame_begin();
    open_window();

    float alpha0 = ctx->style.alpha;
    ELI_ASSERT_FALSE((ctx->current_item_flags & ELI_ITEM_DISABLED) != 0);

    eli_begin_disabled(true);
    ELI_ASSERT_TRUE((ctx->current_item_flags & ELI_ITEM_DISABLED) != 0);
    ELI_ASSERT_FLT_NEAR(ctx->style.alpha, alpha0 * ctx->style.disabled_alpha, 0.0001f);
    ELI_ASSERT_LT(ctx->style.alpha, alpha0);

    eli_end_disabled();
    ELI_ASSERT_FALSE((ctx->current_item_flags & ELI_ITEM_DISABLED) != 0);
    ELI_ASSERT_FLT_NEAR(ctx->style.alpha, alpha0, 0.0001f);

    eli_end();
    eli_destroy_context(ctx);
}

ELI_TEST(begin_disabled_false_is_noop_scope) {
    eli_context *ctx = setup();
    frame_begin();
    open_window();

    float alpha0 = ctx->style.alpha;
    eli_begin_disabled(false);
    ELI_ASSERT_FALSE((ctx->current_item_flags & ELI_ITEM_DISABLED) != 0);
    ELI_ASSERT_FLT_NEAR(ctx->style.alpha, alpha0, 0.0001f);
    eli_end_disabled();
    ELI_ASSERT_FLT_NEAR(ctx->style.alpha, alpha0, 0.0001f);

    eli_end();
    eli_destroy_context(ctx);
}

ELI_TEST(nested_disabled_stays_disabled_and_dims_once) {
    eli_context *ctx = setup();
    frame_begin();
    open_window();

    float alpha0 = ctx->style.alpha;

    eli_begin_disabled(true);
    float alpha1 = ctx->style.alpha;
    ELI_ASSERT_FLT_NEAR(alpha1, alpha0 * ctx->style.disabled_alpha, 0.0001f);

    /* Inner begin_disabled(false) must not re-enable and must not dim again. */
    eli_begin_disabled(false);
    ELI_ASSERT_TRUE((ctx->current_item_flags & ELI_ITEM_DISABLED) != 0);
    ELI_ASSERT_FLT_NEAR(ctx->style.alpha, alpha1, 0.0001f);

    /* Inner true scope also stays at the same dimmed alpha (dims only on entry). */
    eli_begin_disabled(true);
    ELI_ASSERT_FLT_NEAR(ctx->style.alpha, alpha1, 0.0001f);
    eli_end_disabled();

    eli_end_disabled();
    /* Still inside the outer disabled scope. */
    ELI_ASSERT_TRUE((ctx->current_item_flags & ELI_ITEM_DISABLED) != 0);
    ELI_ASSERT_FLT_NEAR(ctx->style.alpha, alpha1, 0.0001f);

    eli_end_disabled();
    ELI_ASSERT_FALSE((ctx->current_item_flags & ELI_ITEM_DISABLED) != 0);
    ELI_ASSERT_FLT_NEAR(ctx->style.alpha, alpha0, 0.0001f);

    eli_end();
    eli_destroy_context(ctx);
}

ELI_TEST_MAIN()
