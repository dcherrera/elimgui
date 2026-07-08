/**
 * @file test_font_atlas.c
 * @brief Unit tests for the font atlas: default-font decompression + build,
 *        texture data, exact ProggyClean glyph metrics, and glyph-range tables.
 *
 * @status Phase 3 font atlas coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/font/eli_font.h>

ELI_TEST(proggy_decompress_length_is_ttf_size) {
    /* The embedded blob decompresses to the 41208-byte ProggyClean.ttf. */
    unsigned int len = eli_font_stb_decompress_length(eli_font_proggy_compressed_data);
    ELI_ASSERT_EQ(len, 41208u);

    unsigned char *ttf = (unsigned char *)malloc(len);
    ELI_ASSERT_NOT_NULL(ttf);
    unsigned int got = eli_font_stb_decompress(ttf, eli_font_proggy_compressed_data);
    ELI_ASSERT_EQ(got, len);
    /* TTF/OTF magic: sfnt version 0x00010000 for TrueType outlines. */
    ELI_ASSERT_EQ(ttf[0], 0u);
    ELI_ASSERT_EQ(ttf[1], 1u);
    ELI_ASSERT_EQ(ttf[2], 0u);
    ELI_ASSERT_EQ(ttf[3], 0u);
    free(ttf);
}

ELI_TEST(default_atlas_builds_and_is_built) {
    eli_font_atlas *atlas = eli_font_atlas_create();
    ELI_ASSERT_NOT_NULL(atlas);
    ELI_ASSERT_FALSE(eli_font_atlas_is_built(atlas));

    eli_font *font = eli_font_atlas_add_font_default(atlas, NULL);
    ELI_ASSERT_NOT_NULL(font);

    ELI_ASSERT_TRUE(eli_font_atlas_build(atlas));
    ELI_ASSERT_TRUE(eli_font_atlas_is_built(atlas));
    eli_font_atlas_destroy(atlas);
}

ELI_TEST(build_with_no_fonts_adds_default) {
    /* Building an empty atlas implicitly adds the default font. */
    eli_font_atlas *atlas = eli_font_atlas_create();
    ELI_ASSERT_TRUE(eli_font_atlas_build(atlas));
    ELI_ASSERT_EQ(atlas->fonts_count, 1);
    ELI_ASSERT_TRUE(eli_font_atlas_is_built(atlas));
    eli_font_atlas_destroy(atlas);
}

ELI_TEST(alpha8_tex_data_non_null_with_sane_size) {
    eli_font_atlas *atlas = eli_font_atlas_create();
    eli_font_atlas_add_font_default(atlas, NULL);

    unsigned char *pixels = NULL;
    int w = 0, h = 0, bpp = -1;
    eli_font_atlas_get_tex_data_as_alpha8(atlas, &pixels, &w, &h, &bpp);
    ELI_ASSERT_NOT_NULL(pixels);
    ELI_ASSERT_GT(w, 0);
    ELI_ASSERT_GT(h, 0);
    ELI_ASSERT_EQ(bpp, 1);
    /* Height is rounded to a power of two by default. */
    ELI_ASSERT_EQ((h & (h - 1)), 0);

    /* The reserved white texel is fully opaque. */
    int wx = (int)(atlas->tex_uv_white_pixel.x * (float)w);
    int wy = (int)(atlas->tex_uv_white_pixel.y * (float)h);
    ELI_ASSERT_EQ(pixels[(size_t)wy * w + wx], 255u);
    eli_font_atlas_destroy(atlas);
}

ELI_TEST(rgba32_tex_data_is_white_with_alpha) {
    eli_font_atlas *atlas = eli_font_atlas_create();
    eli_font_atlas_add_font_default(atlas, NULL);

    unsigned char *pixels = NULL;
    int w = 0, h = 0, bpp = -1;
    eli_font_atlas_get_tex_data_as_rgba32(atlas, &pixels, &w, &h, &bpp);
    ELI_ASSERT_NOT_NULL(pixels);
    ELI_ASSERT_EQ(bpp, 4);
    /* Every texel is white; only the alpha byte varies. */
    unsigned int *rgba = (unsigned int *)pixels;
    ELI_ASSERT_EQ(rgba[0] & 0x00FFFFFFu, 0x00FFFFFFu);
    eli_font_atlas_destroy(atlas);
}

