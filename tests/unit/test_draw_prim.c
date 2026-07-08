/**
 * @file test_draw_prim.c
 * @brief Unit tests for draw primitives: exact vertex/index counts and
 *        positions for filled rects, lines, polylines, convex fills, filled
 *        circles, and the multi-color rect.
 *
 * @status Phase 2 primitive coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/draw/eli_draw.h>

ELI_TEST(rect_filled_geometry_and_bounds) {
    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);

    eli_draw_list_add_rect_filled(&dl, eli_make_vec2(10, 10), eli_make_vec2(50, 60),
                                  ELI_COL32_WHITE, 0.0f, ELI_DRAW_NONE);
    ELI_ASSERT_EQ(dl.vtx_count, 4u);
    ELI_ASSERT_EQ(dl.idx_count, 6u);

    /* Corners: TL, TR, BR, BL. */
    ELI_ASSERT_FLT_NEAR(dl.vtx[0].x, 10.0f, 1e-4);
    ELI_ASSERT_FLT_NEAR(dl.vtx[0].y, 10.0f, 1e-4);
    ELI_ASSERT_FLT_NEAR(dl.vtx[1].x, 50.0f, 1e-4);
    ELI_ASSERT_FLT_NEAR(dl.vtx[1].y, 10.0f, 1e-4);
    ELI_ASSERT_FLT_NEAR(dl.vtx[2].x, 50.0f, 1e-4);
    ELI_ASSERT_FLT_NEAR(dl.vtx[2].y, 60.0f, 1e-4);
    ELI_ASSERT_FLT_NEAR(dl.vtx[3].x, 10.0f, 1e-4);
    ELI_ASSERT_FLT_NEAR(dl.vtx[3].y, 60.0f, 1e-4);
    for (int i = 0; i < 4; i++)
        ELI_ASSERT_EQ(dl.vtx[i].col, ELI_COL32_WHITE);

    /* Two triangles: 0,1,2 and 0,2,3. */
    ELI_ASSERT_EQ(dl.idx[0], 0);
    ELI_ASSERT_EQ(dl.idx[1], 1);
    ELI_ASSERT_EQ(dl.idx[2], 2);
    ELI_ASSERT_EQ(dl.idx[3], 0);
    ELI_ASSERT_EQ(dl.idx[4], 2);
    ELI_ASSERT_EQ(dl.idx[5], 3);

    eli_draw_list_clear(&dl);
}

ELI_TEST(line_vertex_positions) {
    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);

    /* Horizontal line, thickness 2. Non-AA one-segment quad => 4 vtx, 6 idx.
     * Endpoints are offset by (0.5,0.5); half-thickness normal is (0,1). */
    eli_draw_list_add_line(&dl, eli_make_vec2(0, 0), eli_make_vec2(10, 0), ELI_COL32_RED, 2.0f);
    ELI_ASSERT_EQ(dl.vtx_count, 4u);
    ELI_ASSERT_EQ(dl.idx_count, 6u);

    ELI_ASSERT_FLT_NEAR(dl.vtx[0].x, 0.5f, 1e-4);
    ELI_ASSERT_FLT_NEAR(dl.vtx[0].y, -0.5f, 1e-4);
    ELI_ASSERT_FLT_NEAR(dl.vtx[1].x, 10.5f, 1e-4);
    ELI_ASSERT_FLT_NEAR(dl.vtx[1].y, -0.5f, 1e-4);
    ELI_ASSERT_FLT_NEAR(dl.vtx[2].x, 10.5f, 1e-4);
    ELI_ASSERT_FLT_NEAR(dl.vtx[2].y, 1.5f, 1e-4);
    ELI_ASSERT_FLT_NEAR(dl.vtx[3].x, 0.5f, 1e-4);
    ELI_ASSERT_FLT_NEAR(dl.vtx[3].y, 1.5f, 1e-4);

    eli_draw_list_clear(&dl);
}

