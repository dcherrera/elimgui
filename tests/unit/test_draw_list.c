/**
 * @file test_draw_list.c
 * @brief Unit tests for the draw-list core: lifecycle, buffer growth, clip and
 *        texture stacks, command batching, prim reservation, and clone_output.
 *
 * @status Phase 2 draw-list coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/draw/eli_draw.h>

ELI_TEST(init_opens_single_empty_cmd) {
    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);
    ELI_ASSERT_EQ(dl.cmd_count, 1u);
    ELI_ASSERT_EQ(dl.cmds[0].elem_count, 0u);
    ELI_ASSERT_EQ(dl.vtx_count, 0u);
    ELI_ASSERT_EQ(dl.idx_count, 0u);
    ELI_ASSERT_EQ(dl.clip_rect_stack_count, 0);
    ELI_ASSERT_EQ(dl.texture_stack_count, 0);
    eli_draw_list_clear(&dl);
    ELI_ASSERT_NULL(dl.cmds);
    ELI_ASSERT_EQ(dl.cmd_count, 0u);
}

ELI_TEST(clip_rect_stack_push_pop_and_query) {
    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);

    /* Default clip is fullscreen. */
    eli_vec2 fs_min = eli_draw_list_get_clip_rect_min(&dl);
    ELI_ASSERT_FLT_NEAR(fs_min.x, -ELI_DRAW_CLIP_FULLSCREEN_EXTENT, 1e-3);

    eli_draw_list_push_clip_rect(&dl, eli_make_vec2(5, 5), eli_make_vec2(20, 20), false);
    ELI_ASSERT_EQ(dl.clip_rect_stack_count, 1);
    eli_vec2 mn = eli_draw_list_get_clip_rect_min(&dl);
    eli_vec2 mx = eli_draw_list_get_clip_rect_max(&dl);
    ELI_ASSERT_FLT_NEAR(mn.x, 5.0f, 1e-4);
    ELI_ASSERT_FLT_NEAR(mn.y, 5.0f, 1e-4);
    ELI_ASSERT_FLT_NEAR(mx.x, 20.0f, 1e-4);
    ELI_ASSERT_FLT_NEAR(mx.y, 20.0f, 1e-4);

    /* Intersect a larger rect with the current one: clamps back to (5,5)-(20,20). */
    eli_draw_list_push_clip_rect(&dl, eli_make_vec2(0, 0), eli_make_vec2(100, 100), true);
    ELI_ASSERT_EQ(dl.clip_rect_stack_count, 2);
    mn = eli_draw_list_get_clip_rect_min(&dl);
    mx = eli_draw_list_get_clip_rect_max(&dl);
    ELI_ASSERT_FLT_NEAR(mn.x, 5.0f, 1e-4);
    ELI_ASSERT_FLT_NEAR(mx.x, 20.0f, 1e-4);

    eli_draw_list_pop_clip_rect(&dl);
    ELI_ASSERT_EQ(dl.clip_rect_stack_count, 1);
    mx = eli_draw_list_get_clip_rect_max(&dl);
    ELI_ASSERT_FLT_NEAR(mx.x, 20.0f, 1e-4);

    eli_draw_list_pop_clip_rect(&dl);
    ELI_ASSERT_EQ(dl.clip_rect_stack_count, 0);
    mn = eli_draw_list_get_clip_rect_min(&dl);
    ELI_ASSERT_FLT_NEAR(mn.x, -ELI_DRAW_CLIP_FULLSCREEN_EXTENT, 1e-3);

    eli_draw_list_clear(&dl);
}

ELI_TEST(clip_change_splits_command) {
    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);

    eli_draw_list_add_rect_filled(&dl, eli_make_vec2(10, 10), eli_make_vec2(50, 60),
                                  ELI_COL32_WHITE, 0.0f, ELI_DRAW_NONE);
    ELI_ASSERT_EQ(dl.cmd_count, 1u);
    ELI_ASSERT_EQ(dl.cmds[0].elem_count, 6u);

    /* Pushing a clip after geometry with a different clip opens a new command. */
    eli_draw_list_push_clip_rect(&dl, eli_make_vec2(5, 5), eli_make_vec2(20, 20), false);
    ELI_ASSERT_EQ(dl.cmd_count, 2u);
    ELI_ASSERT_EQ(dl.cmds[1].elem_count, 0u);
    ELI_ASSERT_FLT_NEAR(dl.cmds[1].clip_rect.x, 5.0f, 1e-4);
    ELI_ASSERT_FLT_NEAR(dl.cmds[1].clip_rect.w, 15.0f, 1e-4);

    eli_draw_list_add_rect_filled(&dl, eli_make_vec2(6, 6), eli_make_vec2(18, 18),
                                  ELI_COL32_RED, 0.0f, ELI_DRAW_NONE);
    ELI_ASSERT_EQ(dl.cmd_count, 2u);
    ELI_ASSERT_EQ(dl.cmds[1].elem_count, 6u);

    eli_draw_list_clear(&dl);
}

