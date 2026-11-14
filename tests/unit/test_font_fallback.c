/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file test_font_fallback.c
 * @brief Unit tests for fallback chains and missing glyphs: merge-mode sources
 *        (only missing codepoints, baseline alignment, cell-advance fit, baked ==
 *        on-demand), the tofu box default, fallback_char, and custom glyphs.
 *
 * ProggyClean is the only embedded face, so a "fallback face" is ProggyClean at
 * another size: the primary leaves out uppercase, the merge source supplies it.
 *
 * @status Fallback-chain coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/font/eli_font.h>

/* Everything but 'A'-'Z'. */
static const uint16_t kNoUpper[] = { 0x0020, 0x0040, 0x005B, 0x00FF, 0 };

/** Primary ProggyClean 13 px without uppercase, merged with a 26 px source. */
static eli_font_atlas *merged_atlas(bool on_demand, bool match_advance, eli_font **out)
{
    eli_font_atlas *atlas = eli_font_atlas_create();
    if (on_demand)
        atlas->flags |= ELI_FONT_ATLAS_ON_DEMAND;
    eli_font_config cfg = eli_font_config_default();
    cfg.glyph_ranges = kNoUpper;
    *out = eli_font_atlas_add_font_default(atlas, &cfg);
    eli_font_config merge = eli_font_config_default();
    merge.size_pixels = 26.0f;
    merge.merge_mode = true;
    merge.merge_match_advance = match_advance;
    eli_font *into = eli_font_atlas_add_font_default(atlas, &merge);
    if (into != *out || !eli_font_atlas_build(atlas)) {
        eli_font_atlas_destroy(atlas);
        return NULL;
    }
    return atlas;
}

/** A plain baked ProggyClean font at `px`. */
static eli_font_atlas *plain_atlas(float px, eli_font **out)
{
    eli_font_atlas *atlas = eli_font_atlas_create();
    eli_font_config cfg = eli_font_config_default();
    cfg.size_pixels = px;
    *out = eli_font_atlas_add_font_default(atlas, &cfg);
    eli_font_atlas_build(atlas);
    return atlas;
}

/* ---- merge mode -------------------------------------------------------------- */

ELI_TEST(merge_joins_previous_font_and_supplies_only_missing) {
    eli_font *f;
    eli_font_atlas *atlas = merged_atlas(false, false, &f);
    ELI_ASSERT_NOT_NULL(atlas);
    ELI_ASSERT_EQ(atlas->fonts_count, 1);
    ELI_ASSERT_EQ(atlas->configs_count, 2);
    ELI_ASSERT_FLT_NEAR(eli_font_find_glyph(f, 'a')->advance_x, 7.0f, 1e-4);  /* primary */
    ELI_ASSERT_FLT_NEAR(eli_font_find_glyph(f, 'A')->advance_x, 14.0f, 1e-4); /* merged */
    /* Each codepoint baked once: the merge source skipped what the primary had. */
    int expected = 0;
    for (uint32_t cp = 0x20; cp <= 0xFF; cp++)
        expected += eli_font_find_glyph_no_fallback(f, cp) != NULL;
    ELI_ASSERT_EQ(f->glyph_count, expected);
    eli_font_atlas_destroy(atlas);
}

ELI_TEST(merged_glyph_sits_on_the_primary_baseline) {
    eli_font *f, *big;
    eli_font_atlas *atlas = merged_atlas(false, false, &f);
    eli_font_atlas *ref = plain_atlas(26.0f, &big);
    const eli_font_glyph *m = eli_font_find_glyph(f, 'A');
    const eli_font_glyph *r = eli_font_find_glyph(big, 'A');
    float dy = f->ascent - big->ascent;
    ELI_ASSERT_TRUE(dy != 0.0f);
    ELI_ASSERT_FLT_NEAR(m->y0, r->y0 + dy, 1e-4);
    ELI_ASSERT_FLT_NEAR(m->y1, r->y1 + dy, 1e-4);
    ELI_ASSERT_FLT_NEAR(m->x0, r->x0, 1e-4);
    eli_font_atlas_destroy(atlas);
    eli_font_atlas_destroy(ref);
}

