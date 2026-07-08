/**
 * @file test_p14_listbox.c
 * @brief Unit tests for Phase 14 list boxes: eli_begin_list_box / eli_end_list_box
 *        host selectable rows inside a framed child, and eli_list_box updates the
 *        current index (returning true) when a row is clicked. The item geometry is
 *        discovered from the container's selectables, then clicked across frames via
 *        the input backend.
 *
 * @status Phase 14 list box coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/widgets/eli_widgets.h>
#include <eli/widgets/eli_combo.h>
#include <eli/font/eli_font.h>

#define TEST_DT (1.0f / 60.0f)

static eli_font_atlas *g_atlas = NULL;
static const char *const g_items[] = {"alpha", "bravo", "charlie"};

static eli_context *lb_setup(void)
{
    eli_context *ctx = eli_create_context();
    ctx->io.delta_time = TEST_DT;
    ctx->io.display_size = eli_make_vec2(1024.0f, 768.0f);
    eli_style_colors_dark(NULL);
    g_atlas = eli_font_atlas_create();
    eli_font *f = eli_font_atlas_add_font_default(g_atlas, NULL);
    eli_font_atlas_build(g_atlas);
    eli_push_font(f);
    return ctx;
}

static void lb_teardown(eli_context *ctx)
{
    eli_destroy_context(ctx);
    eli_font_atlas_destroy(g_atlas);
    g_atlas = NULL;
}

static void frame_begin(void)
{
    eli_new_frame();
    eli_input_update_begin_frame();
    eli_window_new_frame();
    eli_popup_new_frame();
}

static void frame_end(void)
{
    eli_popup_end_frame();
    eli_window_render();
    eli_render();
    eli_input_update_end_frame();
}

static void open_win(void)
{
    eli_set_next_window_pos(eli_make_vec2(0.0f, 0.0f), 0, eli_make_vec2(0.0f, 0.0f));
    eli_set_next_window_size(eli_make_vec2(300.0f, 400.0f), 0);
    eli_begin("W", NULL, 0);
}

static eli_vec2 rect_center(eli_rect r)
{
    return eli_make_vec2(r.x + r.w * 0.5f, r.y + r.h * 0.5f);
}

/* Emit a manual list box ("LB") and return the middle row's rect (captured before
 * end_group overwrites the last-item rect with the whole-widget bounds). The
 * geometry matches eli_list_box's item layout because it shares the same label,
 * child, and item order. */
static eli_rect manual_listbox_capture_row1(void)
{
    eli_context *ctx = eli_get_current_context();
    eli_rect row1 = eli_make_rect(0.0f, 0.0f, 0.0f, 0.0f);
    if (eli_begin_list_box("LB", eli_make_vec2(0.0f, 0.0f))) {
        for (int i = 0; i < 3; i++) {
            eli_push_id_int(i);
            eli_selectable(g_items[i], false, ELI_SELECTABLE_NONE, eli_make_vec2(0.0f, 0.0f));
            if (i == 1)
                row1 = ctx->last_item_rect;
            eli_pop_id();
        }
        eli_end_list_box();
    }
    return row1;
}

/* ------------------------------------------------------------------------- */

ELI_TEST(begin_list_box_hosts_selectables) {
    eli_context *ctx = lb_setup();

    eli_io_add_mouse_pos_event(900.0f, 900.0f);
    frame_begin();
    open_win();
    eli_rect row1 = manual_listbox_capture_row1();
    eli_end();
    frame_end();

    /* The middle row got a non-empty rect, i.e. selectables were hosted and laid
     * out inside the list-box child. */
    ELI_ASSERT_GT(row1.h, 0.0f);
    ELI_ASSERT_GT(row1.w, 0.0f);

    lb_teardown(ctx);
}

ELI_TEST(list_box_selection_changes_index) {
    eli_context *ctx = lb_setup();
    int current = 0;

    /* Frames 1-2: discover the middle row's rect (settle the child geometry). */
    eli_rect row1 = eli_make_rect(0, 0, 0, 0);
    for (int frame = 0; frame < 2; frame++) {
        eli_io_add_mouse_pos_event(900.0f, 900.0f);
        frame_begin();
        open_win();
        row1 = manual_listbox_capture_row1();
        eli_end();
        frame_end();
    }
    eli_vec2 click = rect_center(row1);
    ELI_ASSERT_GT(row1.h, 0.0f);

    /* Frame 3: press the middle row via eli_list_box (down edge). */
    eli_io_add_mouse_pos_event(click.x, click.y);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    frame_begin();
    open_win();
    ELI_ASSERT_FALSE(eli_list_box("LB", &current, g_items, 3, -1));
    eli_end();
    frame_end();

    /* Frame 4: release over the middle row -> index becomes 1 and it returns true. */
    eli_io_add_mouse_pos_event(click.x, click.y);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, false);
    frame_begin();
    open_win();
    bool changed = eli_list_box("LB", &current, g_items, 3, -1);
    eli_end();
    frame_end();

    ELI_ASSERT_TRUE(changed);
    ELI_ASSERT_EQ(current, 1);

    lb_teardown(ctx);
}

ELI_TEST_MAIN()
