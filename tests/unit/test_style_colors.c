/**
 * @file test_style_colors.c
 * @brief Unit tests for elimgui built-in themes: exact RGBA values for dark,
 *        light, and classic, plus the derived/aliased slots (tab lerps,
 *        separator = border).
 *
 * @status Phase 6 theme coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/style/eli_style.h>

static void assert_color(eli_vec4 c, float r, float g, float b, float a)
{
    ELI_ASSERT_FLT_NEAR(c.x, r, 1e-5);
    ELI_ASSERT_FLT_NEAR(c.y, g, 1e-5);
    ELI_ASSERT_FLT_NEAR(c.z, b, 1e-5);
    ELI_ASSERT_FLT_NEAR(c.w, a, 1e-5);
}

ELI_TEST(dark_theme_exact_values) {
    eli_style st = {0};
    eli_style_colors_dark(&st);

    assert_color(st.colors[ELI_COL_TEXT], 1.00f, 1.00f, 1.00f, 1.00f);
    assert_color(st.colors[ELI_COL_TEXT_DISABLED], 0.50f, 0.50f, 0.50f, 1.00f);
    assert_color(st.colors[ELI_COL_WINDOW_BG], 0.06f, 0.06f, 0.06f, 0.94f);
    assert_color(st.colors[ELI_COL_TITLE_BG_ACTIVE], 0.16f, 0.29f, 0.48f, 1.00f);
    assert_color(st.colors[ELI_COL_CHECK_MARK], 0.26f, 0.59f, 0.98f, 1.00f);
    assert_color(st.colors[ELI_COL_MODAL_WINDOW_DIM_BG], 0.80f, 0.80f, 0.80f, 0.35f);
}

ELI_TEST(dark_theme_derived_slots) {
    eli_style st = {0};
    eli_style_colors_dark(&st);

    /* Aliased slots. */
    assert_color(st.colors[ELI_COL_SEPARATOR],
                 st.colors[ELI_COL_BORDER].x, st.colors[ELI_COL_BORDER].y,
                 st.colors[ELI_COL_BORDER].z, st.colors[ELI_COL_BORDER].w);
    assert_color(st.colors[ELI_COL_TAB_HOVERED],
                 st.colors[ELI_COL_HEADER_HOVERED].x, st.colors[ELI_COL_HEADER_HOVERED].y,
                 st.colors[ELI_COL_HEADER_HOVERED].z, st.colors[ELI_COL_HEADER_HOVERED].w);
    assert_color(st.colors[ELI_COL_TEXT_LINK],
                 st.colors[ELI_COL_HEADER_ACTIVE].x, st.colors[ELI_COL_HEADER_ACTIVE].y,
                 st.colors[ELI_COL_HEADER_ACTIVE].z, st.colors[ELI_COL_HEADER_ACTIVE].w);

    /* Tab = lerp(Header, TitleBgActive, 0.80). */
    eli_vec4 header = st.colors[ELI_COL_HEADER];
    eli_vec4 tba = st.colors[ELI_COL_TITLE_BG_ACTIVE];
    assert_color(st.colors[ELI_COL_TAB],
                 header.x + (tba.x - header.x) * 0.80f,
                 header.y + (tba.y - header.y) * 0.80f,
                 header.z + (tba.z - header.z) * 0.80f,
                 header.w + (tba.w - header.w) * 0.80f);
}

ELI_TEST(light_theme_exact_values) {
    eli_style st = {0};
    eli_style_colors_light(&st);

    assert_color(st.colors[ELI_COL_TEXT], 0.00f, 0.00f, 0.00f, 1.00f);
    assert_color(st.colors[ELI_COL_WINDOW_BG], 0.94f, 0.94f, 0.94f, 1.00f);
    assert_color(st.colors[ELI_COL_FRAME_BG], 1.00f, 1.00f, 1.00f, 1.00f);
    assert_color(st.colors[ELI_COL_SEPARATOR], 0.39f, 0.39f, 0.39f, 0.62f);
    /* NavCursor aliases HeaderHovered in the light theme. */
    assert_color(st.colors[ELI_COL_NAV_CURSOR],
                 st.colors[ELI_COL_HEADER_HOVERED].x, st.colors[ELI_COL_HEADER_HOVERED].y,
                 st.colors[ELI_COL_HEADER_HOVERED].z, st.colors[ELI_COL_HEADER_HOVERED].w);
}

ELI_TEST(classic_theme_exact_values) {
    eli_style st = {0};
    eli_style_colors_classic(&st);

    assert_color(st.colors[ELI_COL_TEXT], 0.90f, 0.90f, 0.90f, 1.00f);
    assert_color(st.colors[ELI_COL_WINDOW_BG], 0.00f, 0.00f, 0.00f, 0.85f);
    assert_color(st.colors[ELI_COL_TITLE_BG], 0.27f, 0.27f, 0.54f, 0.83f);
    assert_color(st.colors[ELI_COL_CHECK_MARK], 0.90f, 0.90f, 0.90f, 0.50f);
    assert_color(st.colors[ELI_COL_TEXT_SELECTED_BG], 0.00f, 0.00f, 1.00f, 0.35f);
}

ELI_TEST(theme_null_uses_current_context) {
    eli_context *ctx = eli_create_context();
    ELI_ASSERT_NOT_NULL(ctx);

    eli_style_colors_dark(NULL);
    assert_color(ctx->style.colors[ELI_COL_TEXT], 1.00f, 1.00f, 1.00f, 1.00f);

    eli_style_colors_classic(NULL);
    assert_color(ctx->style.colors[ELI_COL_TEXT], 0.90f, 0.90f, 0.90f, 1.00f);

    eli_destroy_context(ctx);
}

ELI_TEST_MAIN()
