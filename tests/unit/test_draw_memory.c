/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file test_draw_memory.c
 * @brief Draw memory: text reserves in chunks (long clipped, multi-byte and
 *        past-16-bit runs), draw lists shrink after a spike and never in a steady
 *        state, unused windows give back their draw lists, and the shared-vertex
 *        polyline's miter joins.
 *
 * @status Memory pass coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"
#include "eli_test_ui.h"

/* ---------------------------------------------------------------------------
 * Counting allocator
 * ------------------------------------------------------------------------- */

static int g_allocs = 0;

static void *count_alloc(size_t sz, void *user)
{
    (void)user;
    g_allocs++;
    return malloc(sz);
}

static void count_free(void *p, void *user)
{
    (void)user;
    free(p);
}

/** A string of `n` copies of `unit` (caller frees). */
static char *repeat(const char *unit, int n)
{
    size_t u = strlen(unit);
    char *s = malloc(u * (size_t)n + 1);
    for (int i = 0; i < n; i++)
        memcpy(s + u * (size_t)i, unit, u);
    s[u * (size_t)n] = '\0';
    return s;
}

/* ---------------------------------------------------------------------------
 * Text reserve
 * ------------------------------------------------------------------------- */

ELI_TEST(text_clipped_long_run_reserves_one_chunk)
{
    eli_font_atlas *atlas = eli_font_atlas_create();
    eli_font *f = eli_font_atlas_add_font_default(atlas, NULL);
    eli_font_atlas_build(atlas);
    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);

    /* 100 000 bytes on one line, only the first ~20 glyphs inside the clip: a quad
     * per byte up front would grow the list to 400 000 vertices (and refuse past
     * 65 536 with 16-bit indices, drawing nothing). */
    char *text = repeat("abcdefghij", 10000);
    eli_vec4 clip = eli_make_vec4(0.0f, 0.0f, 100.0f, 50.0f);
    eli_draw_list_add_text_ex(&dl, f, f->font_size, eli_make_vec2(0, 0), ELI_COL32_WHITE, text,
                              NULL, 0.0f, &clip);
    ELI_ASSERT_TRUE(dl.vtx_count > 0u);
    ELI_ASSERT_TRUE(dl.vtx_count < 200u);
    ELI_ASSERT_TRUE(dl.vtx_capacity <= (uint32_t)(ELI_FONT_TEXT_RESERVE_CHUNK * 4 * 2));
    ELI_ASSERT_EQ(dl.cmds[dl.cmd_count - 1].elem_count, dl.idx_count);
    free(text);
    eli_draw_list_clear(&dl);
    eli_font_atlas_destroy(atlas);
}

ELI_TEST(text_multibyte_reserves_per_glyph_not_per_byte)
{
    eli_font_atlas *atlas = eli_font_atlas_create();
    eli_font *f = eli_font_atlas_add_font_default(atlas, NULL);
    eli_font_atlas_build(atlas);
    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);

    /* 4000 glyphs of 2 bytes each, all visible: room for ~4000 quads, not 8000. */
    char *text = repeat("\xC3\xA9", 4000);   /* U+00E9 */
    eli_draw_list_add_text_ex(&dl, f, f->font_size, eli_make_vec2(0, 0), ELI_COL32_WHITE, text,
                              NULL, 0.0f, NULL);
    ELI_ASSERT_EQ(dl.vtx_count, 16000u);
    ELI_ASSERT_EQ(dl.idx_count, 24000u);
    ELI_ASSERT_TRUE(dl.vtx_capacity < 32000u);   /* a per-byte reserve asked for 32 000 */
    free(text);
    eli_draw_list_clear(&dl);
    eli_font_atlas_destroy(atlas);
}

