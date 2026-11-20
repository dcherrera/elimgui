/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file test_draw_dash.c
 * @brief Dashed strokes: eli_draw_list_path_stroke_dashed and the dashed rect /
 *        circle outlines.
 *
 * Covers vertex counts (four per polyline segment), the dash phase carried
 * round corners (a dash bends round a corner and keeps its length), closed paths
 * holding a whole number of periods (no partial dash at the seam), the solid
 * fallback, and zero allocations once the draw list has grown (a second
 * identical frame allocates nothing).
 *
 * Dashes are read back from the vertices: each polyline segment's midline runs
 * from (v0+v3)/2 to (v1+v2)/2, and a dash breaks where one segment does not start
 * where the previous one ended.
 *
 * @status In use.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/draw/eli_draw.h>
#include <eli/util/eli_mem.h>

/* ---------------------------------------------------------------------------
 * Reading dashes back from the draw list
 * ------------------------------------------------------------------------- */

#define TEST_DASH_MAX 64

typedef struct test_dashes {
    int count;
    int segs[TEST_DASH_MAX];        /* polyline segments per dash */
    float len[TEST_DASH_MAX];       /* arc length per dash */
    eli_vec2 start[TEST_DASH_MAX];
    eli_vec2 end[TEST_DASH_MAX];
} test_dashes;

static eli_vec2 test_mid(eli_vec2 a, eli_vec2 b)
{
    return eli_make_vec2((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f);
}

static float test_dist(eli_vec2 a, eli_vec2 b)
{
    float dx = b.x - a.x, dy = b.y - a.y;
    return sqrtf(dx * dx + dy * dy);
}

/** Group the list's polyline segments into dashes. A segment is six indices
 *  (a, c, d, a, d, b): a/b are its first point's pair, c/d its second's, and a
 *  point is the middle of its pair. */
static test_dashes test_read_dashes(const eli_draw_list *dl)
{
    test_dashes d;
    memset(&d, 0, sizeof(d));
    for (uint32_t i = 0; i + 5 < dl->idx_count; i += 6) {
        const eli_draw_vert *a = &dl->vtx[dl->idx[i]], *c = &dl->vtx[dl->idx[i + 1]];
        const eli_draw_vert *dd = &dl->vtx[dl->idx[i + 2]], *b = &dl->vtx[dl->idx[i + 5]];
        eli_vec2 p1 = test_mid(eli_make_vec2(a->x, a->y), eli_make_vec2(b->x, b->y));
        eli_vec2 p2 = test_mid(eli_make_vec2(c->x, c->y), eli_make_vec2(dd->x, dd->y));
        bool joins = d.count > 0 && test_dist(d.end[d.count - 1], p1) < 1e-3f;
        if (!joins) {
            if (d.count == TEST_DASH_MAX)
                break;
            d.start[d.count++] = p1;
        }
        int k = d.count - 1;
        d.segs[k]++;
        d.len[k] += test_dist(p1, p2);
        d.end[k] = p2;
    }
    return d;
}

/* ---------------------------------------------------------------------------
 * Open paths
 * ------------------------------------------------------------------------- */

ELI_TEST(dash_straight_line_counts_and_positions)
{
    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);

    /* 100 px, dash 10 gap 5: dashes start at 0, 15, ... 90 -> 7 dashes of one
     * segment: 2 points (4 vertices) and 6 indices each. */
    eli_draw_list_path_line_to(&dl, eli_make_vec2(0, 0));
    eli_draw_list_path_line_to(&dl, eli_make_vec2(100, 0));
    eli_draw_list_path_stroke_dashed(&dl, ELI_COL32_WHITE, 0, 1.0f, 10.0f, 5.0f);

    ELI_ASSERT_EQ(dl.vtx_count, 28u);
    ELI_ASSERT_EQ(dl.idx_count, 42u);
    ELI_ASSERT_EQ(dl.path_count, 0);
    test_dashes d = test_read_dashes(&dl);
    ELI_ASSERT_EQ(d.count, 7);
    for (int k = 0; k < d.count; k++) {
        ELI_ASSERT_EQ(d.segs[k], 1);
        ELI_ASSERT_FLT_NEAR(d.start[k].x, 15.0f * (float)k, 1e-3);
        ELI_ASSERT_FLT_NEAR(d.len[k], 10.0f, 1e-3);
    }
    eli_draw_list_clear(&dl);
}

