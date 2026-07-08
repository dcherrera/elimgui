/**
 * @file test_font_text.c
 * @brief Unit tests for text measurement, glyph-quad emission into a draw list,
 *        and the context font stack.
 *
 * @status Phase 3 text + font stack coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/font/eli_font.h>

/* Build a shared default atlas once per test that needs it. */
static eli_font *test_build_default(eli_font_atlas **out_atlas)
{
    eli_font_atlas *atlas = eli_font_atlas_create();
    eli_font *f = eli_font_atlas_add_font_default(atlas, NULL);
    eli_font_atlas_build(atlas);
    *out_atlas = atlas;
    return f;
}

ELI_TEST(calc_text_size_sums_advances_and_line_height) {
    eli_font_atlas *atlas;
    eli_font *f = test_build_default(&atlas);

    eli_vec2 sz = eli_font_calc_text_size(f, f->font_size, "Hi", NULL);
    float expect_w = eli_font_find_glyph(f, 'H')->advance_x +
                     eli_font_find_glyph(f, 'i')->advance_x;
    ELI_ASSERT_FLT_NEAR(sz.x, expect_w, 1e-4);   /* 7 + 7 = 14 (monospace) */
    ELI_ASSERT_FLT_NEAR(sz.x, 14.0f, 1e-4);
    ELI_ASSERT_FLT_NEAR(sz.y, f->line_height, 1e-4);
    eli_font_atlas_destroy(atlas);
}

ELI_TEST(calc_text_size_multiline_height_and_width) {
    eli_font_atlas *atlas;
    eli_font *f = test_build_default(&atlas);

    /* Two lines: width is the widest line, height is two line-heights. */
    eli_vec2 sz = eli_font_calc_text_size(f, f->font_size, "AB\nCDE", NULL);
    ELI_ASSERT_FLT_NEAR(sz.x, 21.0f, 1e-4);              /* "CDE" = 3 * 7 */
    ELI_ASSERT_FLT_NEAR(sz.y, 2.0f * f->line_height, 1e-4);
    eli_font_atlas_destroy(atlas);
}

ELI_TEST(add_text_emits_quad_per_visible_glyph) {
    eli_font_atlas *atlas;
    eli_font *f = test_build_default(&atlas);

    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);

    /* "AB" -> 2 visible glyphs -> 2 quads (4 vtx, 6 idx each). */
    eli_draw_list_add_text_ex(&dl, f, f->font_size, eli_make_vec2(0, 0), ELI_COL32_WHITE,
                              "AB", NULL, 0.0f, NULL);
    ELI_ASSERT_EQ(dl.vtx_count, 8u);
    ELI_ASSERT_EQ(dl.idx_count, 12u);

    /* First glyph's top-left vertex sits at pen + glyph offset. */
    const eli_font_glyph *A = eli_font_find_glyph(f, 'A');
    ELI_ASSERT_FLT_NEAR(dl.vtx[0].x, A->x0, 1e-4);
    ELI_ASSERT_FLT_NEAR(dl.vtx[0].y, A->y0, 1e-4);
    ELI_ASSERT_EQ(dl.vtx[0].col, ELI_COL32_WHITE);

    eli_draw_list_clear(&dl);
    eli_font_atlas_destroy(atlas);
}

ELI_TEST(add_text_skips_whitespace_geometry) {
    eli_font_atlas *atlas;
    eli_font *f = test_build_default(&atlas);

    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);

    /* "A B" has 2 visible glyphs; the space advances but emits nothing. */
    eli_draw_list_add_text_ex(&dl, f, f->font_size, eli_make_vec2(0, 0), ELI_COL32_WHITE,
                              "A B", NULL, 0.0f, NULL);
    ELI_ASSERT_EQ(dl.vtx_count, 8u);
    ELI_ASSERT_EQ(dl.idx_count, 12u);
    eli_draw_list_clear(&dl);
    eli_font_atlas_destroy(atlas);
}

ELI_TEST(add_text_transparent_is_noop) {
    eli_font_atlas *atlas;
    eli_font *f = test_build_default(&atlas);

    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);
    eli_draw_list_add_text_ex(&dl, f, f->font_size, eli_make_vec2(0, 0), 0x00FFFFFFu /* a=0 */,
                              "AB", NULL, 0.0f, NULL);
    ELI_ASSERT_EQ(dl.vtx_count, 0u);
    eli_draw_list_clear(&dl);
    eli_font_atlas_destroy(atlas);
}

ELI_TEST(font_stack_push_pop_depth_and_current) {
    eli_context *ctx = eli_create_context();
    eli_font_atlas *atlas;
    eli_font *f = test_build_default(&atlas);

    ELI_ASSERT_NULL(eli_get_font());
    ELI_ASSERT_EQ(ctx->font_stack_size, 0);

    eli_push_font(f);
    ELI_ASSERT_EQ(ctx->font_stack_size, 1);
    ELI_ASSERT_EQ(eli_get_font(), f);
    ELI_ASSERT_FLT_NEAR(eli_get_font_size(), 13.0f, 1e-4);

    /* White-pixel UV is sourced from the current font's atlas. */
    eli_vec2 wp = eli_get_font_tex_uv_white_pixel();
    ELI_ASSERT_FLT_NEAR(wp.x, atlas->tex_uv_white_pixel.x, 1e-6);
    ELI_ASSERT_FLT_NEAR(wp.y, atlas->tex_uv_white_pixel.y, 1e-6);

    eli_pop_font();
    ELI_ASSERT_EQ(ctx->font_stack_size, 0);
    ELI_ASSERT_NULL(eli_get_font());

    eli_font_atlas_destroy(atlas);
    eli_destroy_context(ctx);
}

ELI_TEST(font_global_scale_affects_font_size) {
    eli_context *ctx = eli_create_context();
    eli_font_atlas *atlas;
    eli_font *f = test_build_default(&atlas);

    ctx->io.font_global_scale = 2.0f;
    eli_push_font(f);
    ELI_ASSERT_FLT_NEAR(eli_get_font_size(), 26.0f, 1e-4); /* 13 * 2 */
    eli_pop_font();

    eli_font_atlas_destroy(atlas);
    eli_destroy_context(ctx);
}

ELI_TEST_MAIN()
