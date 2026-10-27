/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file test_font_density.c
 * @brief Font rasterizer density (D7): metrics in points are unchanged at density
 *        2, the atlas holds twice the texels per glyph, and CPU pixels can be
 *        released without unbuilding. Also eli_style_scale_all_sizes.
 *
 * @status In use.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/elimgui.h>

static eli_font_atlas *build_atlas(float density)
{
    eli_font_atlas *atlas = eli_font_atlas_create();
    eli_font_config cfg = eli_font_config_default();
    cfg.rasterizer_density = density;
    eli_font_atlas_add_font_default(atlas, &cfg);
    eli_font_atlas_build(atlas);
    return atlas;
}

ELI_TEST(density_2_keeps_metrics_in_points) {
    eli_font_atlas *a1 = build_atlas(1.0f);
    eli_font_atlas *a2 = build_atlas(2.0f);
    const eli_font *f1 = a1->fonts[0], *f2 = a2->fonts[0];
    ELI_ASSERT_FLT_NEAR(f2->font_size, f1->font_size, 1e-4);
    ELI_ASSERT_FLT_NEAR(f2->ascent, f1->ascent, 1e-4);
    ELI_ASSERT_FLT_NEAR(f2->descent, f1->descent, 1e-4);
    ELI_ASSERT_FLT_NEAR(f2->line_height, f1->line_height, 1e-4);
    int advance_mismatch = 0, box_far = 0;
    for (uint32_t cp = 32; cp < 127; cp++) {
        const eli_font_glyph *g1 = eli_font_find_glyph(f1, cp);
        const eli_font_glyph *g2 = eli_font_find_glyph(f2, cp);
        if (!g1 || !g2 || fabsf(g1->advance_x - g2->advance_x) > 1e-4f)
            advance_mismatch++;
        else if (g1->visible && (fabsf(g1->x1 - g2->x1) > 1.0f || fabsf(g1->y1 - g2->y1) > 1.0f))
            box_far++;                         /* quads within a point of each other */
    }
    ELI_ASSERT_EQ(advance_mismatch, 0);
    ELI_ASSERT_EQ(box_far, 0);
    float w1 = eli_font_calc_text_size(f1, f1->font_size, "Hello, world", NULL).x;
    float w2 = eli_font_calc_text_size(f2, f2->font_size, "Hello, world", NULL).x;
    ELI_ASSERT_FLT_NEAR(w1, w2, 1e-4);
    eli_font_atlas_destroy(a1);
    eli_font_atlas_destroy(a2);
}

ELI_TEST(density_2_doubles_glyph_texels) {
    eli_font_atlas *a1 = build_atlas(1.0f);
    eli_font_atlas *a2 = build_atlas(2.0f);
    const eli_font_glyph *g1 = eli_font_find_glyph(a1->fonts[0], 'M');
    const eli_font_glyph *g2 = eli_font_find_glyph(a2->fonts[0], 'M');
    float t1 = (g1->u1 - g1->u0) * (float)a1->tex_width;
    float t2 = (g2->u1 - g2->u0) * (float)a2->tex_width;
    ELI_ASSERT_TRUE(t2 >= 2.0f * t1 - 1.0f && t2 <= 2.0f * t1 + 1.0f);
    ELI_ASSERT_GE((long)a2->tex_width * a2->tex_height, 3L * a1->tex_width * a1->tex_height);
    eli_font_atlas_destroy(a1);
    eli_font_atlas_destroy(a2);
}

ELI_TEST(release_cpu_pixels_keeps_atlas_built) {
    eli_font_atlas *a = build_atlas(2.0f);
    unsigned char *px = NULL;
    int w = 0, h = 0, bpp = 0;
    eli_font_atlas_get_tex_data_as_rgba32(a, &px, &w, &h, &bpp);
    ELI_ASSERT_NOT_NULL(px);
    eli_font_atlas_release_cpu_pixels(a);
    ELI_ASSERT_TRUE(eli_font_atlas_is_built(a));
    ELI_ASSERT_NULL(a->tex_pixels_alpha8);
    ELI_ASSERT_NULL(a->tex_pixels_rgba32);
    ELI_ASSERT_NOT_NULL(eli_font_find_glyph(a->fonts[0], 'A'));
    eli_font_atlas_destroy(a);
}

ELI_TEST(style_scale_all_sizes_scales_sizes_not_alignment) {
    eli_context *ctx = eli_create_context();
    eli_style s = ctx->style;
    eli_style_scale_all_sizes(&s, 2.0f);
    ELI_ASSERT_FLT_NEAR(s.window_padding.x, ctx->style.window_padding.x * 2.0f, 1e-5);
    ELI_ASSERT_FLT_NEAR(s.scrollbar_size, ctx->style.scrollbar_size * 2.0f, 1e-5);
    ELI_ASSERT_FLT_NEAR(s.frame_rounding, ctx->style.frame_rounding * 2.0f, 1e-5);
    ELI_ASSERT_FLT_NEAR(s.button_text_align.x, ctx->style.button_text_align.x, 1e-5);
    ELI_ASSERT_FLT_NEAR(s.window_border_size, ctx->style.window_border_size, 1e-5);
    eli_destroy_context(ctx);
}

ELI_TEST_MAIN()
