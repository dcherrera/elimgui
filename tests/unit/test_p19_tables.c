/**
 * @file test_p19_tables.c
 * @brief Unit tests for Phase 19 tables: column count, cursor advancement per
 *        cell, stretch width splitting, set_column_index jumps, column-name setup,
 *        header click-to-sort (asc/desc toggling), hidden-column exclusion, and
 *        row background colors. Interaction is driven across frames through the
 *        input backend, mirroring the browser event flow.
 *
 * @status Phase 19 table coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/widgets/eli_table.h>
#include <eli/widgets/eli_widgets.h>
#include <eli/font/eli_font.h>

#include <string.h>

#define TEST_DT (1.0f / 60.0f)

static eli_font_atlas *g_atlas = NULL;

static eli_context *table_setup(void)
{
    eli_context *ctx = eli_create_context();
    ctx->io.delta_time = TEST_DT;
    ctx->io.display_size = eli_make_vec2(1024.0f, 768.0f);

    g_atlas = eli_font_atlas_create();
    eli_font *f = eli_font_atlas_add_font_default(g_atlas, NULL);
    eli_font_atlas_build(g_atlas);
    eli_push_font(f);
    return ctx;
}

static void table_teardown(eli_context *ctx)
{
    eli_table_shutdown();
    eli_destroy_context(ctx);
    eli_font_atlas_destroy(g_atlas);
    g_atlas = NULL;
}

static void frame_begin(void)
{
    eli_new_frame();
    eli_input_update_begin_frame();
    eli_window_new_frame();
    eli_table_new_frame();
}

static void frame_end(void)
{
    eli_window_render();
    eli_render();
    eli_input_update_end_frame();
}

static void open_win(void)
{
    eli_set_next_window_pos(eli_make_vec2(0.0f, 0.0f), 0, eli_make_vec2(0.0f, 0.0f));
    eli_set_next_window_size(eli_make_vec2(500.0f, 400.0f), 0);
    eli_begin("W", NULL, 0);
}

/* ------------------------------------------------------------------------- */

ELI_TEST(begin_table_reports_column_count) {
    eli_context *ctx = table_setup();

    frame_begin();
    open_win();
    ELI_ASSERT_TRUE(eli_begin_table("t", 3, ELI_TABLE_NONE));
    ELI_ASSERT_EQ(eli_table_get_column_count(), 3);
    eli_end_table();
    eli_end();
    frame_end();

    table_teardown(ctx);
}

ELI_TEST(cells_advance_cursor_to_column_positions) {
    eli_context *ctx = table_setup();

    frame_begin();
    open_win();
    ELI_ASSERT_TRUE(eli_begin_table("t", 3, ELI_TABLE_NONE));
    eli_table_next_row();
    eli_table *table = eli_table_get_current();
    ELI_ASSERT_NOT_NULL(table);

    for (int i = 0; i < 3; i++) {
        eli_table_next_column();
        ELI_ASSERT_EQ(eli_table_get_column_index(), i);
        eli_vec2 cur = eli_get_cursor_screen_pos();
        ELI_ASSERT_FLT_NEAR(cur.x, table->columns[i].work_min_x, 0.01f);
    }
    eli_end_table();
    eli_end();
    frame_end();

    table_teardown(ctx);
}

ELI_TEST(stretch_columns_split_available_width) {
    eli_context *ctx = table_setup();

    frame_begin();
    open_win();
    ELI_ASSERT_TRUE(eli_begin_table("t", 3, ELI_TABLE_NONE));
    eli_table_setup_column("A", ELI_TABLE_COLUMN_WIDTH_STRETCH, 0.0f, 0);
    eli_table_setup_column("B", ELI_TABLE_COLUMN_WIDTH_STRETCH, 0.0f, 0);
    eli_table_setup_column("C", ELI_TABLE_COLUMN_WIDTH_STRETCH, 0.0f, 0);
    eli_table_next_row();
    eli_table *table = eli_table_get_current();

    float work_w = table->work_max_x - table->work_min_x;
    float expect = work_w / 3.0f;
    ELI_ASSERT_GT(work_w, 0.0f);
    ELI_ASSERT_FLT_NEAR(table->columns[0].width_given, expect, 0.5f);
    ELI_ASSERT_FLT_NEAR(table->columns[1].width_given, expect, 0.5f);
    ELI_ASSERT_FLT_NEAR(table->columns[2].width_given, expect, 0.5f);
    float sum = table->columns[0].width_given + table->columns[1].width_given +
                table->columns[2].width_given;
    ELI_ASSERT_FLT_NEAR(sum, work_w, 0.5f);

    eli_end_table();
    eli_end();
    frame_end();

    table_teardown(ctx);
}

