/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file test_draw_arc_fast.c
 * @brief Arc tessellation table: eli_draw_list_path_arc_to_fast
 *        reads a unit-circle table instead of calling cosf/sinf per point, and
 *        rounded rectangles stay bit-identical to the per-point trig they replace.
 *
 * cosf/sinf are redirected to counting wrappers for this translation unit, so the
 * tests can assert that steady-state rounded rects make no trig calls. The
 * reference arcs call the real functions through `(cosf)(x)`.
 *
 * @status In use.
 * @issues None
 * @todo None
 */
#include <math.h>

static long g_trig_calls;

static float test_count_cosf(float x)
{
    g_trig_calls++;
    return (cosf)(x);
}

static float test_count_sinf(float x)
{
    g_trig_calls++;
    return (sinf)(x);
}

#define cosf(x) test_count_cosf(x)
#define sinf(x) test_count_sinf(x)

#include "eli_test.h"

#include <eli/draw/eli_draw.h>

/* ---------------------------------------------------------------------------
 * Reference: the previous per-point trig
 * ------------------------------------------------------------------------- */

/** The previous arc_to_fast body: cosf/sinf of each 1/12 step. */
static void ref_arc_to_fast(eli_draw_list *list, eli_vec2 center, float radius, int a_min,
                            int a_max)
{
    if (radius < 0.5f) {
        eli_draw_list_path_line_to(list, center);
        return;
    }
    int step = (a_max >= a_min) ? 1 : -1;
    for (int a = a_min;; a += step) {
        float angle = ((float)a / 12.0f) * (2.0f * ELI_PI);
        eli_draw_list_path_line_to(list, eli_make_vec2(center.x + (cosf)(angle) * radius,
                                                       center.y + (sinf)(angle) * radius));
        if (a == a_max)
            break;
    }
}

/** eli_draw_list_path_rect with the reference arcs (same clamp and corner order). */
static void ref_path_rect(eli_draw_list *list, eli_vec2 a, eli_vec2 b, float rounding,
                          eli_draw_flags flags)
{
    flags = eli_draw_fix_rect_corner_flags(flags);
    float wf = ((flags & ELI_DRAW_ROUND_CORNERS_TOP) == ELI_DRAW_ROUND_CORNERS_TOP ||
                (flags & ELI_DRAW_ROUND_CORNERS_BOTTOM) == ELI_DRAW_ROUND_CORNERS_BOTTOM)
                   ? 0.5f
                   : 1.0f;
    float hf = ((flags & ELI_DRAW_ROUND_CORNERS_LEFT) == ELI_DRAW_ROUND_CORNERS_LEFT ||
                (flags & ELI_DRAW_ROUND_CORNERS_RIGHT) == ELI_DRAW_ROUND_CORNERS_RIGHT)
                   ? 0.5f
                   : 1.0f;
    rounding = eli_min_f(rounding, fabsf(b.x - a.x) * wf - 1.0f);
    rounding = eli_min_f(rounding, fabsf(b.y - a.y) * hf - 1.0f);
    float r_tl = (flags & ELI_DRAW_ROUND_CORNERS_TOP_LEFT) ? rounding : 0.0f;
    float r_tr = (flags & ELI_DRAW_ROUND_CORNERS_TOP_RIGHT) ? rounding : 0.0f;
    float r_br = (flags & ELI_DRAW_ROUND_CORNERS_BOT_RIGHT) ? rounding : 0.0f;
    float r_bl = (flags & ELI_DRAW_ROUND_CORNERS_BOT_LEFT) ? rounding : 0.0f;
    ref_arc_to_fast(list, eli_make_vec2(a.x + r_tl, a.y + r_tl), r_tl, 6, 9);
    ref_arc_to_fast(list, eli_make_vec2(b.x - r_tr, a.y + r_tr), r_tr, 9, 12);
    ref_arc_to_fast(list, eli_make_vec2(b.x - r_br, b.y - r_br), r_br, 0, 3);
    ref_arc_to_fast(list, eli_make_vec2(a.x + r_bl, b.y - r_bl), r_bl, 3, 6);
}

/* ---------------------------------------------------------------------------
 * Helpers
 * ------------------------------------------------------------------------- */

/** @return true if both lists hold the same path points, bit for bit. */
static bool test_paths_equal(const eli_draw_list *a, const eli_draw_list *b)
{
    if (a->path_count != b->path_count)
        return false;
    for (int i = 0; i < a->path_count; i++)
        if (a->path[i].x != b->path[i].x || a->path[i].y != b->path[i].y)
            return false;
    return true;
}

/** @return true if both lists hold bit-identical vertices and indices. */
static bool test_geometry_equal(const eli_draw_list *a, const eli_draw_list *b)
{
    if (a->vtx_count != b->vtx_count || a->idx_count != b->idx_count)
        return false;
    for (uint32_t i = 0; i < a->vtx_count; i++) {
        const eli_draw_vert *va = &a->vtx[i], *vb = &b->vtx[i];
        if (va->x != vb->x || va->y != vb->y || va->u != vb->u || va->v != vb->v ||
            va->col != vb->col)
            return false;
    }
    return memcmp(a->idx, b->idx, a->idx_count * sizeof(*a->idx)) == 0;
}

