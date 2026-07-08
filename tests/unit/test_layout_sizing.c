/**
 * @file test_layout_sizing.c
 * @brief Unit tests for the vertical sizing helpers: text line height (with/without
 *        item spacing) and frame height (with/without item spacing), derived from
 *        the current font size and style padding/spacing.
 *
 * @status Phase 8 sizing-helper coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/layout/eli_layout.h>

ELI_TEST(sizing_helpers_from_font_and_style) {
    eli_context *ctx = eli_create_context();
    ctx->font_size = 13.0f;
    /* Defaults: frame_padding.y = 3, item_spacing.y = 4. */

    ELI_ASSERT_FLT_NEAR(eli_get_text_line_height(), 13.0f, 0.01f);
    ELI_ASSERT_FLT_NEAR(eli_get_text_line_height_with_spacing(), 17.0f, 0.01f);
    /* Frame height = font_size + frame_padding.y * 2 = 13 + 6 = 19. */
    ELI_ASSERT_FLT_NEAR(eli_get_frame_height(), 19.0f, 0.01f);
    ELI_ASSERT_FLT_NEAR(eli_get_frame_height_with_spacing(), 23.0f, 0.01f);

    eli_destroy_context(ctx);
}

ELI_TEST(sizing_helpers_scale_with_font_size) {
    eli_context *ctx = eli_create_context();
    ctx->font_size = 20.0f;

    ELI_ASSERT_FLT_NEAR(eli_get_text_line_height(), 20.0f, 0.01f);
    ELI_ASSERT_FLT_NEAR(eli_get_frame_height(), 26.0f, 0.01f);

    eli_destroy_context(ctx);
}

ELI_TEST_MAIN()
