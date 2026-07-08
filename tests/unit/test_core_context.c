/**
 * @file test_core_context.c
 * @brief Unit tests for elimgui context/IO/style: create->frame->destroy
 *        lifecycle, current-context accessors, default style values, and the
 *        zeroed color theme.
 *
 * @status Phase 1 context coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/core/eli_core.h>

ELI_TEST(context_create_sets_current_and_accessors) {
    ELI_ASSERT_NULL(eli_get_current_context());

    eli_context *ctx = eli_create_context();
    ELI_ASSERT_NOT_NULL(ctx);
    /* First created context becomes current automatically. */
    ELI_ASSERT_EQ(eli_get_current_context(), ctx);
    ELI_ASSERT_TRUE(ctx->initialized);

    /* IO and style accessors point into the current context. */
    ELI_ASSERT_EQ(eli_get_io(), &ctx->io);
    ELI_ASSERT_EQ(eli_get_style(), &ctx->style);

    eli_destroy_context(ctx);
    /* Destroying the current context clears the current pointer. */
    ELI_ASSERT_NULL(eli_get_current_context());
}

ELI_TEST(context_set_and_get_current) {
    eli_context *a = eli_create_context();
    eli_context *b = eli_create_context();
    ELI_ASSERT_NOT_NULL(a);
    ELI_ASSERT_NOT_NULL(b);
    /* 'a' was current (created first); creating 'b' does not steal current. */
    ELI_ASSERT_EQ(eli_get_current_context(), a);

    eli_set_current_context(b);
    ELI_ASSERT_EQ(eli_get_current_context(), b);

    /* Destroying a non-current context leaves current intact. */
    eli_destroy_context(a);
    ELI_ASSERT_EQ(eli_get_current_context(), b);

    eli_destroy_context(b);
    ELI_ASSERT_NULL(eli_get_current_context());
}

ELI_TEST(frame_lifecycle_counts_and_scope) {
    eli_context *ctx = eli_create_context();
    ELI_ASSERT_EQ(ctx->frame_count, 0);
    ELI_ASSERT_FALSE(ctx->within_frame_scope);

    ctx->io.delta_time = 1.0f / 60.0f;
    eli_new_frame();
    ELI_ASSERT_EQ(ctx->frame_count, 1);
    ELI_ASSERT_TRUE(ctx->within_frame_scope);
    ELI_ASSERT_FLT_NEAR(ctx->io.framerate, 60.0f, 1e-3);

    /* render() closes the frame scope even without an explicit end_frame(). */
    eli_render();
    ELI_ASSERT_FALSE(ctx->within_frame_scope);
    /* No draw data is produced in Phase 1. */
    ELI_ASSERT_NULL(eli_get_draw_data());

    /* A second frame advances the counter. */
    eli_new_frame();
    eli_end_frame();
    ELI_ASSERT_EQ(ctx->frame_count, 2);
    ELI_ASSERT_FALSE(ctx->within_frame_scope);

    eli_destroy_context(ctx);
}

ELI_TEST(destroy_null_destroys_current) {
    eli_context *ctx = eli_create_context();
    ELI_ASSERT_EQ(eli_get_current_context(), ctx);
    /* Passing NULL targets the current context. */
    eli_destroy_context(NULL);
    ELI_ASSERT_NULL(eli_get_current_context());
}