ELI_TEST(dash_phase_carries_round_a_corner)
{
    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);

    /* (0,0) -> (10,0) -> (10,10), dash 12 gap 4: the first dash runs 10 px along the
     * top and bends 2 px down the side (two segments, 12 px); the gap covers 12..16;
     * the last dash is the 4 px left, 16..20. */
    eli_draw_list_path_line_to(&dl, eli_make_vec2(0, 0));
    eli_draw_list_path_line_to(&dl, eli_make_vec2(10, 0));
    eli_draw_list_path_line_to(&dl, eli_make_vec2(10, 10));
    eli_draw_list_path_stroke_dashed(&dl, ELI_COL32_WHITE, 0, 1.0f, 12.0f, 4.0f);

    /* 3 points + 2 points, two vertices each. */
    ELI_ASSERT_EQ(dl.vtx_count, 10u);
    test_dashes d = test_read_dashes(&dl);
    ELI_ASSERT_EQ(d.count, 2);
    ELI_ASSERT_EQ(d.segs[0], 2);
    ELI_ASSERT_FLT_NEAR(d.len[0], 12.0f, 1e-3);
    ELI_ASSERT_FLT_NEAR(d.end[0].x, 10.0f, 1e-3);
    ELI_ASSERT_FLT_NEAR(d.end[0].y, 2.0f, 1e-3);
    ELI_ASSERT_EQ(d.segs[1], 1);
    ELI_ASSERT_FLT_NEAR(d.start[1].y, 6.0f, 1e-3);
    ELI_ASSERT_FLT_NEAR(d.len[1], 4.0f, 1e-3);
    eli_draw_list_clear(&dl);
}

ELI_TEST(dash_gap_across_a_corner_starts_next_dash_on_time)
{
    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);

    /* dash 8 gap 4 on the same L: the gap spans the corner (8..12), so the second
     * dash starts 2 px down the side, not at the corner. */
    eli_draw_list_path_line_to(&dl, eli_make_vec2(0, 0));
    eli_draw_list_path_line_to(&dl, eli_make_vec2(10, 0));
    eli_draw_list_path_line_to(&dl, eli_make_vec2(10, 10));
    eli_draw_list_path_stroke_dashed(&dl, ELI_COL32_WHITE, 0, 1.0f, 8.0f, 4.0f);

    test_dashes d = test_read_dashes(&dl);
    ELI_ASSERT_EQ(d.count, 2);
    ELI_ASSERT_FLT_NEAR(d.start[1].x, 10.0f, 1e-3);
    ELI_ASSERT_FLT_NEAR(d.start[1].y, 2.0f, 1e-3);
    ELI_ASSERT_FLT_NEAR(d.len[1], 8.0f, 1e-3);
    eli_draw_list_clear(&dl);
}

/* ---------------------------------------------------------------------------
 * Closed paths: rect and circle
 * ------------------------------------------------------------------------- */

