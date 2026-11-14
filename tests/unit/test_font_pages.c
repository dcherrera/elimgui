/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file test_font_pages.c
 * @brief Unit tests for on-demand glyphs and atlas pages: lazy rasterization
 *        (identical to the baked glyph), missing-codepoint caching, backend hooks
 *        and dirty uploads, LRU recycling, the soft cap, idle freeing, the
 *        two-level lookup, and text switching textures between pages.
 *
 * @status On-demand page coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/font/eli_font.h>

/* ---- mock backend ------------------------------------------------------------ */

typedef struct MockBackend {
    int creates, updates, destroys;
    int last_x, last_y, last_w, last_h;
    eli_texture_id next_id;
} MockBackend;

static eli_texture_id mock_create(void *user, int w, int h, const unsigned char *a8)
{
    MockBackend *m = (MockBackend *)user;
    (void)w;
    (void)h;
    (void)a8;
    m->creates++;
    return ++m->next_id + 100;
}

static void mock_update(void *user, eli_texture_id id, int x, int y, int w, int h,
                        const unsigned char *a8, int pitch)
{
    MockBackend *m = (MockBackend *)user;
    (void)id;
    (void)a8;
    (void)pitch;
    m->updates++;
    m->last_x = x;
    m->last_y = y;
    m->last_w = w;
    m->last_h = h;
}

static void mock_destroy(void *user, eli_texture_id id)
{
    (void)id;
    ((MockBackend *)user)->destroys++;
}

static void mock_install(eli_font_atlas *atlas, MockBackend *m)
{
    memset(m, 0, sizeof(*m));
    eli_font_atlas_backend b = { m, mock_create, mock_update, mock_destroy };
    eli_font_atlas_set_backend(atlas, &b);
}

/** An on-demand atlas with ProggyClean at `px` points. */
static eli_font_atlas *on_demand_atlas(float px, int page_size, int max_pages, eli_font **out)
{
    eli_font_atlas *atlas = eli_font_atlas_create();
    atlas->flags |= ELI_FONT_ATLAS_ON_DEMAND;
    atlas->page_size = page_size;
    atlas->max_pages = max_pages;
    eli_font_config cfg = eli_font_config_default();
    cfg.size_pixels = px;
    *out = eli_font_atlas_add_font_default(atlas, &cfg);
    return atlas;
}

/* ---- build and lazy load ----------------------------------------------------- */

ELI_TEST(on_demand_build_makes_page0_with_white_block_and_no_glyphs) {
    eli_font *f;
    eli_font_atlas *atlas = on_demand_atlas(13.0f, 0, 0, &f);
    ELI_ASSERT_TRUE(eli_font_atlas_build(atlas));
    ELI_ASSERT_EQ(atlas->pages_count, 1);
    ELI_ASSERT_EQ(atlas->tex_width, ELI_FONT_ATLAS_DEFAULT_PAGE_SIZE);
    ELI_ASSERT_EQ(f->glyph_count, 0);
    for (int y = 0; y < ELI_FONT_ATLAS_WHITE_SIZE; y++)
        for (int x = 0; x < ELI_FONT_ATLAS_WHITE_SIZE; x++)
            ELI_ASSERT_EQ(atlas->tex_pixels_alpha8[y * atlas->tex_width + x], 0xFF);
    ELI_ASSERT_FLT_NEAR(atlas->tex_uv_white_pixel.x, 1.0f / ELI_FONT_ATLAS_DEFAULT_PAGE_SIZE, 1e-9);
    ELI_ASSERT_FLT_NEAR(f->ascent, 10.0f, 1e-4); /* metrics are known without glyphs */
    eli_font_atlas_destroy(atlas);
}