ELI_TEST(merge_match_advance_fits_glyph_in_the_cell) {
    eli_font *f;
    eli_font_atlas *atlas = merged_atlas(false, true, &f);
    ELI_ASSERT_FLT_NEAR(f->cell_advance_x, 0.0f, 1e-4); /* 'M' is not in the primary */
    eli_font_atlas_destroy(atlas);

    /* Primary keeps 'M' this time, so the cell is 7. */
    static const uint16_t kNoW[] = { 0x0020, 0x0056, 0x0058, 0x00FF, 0 };
    atlas = eli_font_atlas_create();
    eli_font_config cfg = eli_font_config_default();
    cfg.glyph_ranges = kNoW;
    f = eli_font_atlas_add_font_default(atlas, &cfg);
    eli_font_config merge = eli_font_config_default();
    merge.size_pixels = 26.0f;
    merge.merge_mode = true;
    merge.merge_match_advance = true;
    eli_font_atlas_add_font_default(atlas, &merge);
    ELI_ASSERT_TRUE(eli_font_atlas_build(atlas));
    const eli_font_glyph *w = eli_font_find_glyph(f, 'W');
    ELI_ASSERT_FLT_NEAR(f->cell_advance_x, 7.0f, 1e-4);
    ELI_ASSERT_FLT_NEAR(w->advance_x, 7.0f, 1e-4);
    ELI_ASSERT_LE(w->x1 - w->x0, 7.0f + 1e-4);                   /* shrunk to the cell */
    ELI_ASSERT_FLT_NEAR(w->x0, (7.0f - (w->x1 - w->x0)) * 0.5f, 1e-4); /* centred */
    eli_font_atlas_destroy(atlas);
}

ELI_TEST(merge_baked_and_on_demand_give_identical_glyphs) {
    eli_font *a, *b;
    eli_font_atlas *baked = merged_atlas(false, true, &a);
    eli_font_atlas *lazy = merged_atlas(true, true, &b);
    for (uint32_t cp = 0x20; cp <= 0xFF; cp++) {
        const eli_font_glyph *ga = eli_font_find_glyph_no_fallback(a, cp);
        const eli_font_glyph *gb = eli_font_find_glyph_no_fallback(b, cp);
        ELI_ASSERT_EQ(ga != NULL, gb != NULL);
        if (!ga)
            continue;
        ELI_ASSERT_EQ(ga->advance_x, gb->advance_x);
        ELI_ASSERT_EQ(ga->x0, gb->x0);
        ELI_ASSERT_EQ(ga->y0, gb->y0);
        ELI_ASSERT_EQ(ga->x1, gb->x1);
        ELI_ASSERT_EQ(ga->y1, gb->y1);
    }
    eli_font_atlas_destroy(baked);
    eli_font_atlas_destroy(lazy);
}

ELI_TEST(merge_without_a_font_is_refused) {
    eli_font_atlas *atlas = eli_font_atlas_create();
    eli_font_config merge = eli_font_config_default();
    merge.merge_mode = true;
    ELI_ASSERT_NULL(eli_font_atlas_add_font_default(atlas, &merge));
    eli_font_atlas_destroy(atlas);
}

/* ---- tofu and fallback_char -------------------------------------------------- */

ELI_TEST(missing_glyph_draws_a_tofu_box_not_a_question_mark) {
    eli_font *f;
    eli_font_atlas *atlas = plain_atlas(13.0f, &f);
    const eli_font_glyph *g = eli_font_find_glyph(f, 0x4E00);
    ELI_ASSERT_EQ(g->kind, ELI_FONT_GLYPH_TOFU);
    ELI_ASSERT_FLT_NEAR(g->advance_x, eli_font_find_glyph(f, '?')->advance_x, 1e-4);

    eli_draw_list list;
    eli_draw_list_init(&list, 0);
    eli_draw_list_reset(&list);
    eli_draw_list_push_clip_rect_full_screen(&list);
    eli_draw_list_add_text_ex(&list, f, 13.0f, eli_make_vec2(10, 20), 0xFFFFFFFFu,
                              "\xE4\xB8\x80", NULL, 0.0f, NULL); /* U+4E00 */
    ELI_ASSERT_GT(list.vtx_count, 4u); /* an outline, not one '?' quad */
    float minx = 1e9f, maxx = -1e9f, miny = 1e9f, maxy = -1e9f;
    for (uint32_t i = 0; i < list.vtx_count; i++) {
        minx = list.vtx[i].x < minx ? list.vtx[i].x : minx;
        maxx = list.vtx[i].x > maxx ? list.vtx[i].x : maxx;
        miny = list.vtx[i].y < miny ? list.vtx[i].y : miny;
        maxy = list.vtx[i].y > maxy ? list.vtx[i].y : maxy;
    }
    /* Inside the cell [10, 10 + advance] x [20, 20 + line height]. */
    ELI_ASSERT_GE(minx, 10.0f);
    ELI_ASSERT_LE(maxx, 10.0f + g->advance_x);
    ELI_ASSERT_GE(miny, 20.0f);
    ELI_ASSERT_LE(maxy, 20.0f + f->line_height);
    eli_draw_list_clear(&list);
    eli_font_atlas_destroy(atlas);
}

