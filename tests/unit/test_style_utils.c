/**
 * @file test_style_utils.c
 * @brief Unit tests for elimgui color utilities: get_color_u32 alpha scaling,
 *        packed<->float round-trips, RGB<->HSV round-trips, and color slot
 *        names.
 *
 * @status Phase 6 color-utility coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/style/eli_style.h>

ELI_TEST(get_color_u32_applies_alpha) {
    eli_context *ctx = eli_create_context();
    eli_style_colors_dark(NULL);
    ctx->style.alpha = 0.5f;

    /* Text is opaque white (1,1,1,1). With style.alpha 0.5 and alpha_mul 1.0,
     * final alpha = 0.5 -> 128, rgb stays 255. */
    eli_col32 col = eli_get_color_u32(ELI_COL_TEXT, 1.0f);
    uint32_t a = (col & ELI_COL32_A_MASK) >> ELI_COL32_A_SHIFT;
    uint32_t r = (col >> ELI_COL32_R_SHIFT) & 0xFFu;
    ELI_ASSERT_EQ(r, 255u);
    ELI_ASSERT_EQ(a, 128u);

    /* An extra alpha_mul of 0.5 halves again -> 0.25 -> 64. */
    eli_col32 col2 = eli_get_color_u32(ELI_COL_TEXT, 0.5f);
    uint32_t a2 = (col2 & ELI_COL32_A_MASK) >> ELI_COL32_A_SHIFT;
    ELI_ASSERT_EQ(a2, 64u);

    eli_destroy_context(ctx);
}

ELI_TEST(get_color_u32_col32_scales_alpha) {
    eli_context *ctx = eli_create_context();
    ctx->style.alpha = 1.0f;

    eli_col32 in = ELI_COL32(200, 100, 50, 200);
    /* alpha_mul 0.5 * style.alpha 1.0 -> 200 * 0.5 = 100. */
    eli_col32 out = eli_get_color_u32_col32(in, 0.5f);
    uint32_t a = (out & ELI_COL32_A_MASK) >> ELI_COL32_A_SHIFT;
    ELI_ASSERT_EQ(a, 100u);
    /* RGB bytes unchanged. */
    ELI_ASSERT_EQ(out & 0x00FFFFFFu, in & 0x00FFFFFFu);

    /* alpha_mul >= 1.0 returns the color unchanged. */
    eli_col32 same = eli_get_color_u32_col32(in, 1.0f);
    ELI_ASSERT_EQ(same, in);

    eli_destroy_context(ctx);
}

ELI_TEST(packed_float_roundtrip) {
    eli_col32 packed = ELI_COL32(12, 34, 56, 78);
    eli_vec4 f = eli_color_convert_u32_to_float4(packed);
    eli_col32 back = eli_color_convert_float4_to_u32(f);
    ELI_ASSERT_EQ(back, packed);
}

ELI_TEST(rgb_hsv_roundtrip) {
    const float samples[][3] = {
        {0.20f, 0.50f, 0.80f},
        {0.90f, 0.10f, 0.30f},
        {0.33f, 0.66f, 0.10f},
        {0.50f, 0.50f, 0.50f},
        {1.00f, 0.00f, 0.00f},
    };
    for (int i = 0; i < 5; i++) {
        float r = samples[i][0], g = samples[i][1], b = samples[i][2];
        float h, s, v;
        eli_color_convert_rgb_to_hsv(r, g, b, &h, &s, &v);
        float r2, g2, b2;
        eli_color_convert_hsv_to_rgb(h, s, v, &r2, &g2, &b2);
        ELI_ASSERT_FLT_NEAR(r2, r, 1e-4);
        ELI_ASSERT_FLT_NEAR(g2, g, 1e-4);
        ELI_ASSERT_FLT_NEAR(b2, b, 1e-4);
    }
}

ELI_TEST(hsv_gray_has_zero_saturation) {
    float h, s, v;
    eli_color_convert_rgb_to_hsv(0.4f, 0.4f, 0.4f, &h, &s, &v);
    ELI_ASSERT_FLT_NEAR(s, 0.0f, 1e-5);
    ELI_ASSERT_FLT_NEAR(v, 0.4f, 1e-5);
}

ELI_TEST(style_color_names) {
    ELI_ASSERT_STR_EQ(eli_get_style_color_name(ELI_COL_TEXT), "Text");
    ELI_ASSERT_STR_EQ(eli_get_style_color_name(ELI_COL_TITLE_BG_ACTIVE), "TitleBgActive");
    ELI_ASSERT_STR_EQ(eli_get_style_color_name(ELI_COL_TAB_DIMMED_SELECTED_OVERLINE),
                      "TabDimmedSelectedOverline");
    ELI_ASSERT_STR_EQ(eli_get_style_color_name(ELI_COL_MODAL_WINDOW_DIM_BG), "ModalWindowDimBg");
    ELI_ASSERT_STR_EQ(eli_get_style_color_name(-1), "Unknown");
    ELI_ASSERT_STR_EQ(eli_get_style_color_name(ELI_COL_COUNT), "Unknown");
}

ELI_TEST(get_style_color_vec4_reads_slot) {
    eli_context *ctx = eli_create_context();
    eli_style_colors_dark(NULL);

    eli_vec4 text = eli_get_style_color_vec4(ELI_COL_TEXT);
    ELI_ASSERT_FLT_NEAR(text.x, 1.0f, 1e-6);
    ELI_ASSERT_FLT_NEAR(text.w, 1.0f, 1e-6);

    eli_destroy_context(ctx);
}

ELI_TEST_MAIN()