ELI_TEST(on_demand_glyph_matches_baked_glyph_and_texels) {
    eli_font *lazy;
    eli_font_atlas *a = on_demand_atlas(13.0f, 0, 0, &lazy);
    ELI_ASSERT_TRUE(eli_font_atlas_build(a));
    eli_font_atlas *b = eli_font_atlas_create();
    eli_font *baked = eli_font_atlas_add_font_default(b, NULL);
    ELI_ASSERT_TRUE(eli_font_atlas_build(b));

    const char *probe = "AMgj@~";
    for (const char *p = probe; *p; p++) {
        const eli_font_glyph *gl = eli_font_find_glyph_no_fallback(lazy, (uint32_t)*p);
        const eli_font_glyph *gb = eli_font_find_glyph_no_fallback(baked, (uint32_t)*p);
        ELI_ASSERT_NOT_NULL(gl);
        ELI_ASSERT_NOT_NULL(gb);
        ELI_ASSERT_EQ(gl->advance_x, gb->advance_x);
        ELI_ASSERT_EQ(gl->x0, gb->x0);
        ELI_ASSERT_EQ(gl->y0, gb->y0);
        ELI_ASSERT_EQ(gl->x1, gb->x1);
        ELI_ASSERT_EQ(gl->y1, gb->y1);
        /* Same coverage texels at each glyph's own spot. */
        int w = (int)(gl->x1 - gl->x0), h = (int)(gl->y1 - gl->y0);
        int lx = (int)(gl->u0 * a->tex_width + 0.5f), ly = (int)(gl->v0 * a->tex_height + 0.5f);
        int bx = (int)(gb->u0 * b->tex_width + 0.5f), by = (int)(gb->v0 * b->tex_height + 0.5f);
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
                ELI_ASSERT_EQ(a->tex_pixels_alpha8[(ly + y) * a->tex_width + lx + x],
                              b->tex_pixels_alpha8[(by + y) * b->tex_width + bx + x]);
    }
    ELI_ASSERT_EQ(lazy->glyph_count, 6);
    eli_font_atlas_destroy(a);
    eli_font_atlas_destroy(b);
}

ELI_TEST(missing_codepoint_is_resolved_once) {
    eli_font *f;
    eli_font_atlas *atlas = on_demand_atlas(13.0f, 0, 0, &f);
    ELI_ASSERT_TRUE(eli_font_atlas_build(atlas));
    ELI_ASSERT_NULL(eli_font_find_glyph_no_fallback(f, 0x4E00));
    ELI_ASSERT_EQ(eli_font__lookup_get(f, 0x4E00), ELI_FONT_LOOKUP_MISSING);
    ELI_ASSERT_EQ(eli_font_find_glyph(f, 0x4E00), &f->tofu_glyph);
    ELI_ASSERT_EQ(f->glyph_count, 0);
    eli_font_atlas_destroy(atlas);
}

ELI_TEST(lookup_is_two_level_and_sparse) {
    eli_font font;
    memset(&font, 0, sizeof(font));
    ELI_ASSERT_TRUE(eli_font__lookup_set(&font, 0x1F600, 7));
    ELI_ASSERT_EQ(eli_font__lookup_get(&font, 0x1F600), 7);
    ELI_ASSERT_EQ(eli_font__lookup_get(&font, 0x1F601), ELI_FONT_LOOKUP_UNKNOWN);
    ELI_ASSERT_EQ(eli_font__lookup_get(&font, 'A'), ELI_FONT_LOOKUP_UNKNOWN);
    ELI_ASSERT_EQ(font.lookup_block_count, 0x1F600 / ELI_FONT_LOOKUP_BLOCK + 1);
    int blocks = 0;
    for (int b = 0; b < font.lookup_block_count; b++)
        blocks += font.lookup_blocks[b] != NULL;
    ELI_ASSERT_EQ(blocks, 1);
    ELI_ASSERT_FALSE(eli_font__lookup_set(&font, 0x110000, 1));
    eli_font__lookup_free(&font);
}

/* ---- backend hooks ----------------------------------------------------------- */

