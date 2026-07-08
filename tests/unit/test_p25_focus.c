/**
 * @file test_p25_focus.c
 * @brief Phase 25 focus tests: eli_set_item_default_focus lands nav_id on the
 *        just-submitted item while the window is appearing; eli_set_keyboard_focus_here
 *        records a request that a later item consumes (offset skipping and the
 *        immediate -1 case).
 *
 * @status Phase 25 focus coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/interaction/eli_interaction.h>
#include <eli/layout/eli_layout.h>
#include <eli/widgets/eli_item_status.h>

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
    eli_begin("FocusWin", NULL, 0);
}

ELI_TEST(default_focus_lands_nav_on_appearing_window) {
    eli_context *ctx = setup();
    frame_begin();
    open_window();

    ELI_ASSERT_TRUE(ctx->current_window->appearing);

    eli_item_add(0x2001u, eli_make_rect(120.0f, 135.0f, 40.0f, 18.0f), 0);
    ELI_ASSERT_EQ(ctx->nav_id, 0u);

    eli_set_item_default_focus();
    ELI_ASSERT_EQ(ctx->nav_id, 0x2001u);
    ELI_ASSERT_TRUE(eli_is_item_focused());

    eli_end();
    eli_destroy_context(ctx);
}

ELI_TEST(keyboard_focus_request_set_and_consumed) {
    eli_context *ctx = setup();
    frame_begin();
    open_window();

    ELI_ASSERT_FALSE(eli_focus_has_keyboard_request());

    eli_set_keyboard_focus_here(0);
    ELI_ASSERT_TRUE(eli_focus_has_keyboard_request());

    /* The very next item consumes the offset-0 request. */
    ELI_ASSERT_TRUE(eli_focus_consume_keyboard_request(0x3001u));
    ELI_ASSERT_EQ(ctx->nav_id, 0x3001u);
    ELI_ASSERT_FALSE(eli_focus_has_keyboard_request());

    eli_end();
    eli_destroy_context(ctx);
}

ELI_TEST(keyboard_focus_request_skips_offset_items) {
    eli_context *ctx = setup();
    frame_begin();
    open_window();

    /* Focus the item submitted 2 items after the call. */
    eli_set_keyboard_focus_here(2);
    ELI_ASSERT_FALSE(eli_focus_consume_keyboard_request(0x1u));
    ELI_ASSERT_FALSE(eli_focus_consume_keyboard_request(0x2u));
    ELI_ASSERT_EQ(ctx->nav_id, 0u);
    ELI_ASSERT_TRUE(eli_focus_consume_keyboard_request(0x3u));
    ELI_ASSERT_EQ(ctx->nav_id, 0x3u);
    ELI_ASSERT_FALSE(eli_focus_has_keyboard_request());

    eli_end();
    eli_destroy_context(ctx);
}

ELI_TEST(keyboard_focus_negative_offset_targets_last_item) {
    eli_context *ctx = setup();
    frame_begin();
    open_window();

    eli_item_add(0x4001u, eli_make_rect(120.0f, 135.0f, 40.0f, 18.0f), 0);
    eli_set_keyboard_focus_here(-1);
    ELI_ASSERT_EQ(ctx->nav_id, 0x4001u);
    /* No pending forward request was left behind. */
    ELI_ASSERT_FALSE(eli_focus_has_keyboard_request());

    eli_end();
    eli_destroy_context(ctx);
}

ELI_TEST_MAIN()
