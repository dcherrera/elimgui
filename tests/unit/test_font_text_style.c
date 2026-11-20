/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file test_font_text_style.c
 * @brief Faux bold text and single-line cuts with an ellipsis.
 *
 * Covers the bold double pass (two runs 1 px apart) and its width, the cut
 * landing on codepoint boundaries for every width (including a text end that
 * splits a UTF-8 sequence), the ellipsis draw (fits whole, cut, ellipsis alone),
 * and zero allocations once the draw list has grown. Uses the embedded default
 * font (monospace, 7 px advance).
 *
 * @status In use.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/font/eli_font.h>
#include <eli/util/eli_mem.h>

static eli_font *test_build_default(eli_font_atlas **out_atlas)
{
    eli_font_atlas *atlas = eli_font_atlas_create();
    eli_font *f = eli_font_atlas_add_font_default(atlas, NULL);
    eli_font_atlas_build(atlas);
    *out_atlas = atlas;
    return f;
}

static bool test_is_continuation(const char *p)
{
    return ((unsigned char)*p & 0xC0) == 0x80;
}

/* ---------------------------------------------------------------------------
 * Faux bold
 * ------------------------------------------------------------------------- */

ELI_TEST(bold_draws_two_runs_one_px_apart)
{
    eli_font_atlas *atlas;
    eli_font *f = test_build_default(&atlas);
    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);

    eli_draw_list_add_text_bold(&dl, f, f->font_size, eli_make_vec2(5, 3), ELI_COL32_WHITE, "AB",
                                NULL);
    ELI_ASSERT_EQ(dl.vtx_count, 16u);   /* 2 glyphs x 2 passes x 4 vtx */
    ELI_ASSERT_EQ(dl.idx_count, 24u);
    for (int v = 0; v < 8; v++) {
        ELI_ASSERT_FLT_NEAR(dl.vtx[v + 8].x, dl.vtx[v].x + ELI_TEXT_BOLD_OFFSET, 1e-4);
        ELI_ASSERT_FLT_NEAR(dl.vtx[v + 8].y, dl.vtx[v].y, 1e-4);
        ELI_ASSERT_EQ(dl.vtx[v + 8].col, ELI_COL32_WHITE);
    }
    eli_draw_list_clear(&dl);
    eli_font_atlas_destroy(atlas);
}

ELI_TEST(bold_width_is_width_plus_offset)
{
    eli_font_atlas *atlas;
    eli_font *f = test_build_default(&atlas);

    ELI_ASSERT_FLT_NEAR(eli_font_calc_text_width_bold(f, f->font_size, "Hi", NULL), 15.0f, 1e-4);
    const char *t = "Hello";
    ELI_ASSERT_FLT_NEAR(eli_font_calc_text_width_bold(f, f->font_size, t, t + 3), 22.0f, 1e-4);
    ELI_ASSERT_FLT_NEAR(eli_font_calc_text_width_bold(f, 2.0f * f->font_size, "Hi", NULL), 29.0f,
                        1e-4);   /* the offset does not scale */
    ELI_ASSERT_FLT_NEAR(eli_font_calc_text_width_bold(f, f->font_size, "", NULL), 0.0f, 1e-4);
    ELI_ASSERT_FLT_NEAR(eli_font_calc_text_width_bold(f, f->font_size, NULL, NULL), 0.0f, 1e-4);
    eli_font_atlas_destroy(atlas);
}

/* ---------------------------------------------------------------------------
 * Cut
 * ------------------------------------------------------------------------- */

ELI_TEST(cut_ascii_prefix_fits)
{
    eli_font_atlas *atlas;
    eli_font *f = test_build_default(&atlas);
    const char *t = "Hello";
    const char *end;

    ELI_ASSERT_FLT_NEAR(eli_text_cut_to_width(f, f->font_size, t, NULL, 20.0f, &end), 14.0f,
                        1e-4);
    ELI_ASSERT_TRUE(end == t + 2);
    ELI_ASSERT_FLT_NEAR(eli_text_cut_to_width(f, f->font_size, t, NULL, 21.0f, &end), 21.0f,
                        1e-4);   /* exactly fits */
    ELI_ASSERT_TRUE(end == t + 3);
    eli_text_cut_to_width(f, f->font_size, t, NULL, 100.0f, &end);
    ELI_ASSERT_TRUE(end == t + 5);
    ELI_ASSERT_FLT_NEAR(eli_text_cut_to_width(f, f->font_size, t, NULL, 3.0f, &end), 0.0f, 1e-4);
    ELI_ASSERT_TRUE(end == t);
    eli_font_atlas_destroy(atlas);
}