ELI_TEST(text_reserve_matches_old_geometry)
{
    eli_font_atlas *atlas = eli_font_atlas_create();
    eli_font *f = eli_font_atlas_add_font_default(atlas, NULL);
    eli_font_atlas_build(atlas);
    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);

    /* Wrapped text crossing several chunks: one quad per visible glyph. */
    char *text = repeat("word ", 300);   /* 1200 visible glyphs, 300 spaces */
    eli_draw_list_add_text_ex(&dl, f, f->font_size, eli_make_vec2(0, 0), ELI_COL32_WHITE, text,
                              NULL, 200.0f, NULL);
    ELI_ASSERT_EQ(dl.vtx_count, 1200u * 4u);
    ELI_ASSERT_EQ(dl.idx_count, 1200u * 6u);
    ELI_ASSERT_EQ(dl.cmds[dl.cmd_count - 1].elem_count, dl.idx_count);
    free(text);
    eli_draw_list_clear(&dl);
    eli_font_atlas_destroy(atlas);
}

/* ---------------------------------------------------------------------------
 * Draw-list shrink
 * ------------------------------------------------------------------------- */

static void draw_rects(eli_draw_list *dl, int n)
{
    for (int i = 0; i < n; i++)
        eli_draw_list_add_rect_filled(dl, eli_make_vec2((float)i, 0), eli_make_vec2((float)i + 1, 1),
                                      ELI_COL32_WHITE, 0.0f, 0);
}

ELI_TEST(draw_list_shrinks_after_a_spike)
{
    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);
    draw_rects(&dl, 10000);   /* the spike: 40 000 vertices */
    uint32_t spike_cap = dl.vtx_capacity;
    ELI_ASSERT_TRUE(spike_cap >= 40000u);
    /* The window holding the spike passes, then a full window of 100-rect frames. */
    for (int f = 0; f < 2 * ELI_DRAW_LIST_SHRINK_FRAMES; f++) {
        eli_draw_list_reset(&dl);
        draw_rects(&dl, 100);
    }
    ELI_ASSERT_TRUE(dl.vtx_capacity <= 800u);
    ELI_ASSERT_TRUE(dl.idx_capacity <= 1200u);
    ELI_ASSERT_EQ(dl.vtx_count, 400u);
    eli_draw_list_clear(&dl);
}

ELI_TEST(steady_draw_list_never_reallocates)
{
    eli_set_allocator_functions(count_alloc, count_free, NULL);
    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);
    for (int f = 0; f < 10; f++) {   /* warm-up */
        eli_draw_list_reset(&dl);
        draw_rects(&dl, 5000);
    }
    g_allocs = 0;
    for (int f = 0; f < 3 * ELI_DRAW_LIST_SHRINK_FRAMES; f++) {
        eli_draw_list_reset(&dl);
        draw_rects(&dl, 5000);
    }
    ELI_ASSERT_EQ(g_allocs, 0);
    eli_draw_list_clear(&dl);
    eli_set_allocator_functions(NULL, NULL, NULL);
}

/* ---------------------------------------------------------------------------
 * Window compaction
 * ------------------------------------------------------------------------- */

static bool g_show_big = true;

static void ui_two_windows(void)
{
    eli_begin("keep", NULL, 0);
    eli_text("small");
    eli_end();
    if (g_show_big) {
        eli_begin("big", NULL, 0);
        eli_draw_list *dl = eli_get_window_draw_list();
        for (int i = 0; i < 4000; i++)
            eli_draw_list_add_rect_filled(dl, eli_make_vec2(10, 10), eli_make_vec2(20, 20),
                                          ELI_COL32_WHITE, 0.0f, 0);
        eli_end();
    }
}