ELI_TEST(set_column_index_jumps) {
    eli_context *ctx = table_setup();

    frame_begin();
    open_win();
    ELI_ASSERT_TRUE(eli_begin_table("t", 3, ELI_TABLE_NONE));
    eli_table_next_row();
    eli_table *table = eli_table_get_current();

    ELI_ASSERT_TRUE(eli_table_set_column_index(2));
    ELI_ASSERT_EQ(eli_table_get_column_index(), 2);
    eli_vec2 cur = eli_get_cursor_screen_pos();
    ELI_ASSERT_FLT_NEAR(cur.x, table->columns[2].work_min_x, 0.01f);

    ELI_ASSERT_TRUE(eli_table_set_column_index(0));
    ELI_ASSERT_EQ(eli_table_get_column_index(), 0);
    cur = eli_get_cursor_screen_pos();
    ELI_ASSERT_FLT_NEAR(cur.x, table->columns[0].work_min_x, 0.01f);

    eli_end_table();
    eli_end();
    frame_end();

    table_teardown(ctx);
}

ELI_TEST(setup_column_names_are_queryable) {
    eli_context *ctx = table_setup();

    frame_begin();
    open_win();
    ELI_ASSERT_TRUE(eli_begin_table("t", 3, ELI_TABLE_NONE));
    eli_table_setup_column("Alpha", ELI_TABLE_COLUMN_NONE, 0.0f, 0);
    eli_table_setup_column("Beta", ELI_TABLE_COLUMN_NONE, 0.0f, 0);
    eli_table_setup_column("Gamma", ELI_TABLE_COLUMN_NONE, 0.0f, 0);

    ELI_ASSERT_STR_EQ(eli_table_get_column_name(0), "Alpha");
    ELI_ASSERT_STR_EQ(eli_table_get_column_name(1), "Beta");
    ELI_ASSERT_STR_EQ(eli_table_get_column_name(2), "Gamma");

    eli_end_table();
    eli_end();
    frame_end();

    table_teardown(ctx);
}

ELI_TEST(hidden_column_is_excluded) {
    eli_context *ctx = table_setup();

    frame_begin();
    open_win();
    ELI_ASSERT_TRUE(eli_begin_table("t", 3, ELI_TABLE_NONE));
    eli_table_setup_column("A", ELI_TABLE_COLUMN_WIDTH_STRETCH, 0.0f, 0);
    eli_table_setup_column("B", ELI_TABLE_COLUMN_WIDTH_STRETCH | ELI_TABLE_COLUMN_DEFAULT_HIDE,
                           0.0f, 0);
    eli_table_setup_column("C", ELI_TABLE_COLUMN_WIDTH_STRETCH, 0.0f, 0);
    eli_table_next_row();
    eli_table *table = eli_table_get_current();

    ELI_ASSERT_FALSE(table->columns[1].is_enabled);
    ELI_ASSERT_FLT_NEAR(table->columns[1].width_given, 0.0f, 0.01f);
    ELI_ASSERT_FALSE(eli_table_get_column_flags(1) & ELI_TABLE_COLUMN_IS_ENABLED);
    ELI_ASSERT_TRUE(eli_table_get_column_flags(0) & ELI_TABLE_COLUMN_IS_ENABLED);

    /* The two visible columns split the whole work width between them. */
    float work_w = table->work_max_x - table->work_min_x;
    float sum = table->columns[0].width_given + table->columns[2].width_given;
    ELI_ASSERT_FLT_NEAR(sum, work_w, 0.5f);
    ELI_ASSERT_EQ(eli_table_get_column_count(), 3);

    eli_end_table();
    eli_end();
    frame_end();

    table_teardown(ctx);
}

ELI_TEST(row_bg_color_is_stored) {
    eli_context *ctx = table_setup();

    frame_begin();
    open_win();
    ELI_ASSERT_TRUE(eli_begin_table("t", 2, ELI_TABLE_ROW_BG));
    eli_table_next_row();
    eli_table_next_column();
    eli_col32 col = ELI_COL32(10, 20, 30, 255);
    eli_table_set_bg_color(ELI_TABLE_BG_TARGET_ROW_BG0, col, -1);
    eli_table *table = eli_table_get_current();
    ELI_ASSERT_EQ((int)table->row_bg_color[0], (int)col);

    /* A cell background targets the current column. */
    eli_col32 cell = ELI_COL32(1, 2, 3, 255);
    eli_table_set_bg_color(ELI_TABLE_BG_TARGET_CELL_BG, cell, -1);
    ELI_ASSERT_EQ((int)table->cell_bg_color[0], (int)cell);

    eli_end_table();
    eli_end();
    frame_end();

    table_teardown(ctx);
}

/* Drive the header at `hdr_center` through a press+release to click it, running a
 * full table frame each step, then return the primary sort column/direction. */
