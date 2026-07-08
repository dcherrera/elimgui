/**
 * @file test_p33_optimization.c
 * @brief Phase 33 optimization invariants: geometric buffer growth, capacity
 *        retention across reset (buffer reuse), the split capacity-reserve /
 *        elem-charge primitives, draw-command batching/merging, and byte-for-byte
 *        identical geometry when a draw list is reused across frames.
 *
 * @status Phase 33 optimization coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/draw/eli_draw.h>
#include <eli/font/eli_font.h>

/* ---------------------------------------------------------------------------
 * Buffer reuse: reset keeps capacity, only clears counts
 * ------------------------------------------------------------------------- */

ELI_TEST(reset_retains_capacity_for_reuse) {
    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);

    for (int i = 0; i < 100; i++) {
        float x = (float)i;
        eli_draw_list_add_rect_filled(&dl, eli_make_vec2(x, x), eli_make_vec2(x + 1, x + 1),
                                      ELI_COL32_WHITE, 0.0f, ELI_DRAW_NONE);
    }
    uint32_t vtx_cap = dl.vtx_capacity;
    uint32_t idx_cap = dl.idx_capacity;
    uint32_t cmd_cap = dl.cmd_capacity;
    ELI_ASSERT_GT((int)vtx_cap, 0);
    ELI_ASSERT_GT((int)idx_cap, 0);
    eli_draw_vert *vtx_ptr = dl.vtx;

    eli_draw_list_reset(&dl);

    /* Counts cleared, but heap buffers (and their capacity) are retained so the
     * next frame draws without reallocating. */
    ELI_ASSERT_EQ(dl.vtx_count, 0u);
    ELI_ASSERT_EQ(dl.idx_count, 0u);
    ELI_ASSERT_EQ(dl.vtx_capacity, vtx_cap);
    ELI_ASSERT_EQ(dl.idx_capacity, idx_cap);
    ELI_ASSERT_EQ(dl.cmd_capacity, cmd_cap);
    ELI_ASSERT_EQ(dl.vtx, vtx_ptr); /* same allocation, not freed/reallocated */

    eli_draw_list_clear(&dl);
}

/* ---------------------------------------------------------------------------
 * Geometric growth: O(log n) reallocations, bounded overhead
 * ------------------------------------------------------------------------- */

ELI_TEST(buffer_growth_is_geometric) {
    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);

    const int rects = 4000; /* 16000 vtx, 24000 idx */
    uint32_t prev_vtx_cap = 0;
    int grow_events = 0;
    for (int i = 0; i < rects; i++) {
        float x = (float)(i % 500);
        eli_draw_list_add_rect_filled(&dl, eli_make_vec2(x, x), eli_make_vec2(x + 1, x + 1),
                                      ELI_COL32_WHITE, 0.0f, ELI_DRAW_NONE);
        if (dl.vtx_capacity != prev_vtx_cap) {
            grow_events++;
            prev_vtx_cap = dl.vtx_capacity;
        }
    }

    ELI_ASSERT_EQ(dl.vtx_count, (uint32_t)(rects * 4));
    /* Geometric ~1.5x growth reaches 16000 elements in well under 30 steps; a
     * linear (grow-by-constant) policy would need hundreds. */
    ELI_ASSERT_LT(grow_events, 30);
    /* Capacity is always sufficient and overhead stays bounded (< 2x count). */
    ELI_ASSERT_GE((int)dl.vtx_capacity, (int)dl.vtx_count);
    ELI_ASSERT_LT((int)dl.vtx_capacity, (int)dl.vtx_count * 2);

    eli_draw_list_clear(&dl);
}

/* Direct check of the growth helper's ~1.5x factor. */
ELI_TEST(buf_grow_uses_1_5x_factor) {
    int cap = 0;
    void *p = NULL;
    p = eli_draw_buf_grow(p, &cap, 1, sizeof(int));
    ELI_ASSERT_EQ(cap, 8); /* seeds at 8 */
    p = eli_draw_buf_grow(p, &cap, 9, sizeof(int));
    ELI_ASSERT_EQ(cap, 13); /* 8 + 8/2 + 1 */
    p = eli_draw_buf_grow(p, &cap, 14, sizeof(int));
    ELI_ASSERT_EQ(cap, 20); /* 13 + 13/2 + 1 */
    /* No-op when already large enough. */
    int before = cap;
    p = eli_draw_buf_grow(p, &cap, 5, sizeof(int));
    ELI_ASSERT_EQ(cap, before);
    free(p);
}