/* ---------------------------------------------------------------------------
 * Tests
 * ------------------------------------------------------------------------- */

ELI_TEST(arc_fast_points_match_trig_bitwise) {
    static const float radii[] = {0.25f, 0.5f, 1.0f, 3.7f, 5.0f, 9.0f, 100.25f};
    eli_draw_list got, want;
    eli_draw_list_init(&got, ELI_DRAW_LIST_NONE);
    eli_draw_list_init(&want, ELI_DRAW_LIST_NONE);
    int mismatches = 0;
    for (size_t r = 0; r < sizeof(radii) / sizeof(radii[0]); r++) {
        /* In-range steps plus out-of-range ones (handled by direct trig). */
        for (int a_min = -3; a_min <= 15; a_min++) {
            for (int a_max = -3; a_max <= 15; a_max++) {
                eli_vec2 c = eli_make_vec2(10.3f, -4.7f);
                eli_draw_list_path_clear(&got);
                eli_draw_list_path_clear(&want);
                eli_draw_list_path_arc_to_fast(&got, c, radii[r], a_min, a_max);
                ref_arc_to_fast(&want, c, radii[r], a_min, a_max);
                if (!test_paths_equal(&got, &want))
                    mismatches++;
            }
        }
    }
    ELI_ASSERT_EQ(mismatches, 0);
    eli_draw_list_clear(&got);
    eli_draw_list_clear(&want);
}

ELI_TEST(rounded_rects_are_bit_identical) {
    static const float roundings[] = {0.5f, 2.0f, 5.0f, 9.0f, 40.0f};
    static const eli_draw_flags corners[] = {
        ELI_DRAW_NONE, ELI_DRAW_ROUND_CORNERS_TOP, ELI_DRAW_ROUND_CORNERS_LEFT,
        ELI_DRAW_ROUND_CORNERS_BOT_RIGHT,
    };
    eli_draw_list got, want;
    eli_draw_list_init(&got, ELI_DRAW_LIST_NONE);
    eli_draw_list_init(&want, ELI_DRAW_LIST_NONE);
    for (size_t r = 0; r < sizeof(roundings) / sizeof(roundings[0]); r++) {
        for (size_t c = 0; c < sizeof(corners) / sizeof(corners[0]); c++) {
            eli_vec2 a = eli_make_vec2(3.25f + (float)r, 7.5f), b = eli_make_vec2(91.0f, 40.75f);
            eli_draw_list_add_rect_filled(&got, a, b, ELI_COL32_WHITE, roundings[r], corners[c]);
            eli_draw_list_add_rect(&got, a, b, ELI_COL32_BLACK, roundings[r], corners[c], 1.5f);

            ref_path_rect(&want, a, b, roundings[r], corners[c]);
            eli_draw_list_path_fill_convex(&want, ELI_COL32_WHITE);
            ref_path_rect(&want, eli_make_vec2(a.x + 0.5f, a.y + 0.5f),
                          eli_make_vec2(b.x - 0.49f, b.y - 0.49f), roundings[r], corners[c]);
            eli_draw_list_path_stroke(&want, ELI_COL32_BLACK, ELI_DRAW_CLOSED, 1.5f);
        }
    }
    ELI_ASSERT_GT((int)got.vtx_count, 0);
    ELI_ASSERT_TRUE(test_geometry_equal(&got, &want));
    eli_draw_list_clear(&got);
    eli_draw_list_clear(&want);
}

ELI_TEST(rounded_rects_make_no_trig_calls) {
    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);
    eli_vec2 a = eli_make_vec2(0.0f, 0.0f), b = eli_make_vec2(60.0f, 20.0f);
    eli_draw_list_add_rect_filled(&dl, a, b, ELI_COL32_WHITE, 5.0f, ELI_DRAW_NONE); /* warm-up */

    g_trig_calls = 0;
    for (int i = 0; i < 10; i++) {
        eli_draw_list_add_rect_filled(&dl, a, b, ELI_COL32_WHITE, 5.0f, ELI_DRAW_NONE);
        eli_draw_list_add_rect(&dl, a, b, ELI_COL32_WHITE, 5.0f, ELI_DRAW_NONE, 1.0f);
    }
    ELI_ASSERT_EQ(g_trig_calls, 0);

    /* The counter is live: an explicit circle still uses direct trig. */
    eli_draw_list_add_circle_filled(&dl, a, 5.0f, ELI_COL32_WHITE, 8);
    ELI_ASSERT_GT(g_trig_calls, 0);
    eli_draw_list_clear(&dl);
}

ELI_TEST_MAIN()