static void run_table_frame(void)
{
    frame_begin();
    open_win();
    if (eli_begin_table("st", 3,
                        ELI_TABLE_SORTABLE | ELI_TABLE_RESIZABLE | ELI_TABLE_BORDERS)) {
        eli_table_setup_column("A", ELI_TABLE_COLUMN_NONE, 0.0f, 100);
        eli_table_setup_column("B", ELI_TABLE_COLUMN_NONE, 0.0f, 200);
        eli_table_setup_column("C", ELI_TABLE_COLUMN_NONE, 0.0f, 300);
        eli_table_headers_row();
        eli_table_next_row();
        eli_table_next_column();
        eli_text("x");
        eli_end_table();
    }
    eli_end();
    frame_end();
}

ELI_TEST(header_click_updates_sort_specs) {
    eli_context *ctx = table_setup();
    eli_rect hdr0 = eli_make_rect(0, 0, 0, 0);

    /* Frame 1: submit the table to establish the header rect for hover next frame. */
    eli_io_add_mouse_pos_event(5.0f, 5.0f);
    frame_begin();
    open_win();
    ELI_ASSERT_TRUE(eli_begin_table("st", 3,
                                    ELI_TABLE_SORTABLE | ELI_TABLE_RESIZABLE | ELI_TABLE_BORDERS));
    eli_table_setup_column("A", ELI_TABLE_COLUMN_NONE, 0.0f, 100);
    eli_table_setup_column("B", ELI_TABLE_COLUMN_NONE, 0.0f, 200);
    eli_table_setup_column("C", ELI_TABLE_COLUMN_NONE, 0.0f, 300);
    eli_table_next_row_ex(ELI_TABLE_ROW_HEADERS,
                          eli_get_font_size() + ctx->style.cell_padding.y * 2.0f);
    eli_table_set_column_index(0);
    eli_table_header("A");
    hdr0 = eli_get_item_rect();
    eli_end_table();
    eli_end();
    frame_end();

    ELI_ASSERT_GT(hdr0.w, 0.0f);
    eli_vec2 c = eli_make_vec2(hdr0.x + hdr0.w * 0.5f, hdr0.y + hdr0.h * 0.5f);

    /* Press on the header. */
    eli_io_add_mouse_pos_event(c.x, c.y);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    run_table_frame();

    /* Release inside -> click fires -> column 0 sorts ascending. */
    eli_io_add_mouse_pos_event(c.x, c.y);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, false);
    frame_begin();
    open_win();
    eli_begin_table("st", 3, ELI_TABLE_SORTABLE | ELI_TABLE_RESIZABLE | ELI_TABLE_BORDERS);
    eli_table_setup_column("A", ELI_TABLE_COLUMN_NONE, 0.0f, 100);
    eli_table_setup_column("B", ELI_TABLE_COLUMN_NONE, 0.0f, 200);
    eli_table_setup_column("C", ELI_TABLE_COLUMN_NONE, 0.0f, 300);
    eli_table_next_row_ex(ELI_TABLE_ROW_HEADERS,
                          eli_get_font_size() + ctx->style.cell_padding.y * 2.0f);
    eli_table_set_column_index(0);
    eli_table_header("A");
    eli_table_sort_specs *specs = eli_table_get_sort_specs();
    ELI_ASSERT_NOT_NULL(specs);
    ELI_ASSERT_EQ(specs->specs_count, 1);
    ELI_ASSERT_EQ(specs->specs[0].column_index, 0);
    ELI_ASSERT_EQ(specs->specs[0].sort_direction, ELI_SORT_ASCENDING);
    ELI_ASSERT_EQ((int)specs->specs[0].column_user_id, 100);
    eli_end_table();
    eli_end();
    frame_end();

    /* Second click toggles to descending: press then release. */
    eli_io_add_mouse_pos_event(c.x, c.y);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    run_table_frame();

    eli_io_add_mouse_pos_event(c.x, c.y);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, false);
    frame_begin();
    open_win();
    eli_begin_table("st", 3, ELI_TABLE_SORTABLE | ELI_TABLE_RESIZABLE | ELI_TABLE_BORDERS);
    eli_table_setup_column("A", ELI_TABLE_COLUMN_NONE, 0.0f, 100);
    eli_table_setup_column("B", ELI_TABLE_COLUMN_NONE, 0.0f, 200);
    eli_table_setup_column("C", ELI_TABLE_COLUMN_NONE, 0.0f, 300);
    eli_table_next_row_ex(ELI_TABLE_ROW_HEADERS,
                          eli_get_font_size() + ctx->style.cell_padding.y * 2.0f);
    eli_table_set_column_index(0);
    eli_table_header("A");
    specs = eli_table_get_sort_specs();
    ELI_ASSERT_NOT_NULL(specs);
    ELI_ASSERT_EQ(specs->specs_count, 1);
    ELI_ASSERT_EQ(specs->specs[0].sort_direction, ELI_SORT_DESCENDING);
    eli_end_table();
    eli_end();
    frame_end();

    table_teardown(ctx);
}

ELI_TEST_MAIN()