ELI_TEST(default_style_scalar_values) {
    eli_context *ctx = eli_create_context();
    eli_style *s = &ctx->style;

    ELI_ASSERT_FLT_NEAR(s->alpha, 1.0f, 1e-6);
    ELI_ASSERT_FLT_NEAR(s->disabled_alpha, 0.60f, 1e-6);
    ELI_ASSERT_FLT_NEAR(s->window_border_size, 1.0f, 1e-6);
    ELI_ASSERT_FLT_NEAR(s->window_rounding, 0.0f, 1e-6);
    ELI_ASSERT_FLT_NEAR(s->indent_spacing, 21.0f, 1e-6);
    ELI_ASSERT_FLT_NEAR(s->scrollbar_size, 14.0f, 1e-6);
    ELI_ASSERT_FLT_NEAR(s->scrollbar_rounding, 9.0f, 1e-6);
    ELI_ASSERT_FLT_NEAR(s->grab_min_size, 12.0f, 1e-6);
    ELI_ASSERT_FLT_NEAR(s->tab_rounding, 5.0f, 1e-6);
    ELI_ASSERT_FLT_NEAR(s->separator_text_border_size, 3.0f, 1e-6);
    ELI_ASSERT_FLT_NEAR(s->curve_tessellation_tol, 1.25f, 1e-6);
    ELI_ASSERT_FLT_NEAR(s->circle_tessellation_max_error, 0.30f, 1e-6);
    /* 35 degrees in radians. */
    ELI_ASSERT_FLT_NEAR(s->table_angled_headers_angle, 0.61086524f, 1e-5);

    ELI_ASSERT_TRUE(s->anti_aliased_lines);
    ELI_ASSERT_TRUE(s->anti_aliased_lines_use_tex);
    ELI_ASSERT_TRUE(s->anti_aliased_fill);

    ELI_ASSERT_EQ(s->window_menu_button_position, ELI_DIR_LEFT);
    ELI_ASSERT_EQ(s->color_button_position, ELI_DIR_RIGHT);

    eli_destroy_context(ctx);
}

ELI_TEST(default_style_vec2_values) {
    eli_context *ctx = eli_create_context();
    eli_style *s = &ctx->style;

    ELI_ASSERT_FLT_NEAR(s->window_padding.x, 8.0f, 1e-6);
    ELI_ASSERT_FLT_NEAR(s->window_padding.y, 8.0f, 1e-6);
    ELI_ASSERT_FLT_NEAR(s->frame_padding.x, 4.0f, 1e-6);
    ELI_ASSERT_FLT_NEAR(s->frame_padding.y, 3.0f, 1e-6);
    ELI_ASSERT_FLT_NEAR(s->item_spacing.x, 8.0f, 1e-6);
    ELI_ASSERT_FLT_NEAR(s->item_spacing.y, 4.0f, 1e-6);
    ELI_ASSERT_FLT_NEAR(s->cell_padding.x, 4.0f, 1e-6);
    ELI_ASSERT_FLT_NEAR(s->cell_padding.y, 2.0f, 1e-6);
    ELI_ASSERT_FLT_NEAR(s->window_min_size.x, 32.0f, 1e-6);
    ELI_ASSERT_FLT_NEAR(s->window_title_align.y, 0.5f, 1e-6);
    ELI_ASSERT_FLT_NEAR(s->button_text_align.x, 0.5f, 1e-6);
    ELI_ASSERT_FLT_NEAR(s->button_text_align.y, 0.5f, 1e-6);

    eli_destroy_context(ctx);
}

ELI_TEST(default_style_colors_are_zeroed) {
    eli_context *ctx = eli_create_context();
    /* Themes are applied by the style phase; Phase 1 leaves colors[] zeroed. */
    for (int i = 0; i < ELI_COL_COUNT; i++) {
        ELI_ASSERT_FLT_NEAR(ctx->style.colors[i].x, 0.0f, 1e-6);
        ELI_ASSERT_FLT_NEAR(ctx->style.colors[i].y, 0.0f, 1e-6);
        ELI_ASSERT_FLT_NEAR(ctx->style.colors[i].z, 0.0f, 1e-6);
        ELI_ASSERT_FLT_NEAR(ctx->style.colors[i].w, 0.0f, 1e-6);
    }
    eli_destroy_context(ctx);
}

ELI_TEST(default_io_values) {
    eli_context *ctx = eli_create_context();
    eli_io *io = &ctx->io;

    ELI_ASSERT_FLT_NEAR(io->font_global_scale, 1.0f, 1e-6);
    ELI_ASSERT_FLT_NEAR(io->display_framebuffer_scale.x, 1.0f, 1e-6);
    ELI_ASSERT_EQ(io->mouse_draw_cursor, ELI_MOUSE_CURSOR_ARROW);
    /* Mouse starts at an invalid (offscreen) position. */
    ELI_ASSERT_LT(io->mouse_pos.x, 0.0f);
    /* Capture-intent outputs start clear. */
    ELI_ASSERT_FALSE(io->want_capture_mouse);
    ELI_ASSERT_FALSE(io->want_capture_keyboard);

    eli_destroy_context(ctx);
}

ELI_TEST_MAIN()
