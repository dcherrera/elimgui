/**
 * @file test_id_active.c
 * @brief Unit tests for elimgui active/hot id tracking: set/clear, the
 *        just-activated edge flag, and the previous-frame roll.
 *
 * @status Phase 5 active/hot ID coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/id/eli_id_active.h>

ELI_TEST(active_set_and_clear) {
    eli_context *ctx = eli_create_context();
    ELI_ASSERT_EQ(eli_get_active_id(), ELI_ID_NONE);

    eli_set_active_id(123u);
    ELI_ASSERT_EQ(eli_get_active_id(), 123u);
    ELI_ASSERT_TRUE(eli_is_active_id(123u));
    ELI_ASSERT_FALSE(eli_is_active_id(456u));

    eli_clear_active_id();
    ELI_ASSERT_EQ(eli_get_active_id(), ELI_ID_NONE);
    /* ELI_ID_NONE is never "active" even after an explicit clear. */
    ELI_ASSERT_FALSE(eli_is_active_id(ELI_ID_NONE));

    eli_destroy_context(ctx);
}

ELI_TEST(active_just_activated_edge) {
    eli_context *ctx = eli_create_context();

    eli_set_active_id(10u);
    ELI_ASSERT_TRUE(ctx->active_id_is_just_activated);

    /* Re-setting the same id is not a fresh activation. */
    eli_set_active_id(10u);
    ELI_ASSERT_FALSE(ctx->active_id_is_just_activated);

    /* Switching to a different id is a fresh activation again. */
    eli_set_active_id(20u);
    ELI_ASSERT_TRUE(ctx->active_id_is_just_activated);

    eli_destroy_context(ctx);
}

ELI_TEST(hot_id_set_and_get) {
    eli_context *ctx = eli_create_context();
    ELI_ASSERT_EQ(eli_get_hot_id(), ELI_ID_NONE);
    eli_set_hot_id(77u);
    ELI_ASSERT_EQ(eli_get_hot_id(), 77u);
    eli_destroy_context(ctx);
}

ELI_TEST(new_frame_rolls_previous_frame) {
    eli_context *ctx = eli_create_context();

    eli_set_active_id(5u);
    eli_set_hot_id(6u);
    ELI_ASSERT_TRUE(ctx->active_id_is_just_activated);

    eli_id_new_frame(ctx);
    ELI_ASSERT_EQ(ctx->active_id_previous_frame, 5u);
    ELI_ASSERT_EQ(ctx->hot_id_previous_frame, 6u);
    /* The edge flag resets each frame. */
    ELI_ASSERT_FALSE(ctx->active_id_is_just_activated);
    /* Current values are unchanged by the roll. */
    ELI_ASSERT_EQ(eli_get_active_id(), 5u);
    ELI_ASSERT_EQ(eli_get_hot_id(), 6u);

    eli_destroy_context(ctx);
}

ELI_TEST_MAIN()