ELI_TEST(fallback_char_restores_question_mark) {
    eli_font *f;
    eli_font_atlas *atlas = plain_atlas(13.0f, &f);
    eli_font_set_fallback_char(f, '?');
    ELI_ASSERT_EQ(eli_font_find_glyph(f, 0x4E00), eli_font_find_glyph_no_fallback(f, '?'));
    eli_font_atlas_destroy(atlas);
}

/* ---- custom glyphs ----------------------------------------------------------- */

typedef struct CustomLog {
    int calls;
    uint32_t cp;
    eli_vec2 a, b;
} CustomLog;

static void log_custom(eli_draw_list *list, const eli_font *font, uint32_t cp, eli_vec2 a,
                       eli_vec2 b, eli_col32 col, void *user)
{
    CustomLog *log = (CustomLog *)user;
    (void)font;
    log->calls++;
    log->cp = cp;
    log->a = a;
    log->b = b;
    eli_draw_list_add_rect_filled(list, a, b, col, 0.0f, 0);
}

ELI_TEST(custom_glyph_range_draws_through_the_callback_in_its_cell) {
    eli_font *f;
    eli_font_atlas *atlas = plain_atlas(13.0f, &f);
    CustomLog log = { 0 };
    eli_font_set_custom_glyph_fn(f, log_custom, &log);
    ELI_ASSERT_TRUE(eli_font_add_custom_glyph_range(f, 0x2800, 0x28FF, 0.0f));
    ELI_ASSERT_TRUE(eli_font_add_custom_glyph_range(f, 'B', 'B', 9.0f)); /* wins over the face */

    eli_draw_list list;
    eli_draw_list_init(&list, 0);
    eli_draw_list_reset(&list);
    eli_draw_list_push_clip_rect_full_screen(&list);
    eli_draw_list_add_text_ex(&list, f, 13.0f, eli_make_vec2(5, 6), 0xFFFFFFFFu,
                              "a\xE2\xA0\x8B", NULL, 0.0f, NULL); /* a, U+280B */
    ELI_ASSERT_EQ(log.calls, 1);
    ELI_ASSERT_EQ(log.cp, 0x280Bu);
    ELI_ASSERT_FLT_NEAR(log.a.x, 5.0f + 7.0f, 1e-4); /* after 'a' */
    ELI_ASSERT_FLT_NEAR(log.b.x - log.a.x, f->cell_advance_x, 1e-4);
    ELI_ASSERT_FLT_NEAR(log.b.y - log.a.y, f->line_height, 1e-4);
    ELI_ASSERT_EQ(list.vtx_count, 8u); /* 'a' quad + the callback's rect */

    eli_draw_list_add_text_ex(&list, f, 13.0f, eli_make_vec2(0, 0), 0xFFFFFFFFu, "B", NULL, 0.0f,
                              NULL);
    ELI_ASSERT_EQ(log.calls, 2);
    ELI_ASSERT_EQ(log.cp, (uint32_t)'B');
    ELI_ASSERT_FLT_NEAR(eli_font_calc_text_size(f, 13.0f, "BB", NULL).x, 18.0f, 1e-4);

    ELI_ASSERT_TRUE(eli_font_atlas_build(atlas)); /* ranges survive a rebuild */
    ELI_ASSERT_EQ(eli_font_find_glyph(f, 'B')->kind, ELI_FONT_GLYPH_CUSTOM);
    eli_draw_list_clear(&list);
    eli_font_atlas_destroy(atlas);
}

ELI_TEST_MAIN()