ELI_TEST(cut_stops_at_newline)
{
    eli_font_atlas *atlas;
    eli_font *f = test_build_default(&atlas);
    const char *t = "ab\ncd";
    const char *end;
    ELI_ASSERT_FLT_NEAR(eli_text_cut_to_width(f, f->font_size, t, NULL, 100.0f, &end), 14.0f,
                        1e-4);
    ELI_ASSERT_TRUE(end == t + 2);
    eli_font_atlas_destroy(atlas);
}

ELI_TEST(cut_lands_on_codepoint_boundaries)
{
    eli_font_atlas *atlas;
    eli_font *f = test_build_default(&atlas);
    /* a, é (2 bytes), € (3 bytes), 😀 (4 bytes), b */
    const char *t = "a\xC3\xA9\xE2\x82\xAC\xF0\x9F\x98\x80" "b\xC3\xA9";
    const char *text_end = t + strlen(t);
    const char *prev = t;
    for (float w = 0.0f; w <= 80.0f; w += 0.5f) {
        const char *end;
        float got = eli_text_cut_to_width(f, f->font_size, t, NULL, w, &end);
        ELI_ASSERT_TRUE(end >= t && end <= text_end);
        ELI_ASSERT_TRUE(end == text_end || !test_is_continuation(end));
        ELI_ASSERT_TRUE(end >= prev);   /* wider never cuts shorter */
        ELI_ASSERT_LE(got, w);
        ELI_ASSERT_FLT_NEAR(got, eli_font_calc_text_size(f, f->font_size, t, end).x, 1e-4);
        prev = end;
    }
    ELI_ASSERT_TRUE(prev == text_end);
    eli_font_atlas_destroy(atlas);
}

ELI_TEST(cut_never_reads_past_a_split_sequence)
{
    eli_font_atlas *atlas;
    eli_font *f = test_build_default(&atlas);
    /* The text end falls inside € (after its first two bytes): the cut stays inside
     * [t, t + 3] and the split bytes count as malformed, not as a whole €. */
    const char *t = "a\xE2\x82\xAC";
    const char *end;
    eli_text_cut_to_width(f, f->font_size, t, t + 3, 1000.0f, &end);
    ELI_ASSERT_TRUE(end == t + 3);
    uint32_t cp;
    ELI_ASSERT_EQ(eli_text_utf8_decode(t + 1, t + 3, &cp), 1);
    ELI_ASSERT_EQ(cp, 0xFFFDu);
    ELI_ASSERT_EQ(eli_text_utf8_decode(t + 1, t + 4, &cp), 3);
    ELI_ASSERT_EQ(cp, 0x20ACu);
    eli_font_atlas_destroy(atlas);
}

/* ---------------------------------------------------------------------------
 * Ellipsis
 * ------------------------------------------------------------------------- */

ELI_TEST(ellipsis_whole_text_when_it_fits)
{
    eli_font_atlas *atlas;
    eli_font *f = test_build_default(&atlas);
    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);
    float w = eli_draw_list_add_text_ellipsis(&dl, f, f->font_size, eli_make_vec2(0, 0),
                                              ELI_COL32_WHITE, "Hi", NULL, 100.0f);
    ELI_ASSERT_FLT_NEAR(w, 14.0f, 1e-4);
    ELI_ASSERT_EQ(dl.vtx_count, 8u);
    eli_draw_list_clear(&dl);
    eli_font_atlas_destroy(atlas);
}