ELI_TEST(hooks_create_page_texture_and_upload_dirty_glyph_rect) {
    eli_font *f;
    eli_font_atlas *atlas = on_demand_atlas(13.0f, 128, 0, &f);
    MockBackend m;
    mock_install(atlas, &m);
    ELI_ASSERT_TRUE(eli_font_atlas_build(atlas));
    ELI_ASSERT_EQ(m.creates, 1);
    ELI_ASSERT_NE(atlas->tex_id, (eli_texture_id)0);
    eli_font_atlas_update(atlas);
    ELI_ASSERT_EQ(m.updates, 0); /* the create uploaded the page */

    const eli_font_glyph *g = eli_font_find_glyph(f, 'W');
    int gx = (int)(g->u0 * 128 + 0.5f), gy = (int)(g->v0 * 128 + 0.5f);
    eli_font_atlas_update(atlas);
    ELI_ASSERT_EQ(m.updates, 1);
    ELI_ASSERT_EQ(m.last_x, gx);
    ELI_ASSERT_EQ(m.last_y, gy);
    ELI_ASSERT_EQ(m.last_w, (int)(g->x1 - g->x0));
    eli_font_atlas_update(atlas);
    ELI_ASSERT_EQ(m.updates, 1); /* nothing new */
    ELI_ASSERT_EQ(atlas->frame_count, 3u);

    eli_font_atlas_destroy(atlas);
    ELI_ASSERT_EQ(m.destroys, 1);
}

/* ---- recycling, soft cap, idle ----------------------------------------------- */

/** Load glyphs until the atlas has `pages` live pages; returns the codepoint reached. */
static uint32_t fill_pages(eli_font *f, int pages, uint32_t cp)
{
    while (eli_font_atlas_live_page_count(f->container_atlas) < pages && cp < 0x7F)
        eli_font_find_glyph(f, cp++);
    return cp;
}

ELI_TEST(full_atlas_recycles_lru_page_not_used_this_frame) {
    /* 52 px glyphs on 64 px pages: two fit per page. One new glyph per frame with a
     * cap of 2 pages must recycle the LRU page instead of growing. */
    eli_font *f;
    eli_font_atlas *atlas = on_demand_atlas(52.0f, 64, 2, &f);
    MockBackend m;
    mock_install(atlas, &m);
    ELI_ASSERT_TRUE(eli_font_atlas_build(atlas));
    uint32_t first_on_page1 = 0;
    for (uint32_t cp = '#'; cp < '#' + 12; cp++) {
        eli_font_find_glyph(f, '!'); /* page 0 stays hot */
        const eli_font_glyph *g = eli_font_find_glyph(f, cp);
        ELI_ASSERT_TRUE(g->visible);
        if (g->page == 1 && !first_on_page1)
            first_on_page1 = cp;
        ELI_ASSERT_LE(eli_font_atlas_live_page_count(atlas), 2);
        eli_font_atlas_update(atlas);
    }
    ELI_ASSERT_NE(first_on_page1, 0u);
    ELI_ASSERT_EQ(m.destroys, 0); /* recycled in place, not freed */
    ELI_ASSERT_EQ(eli_font__lookup_get(f, first_on_page1), ELI_FONT_LOOKUP_UNKNOWN);
    ELI_ASSERT_GE(eli_font__lookup_get(f, '!'), 0);
    /* A dropped glyph reloads on its next use. */
    const eli_font_glyph *again = eli_font_find_glyph_no_fallback(f, first_on_page1);
    ELI_ASSERT_NOT_NULL(again);
    ELI_ASSERT_TRUE(again->visible);
    eli_font_atlas_destroy(atlas);
}

ELI_TEST(all_pages_used_this_frame_allocates_past_the_cap) {
    eli_font *f;
    eli_font_atlas *atlas = on_demand_atlas(52.0f, 64, 2, &f);
    ELI_ASSERT_TRUE(eli_font_atlas_build(atlas));
    fill_pages(f, 3, '!'); /* same frame throughout: nothing may be recycled */
    ELI_ASSERT_EQ(eli_font_atlas_live_page_count(atlas), 3);
    eli_font_atlas_destroy(atlas);
}