/* ---------------------------------------------------------------------------
 * Split reserve: capacity-only reservation vs. elem-count charging
 * ------------------------------------------------------------------------- */

ELI_TEST(reserve_capacity_does_not_charge_command) {
    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);

    ELI_ASSERT_EQ(dl.cmds[0].elem_count, 0u);
    eli_draw_list_prim_reserve_capacity(&dl, 60, 40);
    /* Buffers grew, but nothing was charged or counted yet. */
    ELI_ASSERT_GE((int)dl.idx_capacity, 60);
    ELI_ASSERT_GE((int)dl.vtx_capacity, 40);
    ELI_ASSERT_EQ(dl.cmds[0].elem_count, 0u);
    ELI_ASSERT_EQ(dl.vtx_count, 0u);
    ELI_ASSERT_EQ(dl.idx_count, 0u);

    eli_draw_list_prim_add_idx_to_cmd(&dl, 12);
    ELI_ASSERT_EQ(dl.cmds[0].elem_count, 12u);
    ELI_ASSERT_EQ(dl.vtx_count, 0u); /* charging does not touch counts */

    eli_draw_list_clear(&dl);
}

/* prim_reserve == reserve_capacity + add_idx_to_cmd (behavior preserved). */
ELI_TEST(prim_reserve_equals_capacity_plus_charge) {
    eli_draw_list a, b;
    eli_draw_list_init(&a, ELI_DRAW_LIST_NONE);
    eli_draw_list_init(&b, ELI_DRAW_LIST_NONE);

    eli_draw_list_prim_reserve(&a, 6, 4);

    eli_draw_list_prim_reserve_capacity(&b, 6, 4);
    eli_draw_list_prim_add_idx_to_cmd(&b, 6);

    ELI_ASSERT_EQ(a.cmds[0].elem_count, b.cmds[0].elem_count);
    ELI_ASSERT_GE((int)b.idx_capacity, 6);
    ELI_ASSERT_GE((int)b.vtx_capacity, 4);

    eli_draw_list_clear(&a);
    eli_draw_list_clear(&b);
}

/* ---------------------------------------------------------------------------
 * Draw-command batching / merging
 * ------------------------------------------------------------------------- */

ELI_TEST(same_clip_and_texture_batches_into_one_command) {
    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);

    for (int i = 0; i < 5; i++) {
        float x = (float)i * 4.0f;
        eli_draw_list_add_rect_filled(&dl, eli_make_vec2(x, x), eli_make_vec2(x + 2, x + 2),
                                      ELI_COL32_WHITE, 0.0f, ELI_DRAW_NONE);
    }
    /* Five quads under one clip/texture -> a single draw command of 30 indices. */
    ELI_ASSERT_EQ(dl.cmd_count, 1u);
    ELI_ASSERT_EQ(dl.cmds[0].elem_count, 30u);

    eli_draw_list_clear(&dl);
}

ELI_TEST(distinct_clip_splits_then_empty_scope_merges_back) {
    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);

    /* Geometry under clip A. */
    eli_draw_list_push_clip_rect(&dl, eli_make_vec2(0, 0), eli_make_vec2(50, 50), false);
    eli_draw_list_add_rect_filled(&dl, eli_make_vec2(1, 1), eli_make_vec2(3, 3), ELI_COL32_WHITE,
                                  0.0f, ELI_DRAW_NONE);
    ELI_ASSERT_EQ(dl.cmd_count, 1u);

    /* Popping to a different clip opens a new command. */
    eli_draw_list_pop_clip_rect(&dl);
    ELI_ASSERT_EQ(dl.cmd_count, 2u);

    /* Re-pushing clip A while the new command is still empty and contiguous with
     * the matching previous command merges the two back into one. */
    eli_draw_list_push_clip_rect(&dl, eli_make_vec2(0, 0), eli_make_vec2(50, 50), false);
    ELI_ASSERT_EQ(dl.cmd_count, 1u);
    eli_draw_list_add_rect_filled(&dl, eli_make_vec2(4, 4), eli_make_vec2(6, 6), ELI_COL32_WHITE,
                                  0.0f, ELI_DRAW_NONE);
    ELI_ASSERT_EQ(dl.cmd_count, 1u);
    ELI_ASSERT_EQ(dl.cmds[0].elem_count, 12u); /* both quads in one command */

    eli_draw_list_clear(&dl);
}