ELI_TEST(texture_stack_tracks_current) {
    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);
    ELI_ASSERT_EQ(dl.cmd_texture_id, 0u);

    eli_draw_list_push_texture_id(&dl, 7u);
    ELI_ASSERT_EQ(dl.texture_stack_count, 1);
    ELI_ASSERT_EQ(dl.cmd_texture_id, 7u);

    eli_draw_list_push_texture_id(&dl, 9u);
    ELI_ASSERT_EQ(dl.cmd_texture_id, 9u);

    eli_draw_list_pop_texture_id(&dl);
    ELI_ASSERT_EQ(dl.cmd_texture_id, 7u);
    eli_draw_list_pop_texture_id(&dl);
    ELI_ASSERT_EQ(dl.cmd_texture_id, 0u);
    ELI_ASSERT_EQ(dl.texture_stack_count, 0);

    eli_draw_list_clear(&dl);
}

ELI_TEST(prim_reserve_and_write) {
    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);

    eli_draw_list_prim_reserve(&dl, 6, 4);
    ELI_ASSERT_GE((int)dl.vtx_capacity, 4);
    ELI_ASSERT_GE((int)dl.idx_capacity, 6);
    ELI_ASSERT_EQ(dl.cmds[0].elem_count, 6u);

    eli_draw_list_prim_rect(&dl, eli_make_vec2(0, 0), eli_make_vec2(2, 2), ELI_COL32_WHITE);
    ELI_ASSERT_EQ(dl.vtx_count, 4u);
    ELI_ASSERT_EQ(dl.idx_count, 6u);
    ELI_ASSERT_EQ(dl.vtx_current_idx, 4u);

    eli_draw_list_clear(&dl);
}

ELI_TEST(buffer_growth_across_many_primitives) {
    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);

    const int count = 100;
    for (int i = 0; i < count; i++) {
        float x = (float)i;
        eli_draw_list_add_rect_filled(&dl, eli_make_vec2(x, x), eli_make_vec2(x + 1, x + 1),
                                      ELI_COL32_WHITE, 0.0f, ELI_DRAW_NONE);
    }
    ELI_ASSERT_EQ(dl.vtx_count, (uint32_t)(count * 4));
    ELI_ASSERT_EQ(dl.idx_count, (uint32_t)(count * 6));
    ELI_ASSERT_GE((int)dl.vtx_capacity, count * 4);
    ELI_ASSERT_GE((int)dl.idx_capacity, count * 6);
    /* Same clip/texture the whole time: one merged command. */
    ELI_ASSERT_EQ(dl.cmd_count, 1u);
    ELI_ASSERT_EQ(dl.cmds[0].elem_count, (uint32_t)(count * 6));

    eli_draw_list_clear(&dl);
}

ELI_TEST(clone_output_deep_copies_buffers) {
    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);
    eli_draw_list_add_rect_filled(&dl, eli_make_vec2(10, 10), eli_make_vec2(50, 60),
                                  ELI_COL32_WHITE, 0.0f, ELI_DRAW_NONE);

    eli_draw_list *clone = eli_draw_list_clone_output(&dl);
    ELI_ASSERT_NOT_NULL(clone);
    ELI_ASSERT_EQ(clone->vtx_count, dl.vtx_count);
    ELI_ASSERT_EQ(clone->idx_count, dl.idx_count);
    ELI_ASSERT_EQ(clone->cmd_count, dl.cmd_count);
    ELI_ASSERT_NE(clone->vtx, dl.vtx); /* distinct storage */
    ELI_ASSERT_FLT_NEAR(clone->vtx[2].x, 50.0f, 1e-4);
    ELI_ASSERT_FLT_NEAR(clone->vtx[2].y, 60.0f, 1e-4);

    eli_draw_list_clear(clone);
    free(clone);
    eli_draw_list_clear(&dl);
}

ELI_TEST_MAIN()