ELI_TEST(idle_pages_are_freed_but_page0_stays) {
    eli_font *f;
    eli_font_atlas *atlas = on_demand_atlas(52.0f, 64, 0, &f);
    atlas->page_idle_frames = 2;
    MockBackend m;
    mock_install(atlas, &m);
    ELI_ASSERT_TRUE(eli_font_atlas_build(atlas));
    uint32_t cp = fill_pages(f, 2, '!');
    uint32_t on_page1 = cp - 1;
    size_t two_pages = eli_font_atlas_texture_bytes(atlas);
    for (int frame = 0; frame < 4; frame++) {
        eli_font_find_glyph(f, '!'); /* only page 0 in use */
        eli_font_atlas_update(atlas);
    }
    ELI_ASSERT_EQ(eli_font_atlas_live_page_count(atlas), 1);
    ELI_ASSERT_EQ(eli_font_atlas_texture_bytes(atlas), two_pages / 2);
    ELI_ASSERT_EQ(m.destroys, 1);
    ELI_ASSERT_EQ(eli_font__lookup_get(f, on_page1), ELI_FONT_LOOKUP_UNKNOWN);
    ELI_ASSERT_NOT_NULL(eli_font_find_glyph_no_fallback(f, '!'));
    eli_font_atlas_destroy(atlas);
}

/* ---- text across pages ------------------------------------------------------- */

ELI_TEST(text_switches_texture_between_pages_and_restores_it) {
    eli_font *f;
    eli_font_atlas *atlas = on_demand_atlas(52.0f, 64, 0, &f);
    MockBackend m;
    mock_install(atlas, &m);
    ELI_ASSERT_TRUE(eli_font_atlas_build(atlas));
    uint32_t cp = fill_pages(f, 2, '!');
    char text[3] = { '!', (char)(cp - 1), 0 }; /* page 0 glyph, then a page 1 glyph */
    ELI_ASSERT_EQ(eli_font_find_glyph(f, (uint32_t)text[1])->page, 1);

    eli_draw_list list;
    eli_draw_list_init(&list, 0);
    eli_draw_list_reset(&list);
    eli_draw_list_push_texture_id(&list, atlas->tex_id);
    eli_draw_list_add_text_ex(&list, f, 52.0f, eli_make_vec2(0, 0), 0xFFFFFFFFu, text, NULL, 0.0f,
                              NULL);
    eli_draw_list_add_rect_filled(&list, eli_make_vec2(0, 0), eli_make_vec2(4, 4), 0xFFFFFFFFu,
                                  0.0f, 0);
    ELI_ASSERT_EQ(list.cmd_count, 3u);
    ELI_ASSERT_EQ(list.cmds[0].texture_id, atlas->pages[0].tex_id);
    ELI_ASSERT_EQ(list.cmds[0].elem_count, 6u);
    ELI_ASSERT_EQ(list.cmds[1].texture_id, atlas->pages[1].tex_id);
    ELI_ASSERT_EQ(list.cmds[1].elem_count, 6u);
    ELI_ASSERT_EQ(list.cmds[2].texture_id, atlas->pages[0].tex_id); /* restored */
    ELI_ASSERT_EQ(list.cmd_texture_id, atlas->tex_id);
    eli_draw_list_clear(&list);
    eli_font_atlas_destroy(atlas);
}

ELI_TEST(release_cpu_pixels_is_a_no_op_on_demand) {
    eli_font *f;
    eli_font_atlas *atlas = on_demand_atlas(13.0f, 0, 0, &f);
    ELI_ASSERT_TRUE(eli_font_atlas_build(atlas));
    eli_font_atlas_release_cpu_pixels(atlas);
    ELI_ASSERT_NOT_NULL(atlas->tex_pixels_alpha8);
    ELI_ASSERT_NOT_NULL(eli_font_find_glyph_no_fallback(f, 'Q'));
    eli_font_atlas_destroy(atlas);
}

ELI_TEST_MAIN()