ELI_TEST(default_font_metrics_proggyclean_13px) {
    eli_font_atlas *atlas = eli_font_atlas_create();
    eli_font *f = eli_font_atlas_add_font_default(atlas, NULL);
    eli_font_atlas_build(atlas);

    ELI_ASSERT_FLT_NEAR(f->font_size, 13.0f, 1e-4);
    ELI_ASSERT_FLT_NEAR(f->line_height, 13.0f, 1e-4);
    ELI_ASSERT_FLT_NEAR(f->ascent, 10.0f, 1e-4);
    ELI_ASSERT_FLT_NEAR(f->descent, -4.0f, 1e-4);
    /* 0x20..0xFF requested; one codepoint has no glyph in ProggyClean. */
    ELI_ASSERT_EQ(f->glyph_count, 223);
    eli_font_atlas_destroy(atlas);
}

ELI_TEST(known_glyph_advance_and_bbox) {
    eli_font_atlas *atlas = eli_font_atlas_create();
    eli_font *f = eli_font_atlas_add_font_default(atlas, NULL);
    eli_font_atlas_build(atlas);

    /* ProggyClean is monospace: every glyph advances 7px at 13px. */
    const eli_font_glyph *A = eli_font_find_glyph(f, 'A');
    ELI_ASSERT_NOT_NULL(A);
    ELI_ASSERT_TRUE(A->visible);
    ELI_ASSERT_FLT_NEAR(A->advance_x, 7.0f, 1e-4);
    ELI_ASSERT_FLT_NEAR(A->x0, 1.0f, 1e-4);
    ELI_ASSERT_FLT_NEAR(A->y0, 2.0f, 1e-4);
    ELI_ASSERT_FLT_NEAR(A->x1, 7.0f, 1e-4);
    ELI_ASSERT_FLT_NEAR(A->y1, 10.0f, 1e-4);
    /* UVs form a non-degenerate rect inside the atlas. */
    ELI_ASSERT_GT(A->u1, A->u0);
    ELI_ASSERT_GT(A->v1, A->v0);
    ELI_ASSERT_LE(A->u1, 1.0f);
    ELI_ASSERT_LE(A->v1, 1.0f);

    const eli_font_glyph *M = eli_font_find_glyph(f, 'M');
    ELI_ASSERT_FLT_NEAR(M->advance_x, 7.0f, 1e-4);

    /* Space advances but produces no geometry. */
    const eli_font_glyph *sp = eli_font_find_glyph(f, ' ');
    ELI_ASSERT_NOT_NULL(sp);
    ELI_ASSERT_FALSE(sp->visible);
    ELI_ASSERT_FLT_NEAR(sp->advance_x, 7.0f, 1e-4);
    eli_font_atlas_destroy(atlas);
}

ELI_TEST(missing_glyph_returns_fallback) {
    eli_font_atlas *atlas = eli_font_atlas_create();
    eli_font *f = eli_font_atlas_add_font_default(atlas, NULL);
    eli_font_atlas_build(atlas);

    /* A CJK codepoint is outside the default range: no direct glyph. */
    ELI_ASSERT_NULL(eli_font_find_glyph_no_fallback(f, 0x4E00));
    const eli_font_glyph *fb = eli_font_find_glyph(f, 0x4E00);
    ELI_ASSERT_NOT_NULL(fb);
    ELI_ASSERT_EQ(fb, f->fallback_glyph);
    eli_font_atlas_destroy(atlas);
}

ELI_TEST(glyph_ranges_have_expected_bounds) {
    const uint16_t *def = eli_font_atlas_get_glyph_ranges_default(NULL);
    ELI_ASSERT_EQ(def[0], 0x0020);
    ELI_ASSERT_EQ(def[1], 0x00FF);
    ELI_ASSERT_EQ(def[2], 0);
    /* 0x20..0xFF inclusive == 224 codepoints. */
    ELI_ASSERT_EQ(eli_font__count_range_glyphs(def), 224);

    const uint16_t *cyr = eli_font_atlas_get_glyph_ranges_cyrillic(NULL);
    ELI_ASSERT_EQ(cyr[0], 0x0020);
    ELI_ASSERT_EQ(cyr[2], 0x0400); /* Cyrillic block begins after Latin */

    /* Japanese covers CJK ideographs; a large range count is expected. */
    const uint16_t *jp = eli_font_atlas_get_glyph_ranges_japanese(NULL);
    ELI_ASSERT_GT(eli_font__count_range_glyphs(jp), 20000);
}

ELI_TEST_MAIN()