/* ---------------------------------------------------------------------------
 * Reuse across frames yields identical geometry
 * ------------------------------------------------------------------------- */

static void p33_fill_scene(eli_draw_list *dl) {
    eli_draw_list_add_rect_filled(dl, eli_make_vec2(2, 2), eli_make_vec2(40, 20), ELI_COL32_WHITE,
                                  3.0f, ELI_DRAW_ROUND_CORNERS_ALL);
    eli_draw_list_add_line(dl, eli_make_vec2(0, 0), eli_make_vec2(60, 60),
                           ELI_COL32(255, 0, 0, 255), 2.0f);
    eli_draw_list_add_circle_filled(dl, eli_make_vec2(30, 30), 12.0f, ELI_COL32(0, 255, 0, 255), 0);
    eli_draw_list_add_triangle_filled(dl, eli_make_vec2(0, 0), eli_make_vec2(10, 0),
                                      eli_make_vec2(5, 10), ELI_COL32(0, 0, 255, 255));
}

ELI_TEST(reused_list_produces_identical_geometry) {
    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);

    p33_fill_scene(&dl);
    uint32_t vtx0 = dl.vtx_count, idx0 = dl.idx_count, cmd0 = dl.cmd_count;
    size_t vtx_bytes = (size_t)vtx0 * sizeof(*dl.vtx);
    size_t idx_bytes = (size_t)idx0 * sizeof(*dl.idx);
    eli_draw_vert *snap_vtx = (eli_draw_vert *)malloc(vtx_bytes);
    eli_draw_idx *snap_idx = (eli_draw_idx *)malloc(idx_bytes);
    memcpy(snap_vtx, dl.vtx, vtx_bytes);
    memcpy(snap_idx, dl.idx, idx_bytes);

    /* Frame 2: same scene into the reset (capacity-retaining) list. */
    eli_draw_list_reset(&dl);
    p33_fill_scene(&dl);

    ELI_ASSERT_EQ(dl.vtx_count, vtx0);
    ELI_ASSERT_EQ(dl.idx_count, idx0);
    ELI_ASSERT_EQ(dl.cmd_count, cmd0);
    ELI_ASSERT_TRUE(memcmp(snap_vtx, dl.vtx, vtx_bytes) == 0);
    ELI_ASSERT_TRUE(memcmp(snap_idx, dl.idx, idx_bytes) == 0);

    free(snap_vtx);
    free(snap_idx);
    eli_draw_list_clear(&dl);
}

/* ---------------------------------------------------------------------------
 * Batched text emits identical geometry across reuse
 * ------------------------------------------------------------------------- */

ELI_TEST(batched_text_reuse_is_identical) {
    eli_font_atlas *atlas = eli_font_atlas_create();
    eli_font *f = eli_font_atlas_add_font_default(atlas, NULL);
    eli_font_atlas_build(atlas);

    static const char *s = "Batched text 0123456789\nsecond line";

    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);

    eli_draw_list_add_text_ex(&dl, f, f->font_size, eli_make_vec2(4, 4), ELI_COL32_WHITE, s, NULL,
                              0.0f, NULL);
    uint32_t vtx0 = dl.vtx_count, idx0 = dl.idx_count;
    /* The one command must be charged exactly the emitted index count. */
    ELI_ASSERT_EQ(dl.cmds[dl.cmd_count - 1].elem_count, idx0);
    ELI_ASSERT_EQ(idx0, vtx0 / 4u * 6u);

    size_t vb = (size_t)vtx0 * sizeof(*dl.vtx);
    eli_draw_vert *snap = (eli_draw_vert *)malloc(vb);
    memcpy(snap, dl.vtx, vb);

    eli_draw_list_reset(&dl);
    eli_draw_list_add_text_ex(&dl, f, f->font_size, eli_make_vec2(4, 4), ELI_COL32_WHITE, s, NULL,
                              0.0f, NULL);
    ELI_ASSERT_EQ(dl.vtx_count, vtx0);
    ELI_ASSERT_EQ(dl.idx_count, idx0);
    ELI_ASSERT_TRUE(memcmp(snap, dl.vtx, vb) == 0);

    free(snap);
    eli_draw_list_clear(&dl);
    eli_font_atlas_destroy(atlas);
}

ELI_TEST_MAIN()
