/**
 * @file test_draw_idx.c
 * @brief 16-bit index guard (D5): crossing 65,536 vertices drops and counts the
 *        primitive instead of wrapping indices.
 *
 * @status In use.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/elimgui.h>

/** Every index of every command must address a vertex inside the list. */
static int count_bad_indices(const eli_draw_list *dl)
{
    int bad = 0;
    uint32_t idx_total = 0;
    for (uint32_t c = 0; c < dl->cmd_count; c++) {
        const eli_draw_cmd *cmd = &dl->cmds[c];
        for (uint32_t k = 0; k < cmd->elem_count; k++) {
            uint32_t idx = (uint32_t)dl->idx[cmd->idx_offset + k] + cmd->vtx_offset;
            if (idx >= dl->vtx_count)
                bad++;
        }
        idx_total += cmd->elem_count;
    }
    return bad + (idx_total != dl->idx_count);
}

/** Add `n` 1x1 filled rects (4 vertices each). */
static void add_rects(eli_draw_list *dl, int n)
{
    for (int i = 0; i < n; i++) {
        float x = (float)(i % 512);
        eli_draw_list_add_rect_filled(dl, eli_make_vec2(x, 0), eli_make_vec2(x + 1, 1),
                                      ELI_COL32_WHITE, 0.0f, ELI_DRAW_NONE);
    }
}

ELI_TEST(index_type_is_16_bit_by_default) {
    ELI_ASSERT_EQ((int)sizeof(eli_draw_idx), 2);
}

ELI_TEST(primitive_crossing_65536_vertices_is_dropped_and_counted) {
    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);
    add_rects(&dl, 16384);                       /* exactly 65,536 vertices: fits */
    ELI_ASSERT_EQ(dl.vtx_count, 65536u);
    ELI_ASSERT_EQ(dl.dropped_vtx_count, 0u);

    add_rects(&dl, 10);                          /* each would cross the limit */
    ELI_ASSERT_EQ(dl.vtx_count, 65536u);
    ELI_ASSERT_EQ(dl.dropped_vtx_count, 40u);
    ELI_ASSERT_EQ(count_bad_indices(&dl), 0);
    eli_draw_list_clear(&dl);
}

ELI_TEST(text_run_past_limit_is_dropped_whole) {
    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);
    add_rects(&dl, 16383);                       /* 65,532 vertices */
    uint32_t before = dl.vtx_count;
    ELI_ASSERT_FALSE(eli_draw_list_prim_reserve(&dl, 12, 8));
    ELI_ASSERT_EQ(dl.vtx_count, before);
    ELI_ASSERT_TRUE(eli_draw_list_prim_reserve(&dl, 6, 4));   /* last 4 still fit */
    eli_draw_list_prim_rect(&dl, eli_make_vec2(0, 0), eli_make_vec2(1, 1), ELI_COL32_WHITE);
    ELI_ASSERT_EQ(dl.vtx_count, 65536u);
    ELI_ASSERT_EQ(count_bad_indices(&dl), 0);
    eli_draw_list_clear(&dl);
}

ELI_TEST(reset_clears_the_drop_counter) {
    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);
    add_rects(&dl, 16385);
    ELI_ASSERT_EQ(dl.dropped_vtx_count, 4u);
    eli_draw_list_reset(&dl);
    ELI_ASSERT_EQ(dl.dropped_vtx_count, 0u);
    eli_draw_list_clear(&dl);
}

ELI_TEST(io_metric_reports_dropped_vertices) {
    eli_context *ctx = eli_create_context();
    ctx->io.display_size = eli_make_vec2(800, 600);
    ctx->io.delta_time = 1.0f / 60.0f;
    eli_new_frame();
    eli_input_update_begin_frame();
    eli_window_new_frame();
    eli_begin("Big", NULL, 0);
    add_rects(eli_get_window_draw_list(), 16500);
    eli_end();
    eli_window_render();
    eli_render();
    eli_input_update_end_frame();
    ELI_ASSERT_GT(ctx->io.metrics_dropped_vertices, 0);
    eli_destroy_context(ctx);
}

ELI_TEST_MAIN()