ELI_TEST(dash_closed_square_whole_periods)
{
    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);

    /* 40x40 square, perimeter 160; dash 25 gap 5 (period 30) gives 5.33 periods,
     * rounded to 5: the pattern stretches to dash 26.67 gap 5.33. Every dash is the
     * same length and the last one ends one gap before the start point. */
    eli_draw_list_path_rect(&dl, eli_make_vec2(0, 0), eli_make_vec2(40, 40), 0.0f, 0);
    eli_draw_list_path_stroke_dashed(&dl, ELI_COL32_WHITE, ELI_DRAW_CLOSED, 1.0f, 25.0f, 5.0f);

    float stretch = 160.0f / 150.0f;
    test_dashes d = test_read_dashes(&dl);
    ELI_ASSERT_EQ(d.count, 5);
    for (int k = 0; k < d.count; k++)
        ELI_ASSERT_FLT_NEAR(d.len[k], 25.0f * stretch, 1e-2);
    /* Dash 2 starts at 32 (top side) and runs past the corner at 40. */
    ELI_ASSERT_EQ(d.segs[1], 2);
    ELI_ASSERT_FLT_NEAR(d.start[1].x, 30.0f * stretch, 1e-2);
    /* The last dash ends on the left side, 5.33 px above the start point. */
    ELI_ASSERT_FLT_NEAR(d.end[4].x, 0.0f, 1e-2);
    ELI_ASSERT_FLT_NEAR(d.end[4].y, 5.0f * stretch, 1e-2);
    int segs = 0, points = 0;
    for (int k = 0; k < d.count; k++) {
        segs += d.segs[k];
        points += d.segs[k] + 1;
    }
    ELI_ASSERT_EQ(dl.vtx_count, (uint32_t)(points * 2));
    ELI_ASSERT_EQ(dl.idx_count, (uint32_t)(segs * 6));
    eli_draw_list_clear(&dl);
}

ELI_TEST(dash_rect_matches_add_rect_geometry)
{
    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);

    /* Rounded 100x40 card outline: every dash lies on the add_rect outline, and the
     * dashes cover dash/(dash+gap) of its length. */
    eli_draw_list_add_rect_dashed(&dl, eli_make_vec2(10, 10), eli_make_vec2(110, 50),
                                  ELI_COL32_WHITE, 6.0f, 0, 1.0f, 4.0f, 3.0f);
    test_dashes d = test_read_dashes(&dl);

    eli_draw_list ref;
    eli_draw_list_init(&ref, ELI_DRAW_LIST_NONE);
    eli_draw_list_path_rect(&ref, eli_make_vec2(10.5f, 10.5f), eli_make_vec2(109.51f, 49.51f),
                            6.0f, 0);
    float perimeter = 0.0f;
    for (int i = 0; i < ref.path_count; i++)
        perimeter += test_dist(ref.path[i], ref.path[(i + 1) % ref.path_count]);

    float drawn = 0.0f;
    for (int k = 0; k < d.count; k++)
        drawn += d.len[k];
    float periods = floorf(perimeter / 7.0f + 0.5f);
    ELI_ASSERT_EQ(d.count, (int)periods);
    ELI_ASSERT_FLT_NEAR(drawn, perimeter * 4.0f / 7.0f, 1e-2);
    eli_draw_list_clear(&ref);
    eli_draw_list_clear(&dl);
}

ELI_TEST(dash_circle_whole_periods)
{
    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);

    eli_draw_list_add_circle_dashed(&dl, eli_make_vec2(50, 50), 20.0f, ELI_COL32_WHITE, 32, 1.5f,
                                    5.0f, 4.0f);
    /* The 32-gon of radius 19.5 add_circle strokes. */
    float perimeter = 32.0f * 2.0f * 19.5f * sinf(ELI_PI / 32.0f);
    float periods = floorf(perimeter / 9.0f + 0.5f);
    test_dashes d = test_read_dashes(&dl);
    ELI_ASSERT_EQ(d.count, (int)periods);
    float dash = 5.0f * perimeter / (periods * 9.0f);
    for (int k = 0; k < d.count; k++)
        ELI_ASSERT_FLT_NEAR(d.len[k], dash, 1e-2);
    ELI_ASSERT_EQ(dl.path_count, 0);
    eli_draw_list_clear(&dl);
}

/* ---------------------------------------------------------------------------
 * Edge cases
 * ------------------------------------------------------------------------- */