ELI_TEST(polyline_open_and_closed_counts) {
    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);

    eli_vec2 pts[3] = {{0, 0}, {10, 0}, {10, 10}};
    eli_draw_list_add_polyline(&dl, pts, 3, ELI_COL32_WHITE, ELI_DRAW_NONE, 1.0f);
    /* Open: 2 segments => 8 vtx, 12 idx. */
    ELI_ASSERT_EQ(dl.vtx_count, 8u);
    ELI_ASSERT_EQ(dl.idx_count, 12u);

    eli_draw_list_reset(&dl);
    eli_draw_list_add_polyline(&dl, pts, 3, ELI_COL32_WHITE, ELI_DRAW_CLOSED, 1.0f);
    /* Closed: 3 segments => 12 vtx, 18 idx. */
    ELI_ASSERT_EQ(dl.vtx_count, 12u);
    ELI_ASSERT_EQ(dl.idx_count, 18u);

    eli_draw_list_clear(&dl);
}

ELI_TEST(convex_poly_filled_counts) {
    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);

    eli_vec2 pts[5] = {{0, 0}, {10, 0}, {12, 8}, {6, 14}, {-2, 8}};
    eli_draw_list_add_convex_poly_filled(&dl, pts, 5, ELI_COL32_WHITE);
    /* n vertices, (n-2)*3 indices. */
    ELI_ASSERT_EQ(dl.vtx_count, 5u);
    ELI_ASSERT_EQ(dl.idx_count, 9u);
    ELI_ASSERT_EQ(dl.idx[0], 0);
    ELI_ASSERT_EQ(dl.idx[8], 4);

    eli_draw_list_clear(&dl);
}

ELI_TEST(circle_filled_segment_triangle_count) {
    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);

    /* Explicit 12 segments => 12-point convex fan => 12 vtx, 30 idx. */
    eli_draw_list_add_circle_filled(&dl, eli_make_vec2(30, 30), 12.0f, ELI_COL32_GREEN, 12);
    ELI_ASSERT_EQ(dl.vtx_count, 12u);
    ELI_ASSERT_EQ(dl.idx_count, 30u);
    ELI_ASSERT_EQ(dl.cmds[0].elem_count, 30u);

    /* First point sits at angle 0: (center.x + radius, center.y). */
    ELI_ASSERT_FLT_NEAR(dl.vtx[0].x, 42.0f, 1e-3);
    ELI_ASSERT_FLT_NEAR(dl.vtx[0].y, 30.0f, 1e-3);

    eli_draw_list_clear(&dl);
}

ELI_TEST(circle_stroke_segment_count) {
    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);

    /* Explicit 12 segments, closed stroke => 12 segments => 48 vtx, 72 idx. */
    eli_draw_list_add_circle(&dl, eli_make_vec2(30, 30), 12.0f, ELI_COL32_GREEN, 12, 1.0f);
    ELI_ASSERT_EQ(dl.vtx_count, 48u);
    ELI_ASSERT_EQ(dl.idx_count, 72u);

    eli_draw_list_clear(&dl);
}

ELI_TEST(rect_filled_multi_color_distinct_colors) {
    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);

    eli_col32 tl = ELI_COL32(255, 0, 0, 255);
    eli_col32 tr = ELI_COL32(0, 255, 0, 255);
    eli_col32 br = ELI_COL32(0, 0, 255, 255);
    eli_col32 bl = ELI_COL32(255, 255, 0, 255);
    eli_draw_list_add_rect_filled_multi_color(&dl, eli_make_vec2(0, 0), eli_make_vec2(10, 10),
                                              tl, tr, br, bl);
    ELI_ASSERT_EQ(dl.vtx_count, 4u);
    ELI_ASSERT_EQ(dl.idx_count, 6u);
    ELI_ASSERT_EQ(dl.vtx[0].col, tl);
    ELI_ASSERT_EQ(dl.vtx[1].col, tr);
    ELI_ASSERT_EQ(dl.vtx[2].col, br);
    ELI_ASSERT_EQ(dl.vtx[3].col, bl);

    eli_draw_list_clear(&dl);
}

ELI_TEST(transparent_color_is_noop) {
    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);
    eli_draw_list_add_rect_filled(&dl, eli_make_vec2(0, 0), eli_make_vec2(10, 10),
                                  ELI_COL32_BLACK_TRANS, 0.0f, ELI_DRAW_NONE);
    ELI_ASSERT_EQ(dl.vtx_count, 0u);
    ELI_ASSERT_EQ(dl.idx_count, 0u);
    eli_draw_list_clear(&dl);
}

ELI_TEST_MAIN()
