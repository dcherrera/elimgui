/**
 * @file test_draw_idx32.c
 * @brief ELI_DRAW_IDX_32 (D5): one list holds well over 65,535 vertices with
 *        correct (non-wrapped) indices, e.g. a 4K monospace screen (~260K vertices).
 *
 * @status In use.
 * @issues None
 * @todo None
 */
#define ELI_DRAW_IDX_32
#include "eli_test.h"

#include <eli/elimgui.h>

ELI_TEST(index_type_is_32_bit) {
    ELI_ASSERT_EQ((int)sizeof(eli_draw_idx), 4);
}

ELI_TEST(list_holds_260k_vertices_without_wrapping) {
    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);
    int rects = 65000;                           /* 260,000 vertices */
    for (int i = 0; i < rects; i++) {
        float x = (float)(i % 480), y = (float)(i / 480);
        eli_draw_list_add_rect_filled(&dl, eli_make_vec2(x, y), eli_make_vec2(x + 1, y + 1),
                                      ELI_COL32_WHITE, 0.0f, ELI_DRAW_NONE);
    }
    ELI_ASSERT_EQ(dl.vtx_count, (uint32_t)rects * 4u);
    ELI_ASSERT_EQ(dl.dropped_vtx_count, 0u);
    int bad = 0;
    for (uint32_t k = 0; k < dl.idx_count; k++)
        if ((uint32_t)dl.idx[k] >= dl.vtx_count)
            bad++;
    ELI_ASSERT_EQ(bad, 0);
    /* The last rect's first index addresses vertex 259,996, not a wrapped value. */
    ELI_ASSERT_EQ((uint32_t)dl.idx[dl.idx_count - 6], 259996u);
    eli_draw_list_clear(&dl);
}

ELI_TEST_MAIN()