ELI_TEST(unused_window_gives_back_its_draw_list)
{
    eli_context *ctx = test_ui_setup();
    g_show_big = true;
    test_ui_frames(ui_two_windows, 3);
    eli_window *big = eli_find_window_by_name(ctx, "big");
    ELI_ASSERT_NOT_NULL(big);
    ELI_ASSERT_TRUE(big->draw_list.vtx_capacity >= 16000u);

    g_show_big = false;
    int after = ctx->io.config_memory_compact_frames;
    test_ui_frames(ui_two_windows, after);
    ELI_ASSERT_NOT_NULL(big->draw_list.vtx);   /* not yet: exactly `after` frames unused */
    test_ui_frames(ui_two_windows, 2);
    ELI_ASSERT_NULL(big->draw_list.vtx);
    ELI_ASSERT_NULL(big->draw_list.cmds);
    ELI_ASSERT_EQ(big->draw_list.vtx_capacity, 0u);
    eli_window *keep = eli_find_window_by_name(ctx, "keep");
    ELI_ASSERT_NOT_NULL(keep->draw_list.vtx);   /* the shown one keeps its buffers */

    /* Shown again: the list regrows and draws as before. */
    g_show_big = true;
    test_ui_frames(ui_two_windows, 2);
    ELI_ASSERT_TRUE(big->draw_list.vtx_count >= 16000u);
    ELI_ASSERT_FLT_NEAR(big->draw_list.fringe_scale, 1.0f, 1e-6);
    test_ui_teardown(ctx);
}

ELI_TEST(compaction_off_keeps_buffers)
{
    eli_context *ctx = test_ui_setup();
    ctx->io.config_memory_compact_frames = 0;
    g_show_big = true;
    test_ui_frames(ui_two_windows, 2);
    g_show_big = false;
    test_ui_frames(ui_two_windows, 700);
    ELI_ASSERT_NOT_NULL(eli_find_window_by_name(ctx, "big")->draw_list.vtx);
    test_ui_teardown(ctx);
}

/* ---------------------------------------------------------------------------
 * Shared-vertex polyline
 * ------------------------------------------------------------------------- */

ELI_TEST(polyline_corner_is_mitered)
{
    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);
    eli_vec2 pts[3] = {{0, 0}, {10, 0}, {10, 10}};
    eli_draw_list_add_polyline(&dl, pts, 3, ELI_COL32_WHITE, ELI_DRAW_NONE, 2.0f);
    ELI_ASSERT_EQ(dl.vtx_count, 6u);
    /* The corner's pair sits on the miter: the outer corner (11, -1), inner (9, 1). */
    ELI_ASSERT_FLT_NEAR(dl.vtx[2].x, 11.0f, 1e-4);
    ELI_ASSERT_FLT_NEAR(dl.vtx[2].y, -1.0f, 1e-4);
    ELI_ASSERT_FLT_NEAR(dl.vtx[3].x, 9.0f, 1e-4);
    ELI_ASSERT_FLT_NEAR(dl.vtx[3].y, 1.0f, 1e-4);
    /* The ends are square to their segment. */
    ELI_ASSERT_FLT_NEAR(dl.vtx[0].y, -1.0f, 1e-4);
    ELI_ASSERT_FLT_NEAR(dl.vtx[5].x, 9.0f, 1e-4);
    eli_draw_list_clear(&dl);
}

ELI_TEST(polyline_hairpin_miter_is_capped)
{
    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);
    eli_vec2 pts[3] = {{0, 0}, {10, 0}, {0, 0.01f}};
    eli_draw_list_add_polyline(&dl, pts, 3, ELI_COL32_WHITE, ELI_DRAW_NONE, 2.0f);
    for (uint32_t i = 0; i < dl.vtx_count; i++) {
        float dx = dl.vtx[i].x - 5.0f, dy = dl.vtx[i].y;
        ELI_ASSERT_TRUE(dx * dx + dy * dy < 120.0f * 120.0f);   /* 100x half-width at most */
    }
    eli_draw_list_clear(&dl);
}

ELI_TEST(closed_polyline_shares_every_vertex)
{
    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);
    eli_draw_list_add_rect(&dl, eli_make_vec2(0, 0), eli_make_vec2(20, 10), ELI_COL32_WHITE,
                           0.0f, 0, 1.0f);
    ELI_ASSERT_EQ(dl.vtx_count, 8u);   /* 4 corners x 2, was 4 segments x 4 */
    ELI_ASSERT_EQ(dl.idx_count, 24u);
    /* The last segment closes onto the first corner's pair. */
    ELI_ASSERT_EQ(dl.idx[18 + 1], 0);
    ELI_ASSERT_EQ(dl.idx[18 + 2], 1);
    eli_draw_list_clear(&dl);
}

ELI_TEST_MAIN()
