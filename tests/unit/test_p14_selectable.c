/**
 * @file test_p14_selectable.c
 * @brief Unit tests for Phase 14 selectables: eli_selectable returns true on a
 *        click-release, draws a highlight quad when selected, and eli_selectable_bool
 *        toggles the bound bool. Interaction is driven across frames via the input
 *        backend, matching the browser event flow.
 *
 * @status Phase 14 selectable coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/widgets/eli_widgets.h>
#include <eli/widgets/eli_combo.h>
#include <eli/font/eli_font.h>

#define TEST_DT (1.0f / 60.0f)

static eli_font_atlas *g_atlas = NULL;

static eli_context *sel_setup(void)
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

static void sel_teardown(eli_context *ctx)
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
    eli_set_next_window_size(eli_make_vec2(300.0f, 300.0f), 0);
    eli_begin("W", NULL, 0);
}

static eli_vec2 rect_center(eli_rect r)
{
    return eli_make_vec2(r.x + r.w * 0.5f, r.y + r.h * 0.5f);
}

/* ------------------------------------------------------------------------- */

ELI_TEST(selectable_pressed_on_click_release) {
    eli_context *ctx = sel_setup();

    /* Frame 1: submit to establish the row rect. */
    eli_io_add_mouse_pos_event(500.0f, 500.0f);
    frame_begin();
    open_win();
    ELI_ASSERT_FALSE(eli_selectable("Row", false, ELI_SELECTABLE_NONE, eli_make_vec2(0.0f, 0.0f)));
    eli_end();
    frame_end();
    eli_vec2 c = rect_center(ctx->last_item_rect);

    /* Frame 2: press inside -> not yet pressed. */
    eli_io_add_mouse_pos_event(c.x, c.y);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    frame_begin();
    open_win();
    ELI_ASSERT_FALSE(eli_selectable("Row", false, ELI_SELECTABLE_NONE, eli_make_vec2(0.0f, 0.0f)));
    eli_end();
    frame_end();

    /* Frame 3: release inside -> pressed. */
    eli_io_add_mouse_pos_event(c.x, c.y);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, false);
    frame_begin();
    open_win();
    ELI_ASSERT_TRUE(eli_selectable("Row", false, ELI_SELECTABLE_NONE, eli_make_vec2(0.0f, 0.0f)));
    eli_end();
    frame_end();

    sel_teardown(ctx);
}

ELI_TEST(selectable_selected_draws_highlight) {
    eli_context *ctx = sel_setup();

    /* Mouse far away so neither row is hovered; the only difference between the
     * two rows is the `selected` flag, so a larger vertex delta proves the
     * selected row emitted the header highlight quad. */
    eli_io_add_mouse_pos_event(900.0f, 900.0f);
    frame_begin();
    open_win();
    eli_draw_list *dl = eli_get_window_draw_list();

    uint32_t v0 = dl->vtx_count;
    eli_selectable("Row##a", false, ELI_SELECTABLE_NONE, eli_make_vec2(0.0f, 0.0f));
    uint32_t delta_unselected = dl->vtx_count - v0;

    uint32_t v1 = dl->vtx_count;
    eli_selectable("Row##b", true, ELI_SELECTABLE_NONE, eli_make_vec2(0.0f, 0.0f));
    uint32_t delta_selected = dl->vtx_count - v1;

    ELI_ASSERT_GT(delta_selected, delta_unselected);
    eli_end();
    frame_end();

    sel_teardown(ctx);
}

ELI_TEST(selectable_bool_toggles) {
    eli_context *ctx = sel_setup();
    bool value = false;

    /* Frame 1: establish rect. */
    eli_io_add_mouse_pos_event(500.0f, 500.0f);
    frame_begin();
    open_win();
    eli_selectable_bool("B", &value, ELI_SELECTABLE_NONE, eli_make_vec2(0.0f, 0.0f));
    eli_end();
    frame_end();
    eli_vec2 c = rect_center(ctx->last_item_rect);
    ELI_ASSERT_FALSE(value);

    /* Frame 2: press. */
    eli_io_add_mouse_pos_event(c.x, c.y);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    frame_begin();
    open_win();
    eli_selectable_bool("B", &value, ELI_SELECTABLE_NONE, eli_make_vec2(0.0f, 0.0f));
    eli_end();
    frame_end();

    /* Frame 3: release -> toggles true. */
    eli_io_add_mouse_pos_event(c.x, c.y);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, false);
    frame_begin();
    open_win();
    bool pressed = eli_selectable_bool("B", &value, ELI_SELECTABLE_NONE, eli_make_vec2(0.0f, 0.0f));
    eli_end();
    frame_end();
    ELI_ASSERT_TRUE(pressed);
    ELI_ASSERT_TRUE(value);

    /* Frames 4-5: press+release again -> toggles back to false. */
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    frame_begin();
    open_win();
    eli_selectable_bool("B", &value, ELI_SELECTABLE_NONE, eli_make_vec2(0.0f, 0.0f));
    eli_end();
    frame_end();

    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, false);
    frame_begin();
    open_win();
    eli_selectable_bool("B", &value, ELI_SELECTABLE_NONE, eli_make_vec2(0.0f, 0.0f));
    eli_end();
    frame_end();
    ELI_ASSERT_FALSE(value);

    sel_teardown(ctx);
}

ELI_TEST_MAIN()
