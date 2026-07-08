/**
 * @file test_integration_frame.c
 * @brief End-to-end frame-lifecycle coverage: eli_frame_begin / eli_frame_end
 *        drive several full frames containing a window with a button, a table, a
 *        tab bar, and a popup without crashing, produce populated draw data, and
 *        release all subsystem state on eli_destroy_context.
 *
 * @status Integration coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/elimgui.h>

#define TEST_DT (1.0f / 60.0f)

static eli_font_atlas *g_atlas = NULL;

static eli_context *frame_setup(void)
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

static void frame_teardown(eli_context *ctx)
{
    eli_destroy_context(ctx);
    eli_font_atlas_destroy(g_atlas);
    g_atlas = NULL;
}

/* Emit a window with one widget of every family the frame hooks service. */
static void emit_ui(void)
{
    eli_set_next_window_pos(eli_make_vec2(0.0f, 0.0f), 0, eli_make_vec2(0.0f, 0.0f));
    eli_set_next_window_size(eli_make_vec2(500.0f, 400.0f), 0);
    if (eli_begin("Main", NULL, 0)) {
        eli_button("Click");

        if (eli_begin_table("Grid", 2, 0)) {
            eli_table_setup_column("A", 0, 0.0f, 0);
            eli_table_setup_column("B", 0, 0.0f, 0);
            eli_table_headers_row();
            eli_table_next_row();
            eli_table_next_column();
            eli_text("r0c0");
            eli_table_next_column();
            eli_text("r0c1");
            eli_end_table();
        }

        if (eli_begin_tab_bar("Tabs", 0)) {
            if (eli_begin_tab_item("One", NULL, 0))
                eli_end_tab_item();
            if (eli_begin_tab_item("Two", NULL, 0))
                eli_end_tab_item();
            eli_end_tab_bar();
        }

        eli_open_popup("Ctx", 0);
        if (eli_begin_popup("Ctx", 0)) {
            eli_text("popup body");
            eli_end_popup();
        }
    }
    eli_end();
}

ELI_TEST(full_frame_cycles_produce_draw_data) {
    eli_context *ctx = frame_setup();

    for (int i = 0; i < 5; i++) {
        eli_frame_begin();
        emit_ui();
        eli_frame_end();

        /* Frame scope is closed and the id stack fully unwound each cycle. */
        ELI_ASSERT_FALSE(ctx->within_frame_scope);
        ELI_ASSERT_EQ(ctx->id_stack_size, 0);
        ELI_ASSERT_EQ(ctx->window_stack_size, 0);

        eli_draw_data *dd = eli_get_draw_data();
        ELI_ASSERT_NOT_NULL(dd);
        ELI_ASSERT_TRUE(dd->valid);
        /* At least the window decorations produced geometry. */
        ELI_ASSERT_GT(dd->cmd_lists_count, 0);
        ELI_ASSERT_GT(dd->total_vtx_count, 0);
        ELI_ASSERT_GT(dd->total_idx_count, 0);
    }

    /* The frame layer registered the widget shutdown hook so destroy releases
     * the table/tab pools and drag-drop payload (verified under the runner's
     * clean-exit; no assertion needed beyond not crashing). */
    ELI_ASSERT_NOT_NULL(ctx->widget_shutdown_fn);
    frame_teardown(ctx);
}

ELI_TEST(frame_end_closes_scope_without_ui) {
    eli_context *ctx = frame_setup();

    /* An empty frame (no windows) must still compose cleanly and publish valid,
     * empty draw data. */
    eli_frame_begin();
    eli_frame_end();

    ELI_ASSERT_FALSE(ctx->within_frame_scope);
    eli_draw_data *dd = eli_get_draw_data();
    ELI_ASSERT_NOT_NULL(dd);
    ELI_ASSERT_TRUE(dd->valid);
    ELI_ASSERT_EQ(dd->cmd_lists_count, 0);

    frame_teardown(ctx);
}

ELI_TEST_MAIN()