ELI_TEST(dash_zero_dash_or_gap_strokes_solid)
{
    eli_draw_list solid, dashed;
    eli_draw_list_init(&solid, ELI_DRAW_LIST_NONE);
    eli_draw_list_init(&dashed, ELI_DRAW_LIST_NONE);
    eli_draw_list_add_rect(&solid, eli_make_vec2(0, 0), eli_make_vec2(30, 20), ELI_COL32_WHITE,
                           4.0f, 0, 1.0f);
    eli_draw_list_add_rect_dashed(&dashed, eli_make_vec2(0, 0), eli_make_vec2(30, 20),
                                  ELI_COL32_WHITE, 4.0f, 0, 1.0f, 5.0f, 0.0f);
    ELI_ASSERT_EQ(dashed.vtx_count, solid.vtx_count);
    ELI_ASSERT_EQ(memcmp(dashed.vtx, solid.vtx, solid.vtx_count * sizeof(*solid.vtx)), 0);
    eli_draw_list_clear(&solid);
    eli_draw_list_clear(&dashed);
}

ELI_TEST(dash_transparent_or_short_path_draws_nothing)
{
    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);
    eli_draw_list_path_line_to(&dl, eli_make_vec2(0, 0));
    eli_draw_list_path_line_to(&dl, eli_make_vec2(50, 0));
    eli_draw_list_path_stroke_dashed(&dl, 0, 0, 1.0f, 5.0f, 5.0f);
    ELI_ASSERT_EQ(dl.vtx_count, 0u);
    ELI_ASSERT_EQ(dl.path_count, 0);
    eli_draw_list_path_line_to(&dl, eli_make_vec2(0, 0));
    eli_draw_list_path_stroke_dashed(&dl, ELI_COL32_WHITE, 0, 1.0f, 5.0f, 5.0f);
    ELI_ASSERT_EQ(dl.vtx_count, 0u);
    ELI_ASSERT_EQ(dl.path_count, 0);
    eli_draw_list_add_circle_dashed(&dl, eli_make_vec2(0, 0), 0.2f, ELI_COL32_WHITE, 0, 1.0f,
                                    2.0f, 2.0f);
    ELI_ASSERT_EQ(dl.vtx_count, 0u);
    eli_draw_list_clear(&dl);
}

/* ---------------------------------------------------------------------------
 * Allocations
 * ------------------------------------------------------------------------- */

static int g_dash_allocs;

static void *test_dash_alloc(size_t sz, void *user_data)
{
    (void)user_data;
    g_dash_allocs++;
    return malloc(sz);
}

static void test_dash_free(void *ptr, void *user_data)
{
    (void)user_data;
    free(ptr);
}

static void test_dash_frame(eli_draw_list *dl)
{
    eli_draw_list_reset(dl);
    for (int i = 0; i < 20; i++) {
        float y = 60.0f * (float)i;
        eli_draw_list_add_rect_dashed(dl, eli_make_vec2(0, y), eli_make_vec2(240, y + 48),
                                      ELI_COL32_WHITE, 6.0f, 0, 1.0f, 4.0f, 3.0f);
        eli_draw_list_add_circle_dashed(dl, eli_make_vec2(300, y + 24), 9.0f, ELI_COL32_WHITE, 0,
                                        1.5f, 3.0f, 2.0f);
    }
}

ELI_TEST(dash_no_allocations_after_draw_list_growth)
{
    eli_set_allocator_functions(test_dash_alloc, test_dash_free, NULL);
    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);

    test_dash_frame(&dl);   /* warm-up: the draw list grows to the frame's size */
    uint32_t vtx = dl.vtx_count;
    g_dash_allocs = 0;
    for (int frame = 0; frame < 3; frame++)
        test_dash_frame(&dl);
    ELI_ASSERT_EQ(g_dash_allocs, 0);
    ELI_ASSERT_EQ(dl.vtx_count, vtx);

    eli_draw_list_clear(&dl);
    eli_set_allocator_functions(NULL, NULL, NULL);
}

ELI_TEST_MAIN()