ELI_TEST(ellipsis_cut_prefix_then_ellipsis)
{
    eli_font_atlas *atlas;
    eli_font *f = test_build_default(&atlas);
    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);

    const char *ell = eli_font__ellipsis(f);
    float ell_w = eli_font_calc_text_size(f, f->font_size, ell, NULL).x;
    eli_draw_list ref;
    eli_draw_list_init(&ref, ELI_DRAW_LIST_NONE);
    eli_draw_list_add_text_ex(&ref, f, f->font_size, eli_make_vec2(0, 0), ELI_COL32_WHITE, ell,
                              NULL, 0.0f, NULL);
    uint32_t ell_vtx = ref.vtx_count;

    /* Room for two glyphs plus the ellipsis. */
    float max_w = 14.0f + ell_w + 3.0f;
    float w = eli_draw_list_add_text_ellipsis(&dl, f, f->font_size, eli_make_vec2(0, 0),
                                              ELI_COL32_WHITE, "Hello world", NULL, max_w);
    ELI_ASSERT_FLT_NEAR(w, 14.0f + ell_w, 1e-4);
    ELI_ASSERT_LE(w, max_w);
    ELI_ASSERT_EQ(dl.vtx_count, 8u + ell_vtx);
    /* The ellipsis starts right after the prefix. */
    ELI_ASSERT_FLT_NEAR(dl.vtx[8].x, 14.0f + ref.vtx[0].x, 1e-4);

    /* Text past a newline counts as cut. */
    eli_draw_list_reset(&dl);
    w = eli_draw_list_add_text_ellipsis(&dl, f, f->font_size, eli_make_vec2(0, 0),
                                        ELI_COL32_WHITE, "ab\ncd", NULL, 100.0f);
    ELI_ASSERT_FLT_NEAR(w, 14.0f + ell_w, 1e-4);

    /* Not even the ellipsis fits: it is drawn alone. */
    eli_draw_list_reset(&dl);
    w = eli_draw_list_add_text_ellipsis(&dl, f, f->font_size, eli_make_vec2(0, 0),
                                        ELI_COL32_WHITE, "Hello", NULL, 2.0f);
    ELI_ASSERT_FLT_NEAR(w, ell_w, 1e-4);
    ELI_ASSERT_EQ(dl.vtx_count, ell_vtx);

    eli_draw_list_clear(&ref);
    eli_draw_list_clear(&dl);
    eli_font_atlas_destroy(atlas);
}

ELI_TEST(ellipsis_multibyte_cut_on_boundary)
{
    eli_font_atlas *atlas;
    eli_font *f = test_build_default(&atlas);
    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);
    const char *t = "\xC3\xA9\xC3\xA9\xC3\xA9\xC3\xA9\xC3\xA9";   /* éééé é */
    float ell_w = eli_font_calc_text_size(f, f->font_size, eli_font__ellipsis(f), NULL).x;
    for (float max_w = ell_w; max_w < 35.0f; max_w += 1.0f) {
        eli_draw_list_reset(&dl);
        float w = eli_draw_list_add_text_ellipsis(&dl, f, f->font_size, eli_make_vec2(0, 0),
                                                  ELI_COL32_WHITE, t, NULL, max_w);
        ELI_ASSERT_LE(w, max_w);
        float prefix = w - ell_w;
        ELI_ASSERT_FLT_NEAR(prefix, 7.0f * floorf(prefix / 7.0f + 0.5f), 1e-4);
    }
    eli_draw_list_clear(&dl);
    eli_font_atlas_destroy(atlas);
}

/* ---------------------------------------------------------------------------
 * Allocations
 * ------------------------------------------------------------------------- */

static int g_text_allocs;

static void *test_text_alloc(size_t sz, void *user_data)
{
    (void)user_data;
    g_text_allocs++;
    return malloc(sz);
}

static void test_text_free(void *ptr, void *user_data)
{
    (void)user_data;
    free(ptr);
}

static void test_text_frame(eli_draw_list *dl, const eli_font *f)
{
    eli_draw_list_reset(dl);
    for (int i = 0; i < 30; i++) {
        float y = 16.0f * (float)i;
        eli_draw_list_add_text_bold(dl, f, f->font_size, eli_make_vec2(0, y), ELI_COL32_WHITE,
                                    "Project name", NULL);
        eli_draw_list_add_text_ellipsis(dl, f, f->font_size, eli_make_vec2(100, y),
                                        ELI_COL32_WHITE,
                                        "A long card title that will not fit \xC3\xA9", NULL,
                                        120.0f);
        const char *end;
        eli_text_cut_to_width(f, f->font_size, "caf\xC3\xA9 ol\xC3\xA9", NULL, 30.0f, &end);
        (void)eli_font_calc_text_width_bold(f, f->font_size, "Hand", NULL);
    }
}

ELI_TEST(text_style_no_allocations_after_draw_list_growth)
{
    eli_font_atlas *atlas;
    eli_font *f = test_build_default(&atlas);
    eli_set_allocator_functions(test_text_alloc, test_text_free, NULL);
    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);

    test_text_frame(&dl, f);   /* warm-up: the draw list grows to the frame's size */
    g_text_allocs = 0;
    for (int frame = 0; frame < 3; frame++)
        test_text_frame(&dl, f);
    ELI_ASSERT_EQ(g_text_allocs, 0);

    eli_draw_list_clear(&dl);
    eli_set_allocator_functions(NULL, NULL, NULL);
    eli_font_atlas_destroy(atlas);
}

ELI_TEST_MAIN()
