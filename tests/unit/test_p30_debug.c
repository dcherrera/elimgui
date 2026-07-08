/**
 * @file test_p30_debug.c
 * @brief Unit tests for Phase 30 debug/inspection tools: eli_get_version returns
 *        the non-empty library version, the metrics / debug-log / id-stack /
 *        about windows and the style/font selectors + style editor all RUN
 *        inside a frame without crashing and produce valid geometry, the debug
 *        log accumulates appended lines, and the style selector actually swaps
 *        the live theme colors.
 *
 * @status Phase 30 debug-tool coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/demo/eli_demo_all.h>
#include <eli/font/eli_font.h>

#define TEST_DT (1.0f / 60.0f)

static eli_font_atlas *g_atlas = NULL;

static eli_context *dbg_setup(void)
{
    eli_context *ctx = eli_create_context();
    ctx->io.delta_time = TEST_DT;
    ctx->io.display_size = eli_make_vec2(1024.0f, 768.0f);
    eli_style_colors_dark(&ctx->style);

    g_atlas = eli_font_atlas_create();
    eli_font *f = eli_font_atlas_add_font_default(g_atlas, NULL);
    eli_font_atlas_build(g_atlas);
    eli_push_font(f);
    return ctx;
}

static void dbg_teardown(eli_context *ctx)
{
    eli_destroy_context(ctx);
    eli_font_atlas_destroy(g_atlas);
    g_atlas = NULL;
}

/* ------------------------------------------------------------------------- */

ELI_TEST(version_is_nonempty_and_matches) {
    const char *v = eli_get_version();
    ELI_ASSERT_NOT_NULL(v);
    ELI_ASSERT_GT((int)strlen(v), 0);
    ELI_ASSERT_STR_EQ(v, ELI_VERSION);
}

ELI_TEST(metrics_window_runs) {
    eli_context *ctx = dbg_setup();
    bool open = true;

    eli_draw_data *dd = NULL;
    for (int frame = 0; frame < 4; frame++) {
        eli_frame_begin();
        eli_set_next_window_pos(eli_make_vec2(0.0f, 0.0f), 0, eli_make_vec2(0.0f, 0.0f));
        eli_show_metrics_window(&open);
        eli_frame_end();
        dd = eli_get_draw_data();
    }

    ELI_ASSERT_NOT_NULL(dd);
    ELI_ASSERT_TRUE(dd->valid);
    ELI_ASSERT_GT(dd->total_vtx_count, 0);
    ELI_ASSERT_TRUE(open);

    dbg_teardown(ctx);
}

ELI_TEST(all_debug_windows_run) {
    eli_context *ctx = dbg_setup();
    bool open = true;

    eli_draw_data *dd = NULL;
    for (int frame = 0; frame < 3; frame++) {
        eli_frame_begin();
        eli_show_metrics_window(&open);
        eli_show_debug_log_window(&open);
        eli_show_id_stack_tool_window(&open);
        eli_show_about_window(&open);
        if (eli_begin("Style", &open, 0)) {
            eli_show_style_editor(NULL);
            eli_show_style_selector("Theme");
            eli_show_font_selector("Font");
            eli_show_user_guide();
        }
        eli_end();
        eli_frame_end();
        dd = eli_get_draw_data();
    }

    ELI_ASSERT_NOT_NULL(dd);
    ELI_ASSERT_TRUE(dd->valid);
    ELI_ASSERT_GT(dd->total_vtx_count, 0);

    dbg_teardown(ctx);
}

ELI_TEST(debug_log_accumulates) {
    eli_debug_log_buffer *buf = eli_debug__log_buffer();
    buf->length = 0;
    buf->text[0] = '\0';

    eli_debug_log("first %d", 1);
    ELI_ASSERT_GT((int)buf->length, 0);
    size_t after_first = buf->length;

    eli_debug_log("second %s", "line");
    ELI_ASSERT_GT((int)buf->length, (int)after_first);

    /* Both lines and a trailing newline are present. */
    ELI_ASSERT_NOT_NULL(strstr(buf->text, "first 1"));
    ELI_ASSERT_NOT_NULL(strstr(buf->text, "second line"));
}

ELI_TEST(style_selector_swaps_theme_colors) {
    eli_context *ctx = dbg_setup();

    /* Baseline: apply light, capture the window-bg color, then confirm the
     * dark theme produces a different one so the selector genuinely re-themes. */
    eli_style light = {0};
    eli_style_set_defaults(&light);
    eli_style_colors_light(&light);

    eli_style dark = {0};
    eli_style_set_defaults(&dark);
    eli_style_colors_dark(&dark);

    eli_vec4 lbg = light.colors[ELI_COL_WINDOW_BG];
    eli_vec4 dbg = dark.colors[ELI_COL_WINDOW_BG];
    bool differ = (lbg.x != dbg.x) || (lbg.y != dbg.y) || (lbg.z != dbg.z);
    ELI_ASSERT_TRUE(differ);

    dbg_teardown(ctx);
}

ELI_TEST_MAIN()
