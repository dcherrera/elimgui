/**
 * @file test_integration_channels.c
 * @brief Regression test for the channels-merge trailing-command invariant: a
 *        merge whose channels emitted only culled (alpha-0) geometry must still
 *        leave the draw list with a live current command so a following
 *        clip-rect push/pop does not dereference a non-existent command.
 *
 * @status Integration coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/draw/eli_draw.h>

ELI_TEST(channels_merge_all_culled_keeps_current_cmd) {
    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);

    /* Split into channels and draw only fully-transparent rects into each: every
     * add_rect_filled early-outs on alpha 0, so no geometry or command is
     * emitted and both channels stay empty. */
    eli_draw_list_channels_split(&dl, 2);
    eli_draw_list_channels_set_current(&dl, 1);
    eli_draw_list_add_rect_filled(&dl, eli_make_vec2(0, 0), eli_make_vec2(4, 4),
                                  ELI_COL32(255, 0, 0, 0), 0.0f, ELI_DRAW_NONE);
    eli_draw_list_channels_set_current(&dl, 0);
    eli_draw_list_add_rect_filled(&dl, eli_make_vec2(10, 10), eli_make_vec2(50, 60),
                                  ELI_COL32(255, 255, 255, 0), 0.0f, ELI_DRAW_NONE);

    eli_draw_list_channels_merge(&dl);

    /* Invariant: never left with zero commands, and the trailing command is a
     * real (non-callback) draw command usable as the current command. */
    ELI_ASSERT_GE(dl.cmd_count, 1u);
    ELI_ASSERT_EQ(dl.channels_count, 1);
    ELI_ASSERT_NULL(dl.cmds[dl.cmd_count - 1].user_callback);
    ELI_ASSERT_EQ(dl.idx_count, 0u);
    ELI_ASSERT_EQ(dl.vtx_count, 0u);

    /* This is the exact sequence that used to SIGBUS: pushing then popping a clip
     * rect retags the current command, dereferencing cmds[cmd_count - 1]. */
    eli_draw_list_push_clip_rect(&dl, eli_make_vec2(5, 5), eli_make_vec2(20, 20), true);
    ELI_ASSERT_GE(dl.cmd_count, 1u);
    eli_draw_list_pop_clip_rect(&dl);
    ELI_ASSERT_GE(dl.cmd_count, 1u);

    /* Coherent list: still a valid non-callback trailing command after the pops. */
    ELI_ASSERT_NULL(dl.cmds[dl.cmd_count - 1].user_callback);

    eli_draw_list_clear(&dl);
    ELI_ASSERT_EQ(dl.cmd_count, 0u);
}

ELI_TEST(channels_merge_mixed_culled_and_real) {
    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);

    eli_draw_list_channels_split(&dl, 2);
    /* Channel 1: culled. Channel 0: one real opaque rect. */
    eli_draw_list_channels_set_current(&dl, 1);
    eli_draw_list_add_rect_filled(&dl, eli_make_vec2(0, 0), eli_make_vec2(4, 4),
                                  ELI_COL32(255, 0, 0, 0), 0.0f, ELI_DRAW_NONE);
    eli_draw_list_channels_set_current(&dl, 0);
    eli_draw_list_add_rect_filled(&dl, eli_make_vec2(10, 10), eli_make_vec2(50, 60),
                                  ELI_COL32_WHITE, 0.0f, ELI_DRAW_NONE);

    eli_draw_list_channels_merge(&dl);

    /* The real rect survives; no spurious extra trailing command is appended
     * because the last command already carries geometry. */
    ELI_ASSERT_EQ(dl.cmd_count, 1u);
    ELI_ASSERT_EQ(dl.cmds[0].elem_count, 6u);
    ELI_ASSERT_EQ(dl.idx_count, 6u);
    ELI_ASSERT_NULL(dl.cmds[dl.cmd_count - 1].user_callback);

    eli_draw_list_pop_clip_rect(&dl);
    ELI_ASSERT_GE(dl.cmd_count, 1u);

    eli_draw_list_clear(&dl);
}

ELI_TEST_MAIN()
