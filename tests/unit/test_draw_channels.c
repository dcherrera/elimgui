/**
 * @file test_draw_channels.c
 * @brief Unit tests for draw-list channel splitting: split/set_current/merge
 *        preserve command order and rebuild index offsets correctly.
 *
 * @status Phase 2 channels coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/draw/eli_draw.h>

ELI_TEST(channels_split_merge_preserves_order) {
    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);

    /* Base channel (0): rect A. */
    eli_draw_list_add_rect_filled(&dl, eli_make_vec2(10, 10), eli_make_vec2(50, 60),
                                  ELI_COL32_WHITE, 0.0f, ELI_DRAW_NONE);
    ELI_ASSERT_EQ(dl.vtx_count, 4u);
    ELI_ASSERT_EQ(dl.idx_count, 6u);

    eli_draw_list_channels_split(&dl, 2);
    eli_draw_list_channels_set_current(&dl, 1);
    /* Channel 1: rect B (vertices 4..7, shared vertex buffer). */
    eli_draw_list_add_rect_filled(&dl, eli_make_vec2(0, 0), eli_make_vec2(4, 4),
                                  ELI_COL32_RED, 0.0f, ELI_DRAW_NONE);
    ELI_ASSERT_EQ(dl.vtx_count, 8u); /* vertices are shared across channels */

    eli_draw_list_channels_merge(&dl);

    /* Two commands, base first then channel 1. */
    ELI_ASSERT_EQ(dl.cmd_count, 2u);
    ELI_ASSERT_EQ(dl.cmds[0].elem_count, 6u);
    ELI_ASSERT_EQ(dl.cmds[0].idx_offset, 0u);
    ELI_ASSERT_EQ(dl.cmds[1].elem_count, 6u);
    ELI_ASSERT_EQ(dl.cmds[1].idx_offset, 6u);

    /* Merged index buffer: base's 6, then channel 1's 6 (referencing verts 4..7). */
    ELI_ASSERT_EQ(dl.idx_count, 12u);
    ELI_ASSERT_EQ(dl.idx[0], 0);
    ELI_ASSERT_EQ(dl.idx[6], 4);
    ELI_ASSERT_EQ(dl.channels_count, 1);

    eli_draw_list_clear(&dl);
}

ELI_TEST(channels_order_independent_of_draw_order) {
    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);
    eli_draw_list_channels_split(&dl, 3);

    /* Draw into channels in reverse order; merge must still order 0,1,2. */
    eli_draw_list_channels_set_current(&dl, 2);
    eli_draw_list_add_rect_filled(&dl, eli_make_vec2(20, 20), eli_make_vec2(24, 24),
                                  ELI_COL32(3, 3, 3, 255), 0.0f, ELI_DRAW_NONE);
    eli_draw_list_channels_set_current(&dl, 1);
    eli_draw_list_add_rect_filled(&dl, eli_make_vec2(10, 10), eli_make_vec2(14, 14),
                                  ELI_COL32(2, 2, 2, 255), 0.0f, ELI_DRAW_NONE);
    eli_draw_list_channels_set_current(&dl, 0);
    eli_draw_list_add_rect_filled(&dl, eli_make_vec2(0, 0), eli_make_vec2(4, 4),
                                  ELI_COL32(1, 1, 1, 255), 0.0f, ELI_DRAW_NONE);

    eli_draw_list_channels_merge(&dl);

    ELI_ASSERT_EQ(dl.cmd_count, 3u);
    ELI_ASSERT_EQ(dl.idx_count, 18u);
    /* Command 0 came from channel 0 (drawn last) -> first vertex index it hit. */
    ELI_ASSERT_EQ(dl.cmds[0].idx_offset, 0u);
    ELI_ASSERT_EQ(dl.cmds[1].idx_offset, 6u);
    ELI_ASSERT_EQ(dl.cmds[2].idx_offset, 12u);

    /* Channel 0's rect was drawn last, so its vertices are 8..11; its first
     * index (command 0, offset 0) references vertex 8. */
    ELI_ASSERT_EQ(dl.idx[0], 8);
    /* Channel 2's rect was drawn first: vertices 0..3, now command 2. */
    ELI_ASSERT_EQ(dl.idx[12], 0);

    eli_draw_list_clear(&dl);
}

ELI_TEST_MAIN()
