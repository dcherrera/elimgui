/**
 * @file test_integration_winid.c
 * @brief Verifies eli_begin seeds the id stack with the window id: the same
 *        widget label in two different windows derives DIFFERENT ids, ids are
 *        stable across frames, and the id stack is balanced across begin/end
 *        (including nested child windows).
 *
 * @status Integration coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/window/eli_window.h>

#define TEST_DT (1.0f / 60.0f)

static eli_context *winid_setup(void)
{
    eli_context *ctx = eli_create_context();
    ctx->io.delta_time = TEST_DT;
    ctx->io.display_size = eli_make_vec2(1024.0f, 768.0f);
    eli_style_colors_dark(&ctx->style);
    return ctx;
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

ELI_TEST(same_label_distinct_ids_across_windows) {
    eli_context *ctx = winid_setup();

    frame_begin();

    ELI_ASSERT_TRUE(eli_begin("WinA", NULL, 0));
    eli_id id_a = eli_get_id("Shared");
    ELI_ASSERT_NE(id_a, 0u);
    eli_end();

    ELI_ASSERT_TRUE(eli_begin("WinB", NULL, 0));
    eli_id id_b = eli_get_id("Shared");
    ELI_ASSERT_NE(id_b, 0u);
    eli_end();

    /* Same label, different window seed -> different id (the bug seeded both
     * from 0 and collided). */
    ELI_ASSERT_NE(id_a, id_b);

    frame_end();
    eli_destroy_context(ctx);
}

ELI_TEST(window_seed_id_stable_across_frames) {
    eli_context *ctx = winid_setup();

    frame_begin();
    ELI_ASSERT_TRUE(eli_begin("Persist", NULL, 0));
    eli_id id_frame1 = eli_get_id("Item");
    eli_end();
    frame_end();

    frame_begin();
    ELI_ASSERT_TRUE(eli_begin("Persist", NULL, 0));
    eli_id id_frame2 = eli_get_id("Item");
    eli_end();
    frame_end();

    /* Same window name -> same window id seed -> stable widget id each frame. */
    ELI_ASSERT_EQ(id_frame1, id_frame2);
    eli_destroy_context(ctx);
}

ELI_TEST(id_stack_balanced_across_begin_end) {
    eli_context *ctx = winid_setup();

    frame_begin();
    int depth_before = ctx->id_stack_size;

    ELI_ASSERT_TRUE(eli_begin("Outer", NULL, 0));
    /* Window pushes exactly one seed onto the id stack. */
    ELI_ASSERT_EQ(ctx->id_stack_size, depth_before + 1);

    eli_begin_child("Child", eli_make_vec2(80.0f, 80.0f), 0, 0);
    /* Child window pushes its own seed on top of the parent's. */
    ELI_ASSERT_EQ(ctx->id_stack_size, depth_before + 2);
    eli_end_child();

    ELI_ASSERT_EQ(ctx->id_stack_size, depth_before + 1);
    eli_end();

    /* Fully unwound back to the starting depth. */
    ELI_ASSERT_EQ(ctx->id_stack_size, depth_before);

    frame_end();
    eli_destroy_context(ctx);
}

ELI_TEST_MAIN()
