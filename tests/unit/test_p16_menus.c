/**
 * @file test_p16_menus.c
 * @brief Unit tests for Phase 16 menus: menu-bar gating on ELI_WINDOW_MENU_BAR,
 *        opening a bar menu's dropdown by clicking its button, activating a
 *        menu item, toggling a bool item (and its check-mark geometry), the menu
 *        closing after an item is chosen, and shortcut-width measurement. Driven
 *        across frames through the input backend, mirroring the popup tests.
 *
 * @status Phase 16 menu coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/widgets/eli_widgets.h>
#include <eli/widgets/eli_popup.h>
#include <eli/widgets/eli_menu.h>
#include <eli/font/eli_font.h>

#define TEST_DT (1.0f / 60.0f)

static eli_font_atlas *g_atlas = NULL;

static eli_context *menu_setup(void)
{
    eli_context *ctx = eli_create_context();
    ctx->io.delta_time = TEST_DT;
    ctx->io.display_size = eli_make_vec2(1024.0f, 768.0f);
    eli_style_colors_dark(&ctx->style);
    g_atlas = eli_font_atlas_create();
    eli_font *f = eli_font_atlas_add_font_default(g_atlas, NULL);
    eli_font_atlas_build(g_atlas);
    eli_push_font(f);
    /* The popup/menu stacks are file-static and outlive a context; clear any
     * open popup left by a previous test so no dangling window pointer leaks in. */
    eli_close_popup_to_level(0);
    g_eli_menu_open_root_id = 0u;
    g_eli_menu_depth = 0;
    return ctx;
}

static void menu_teardown(eli_context *ctx)
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

/* Host window W at (0,0)-(400,400) with a menu bar. */
static void open_menu_win(void)
{
    eli_set_next_window_pos(eli_make_vec2(0.0f, 0.0f), 0, eli_make_vec2(0.0f, 0.0f));
    eli_set_next_window_size(eli_make_vec2(400.0f, 400.0f), 0);
    eli_begin("W", NULL, ELI_WINDOW_MENU_BAR);
}

static eli_vec2 rect_center(eli_rect r)
{
    return eli_make_vec2(r.x + r.w * 0.5f, r.y + r.h * 0.5f);
}

/* ------------------------------------------------------------------------- */

ELI_TEST(menu_bar_requires_flag) {
    eli_context *ctx = menu_setup();

    /* A plain window (no ELI_WINDOW_MENU_BAR) rejects the menu bar. */
    frame_begin();
    eli_set_next_window_pos(eli_make_vec2(0.0f, 0.0f), 0, eli_make_vec2(0.0f, 0.0f));
    eli_set_next_window_size(eli_make_vec2(300.0f, 300.0f), 0);
    eli_begin("Plain", NULL, 0);
    ELI_ASSERT_FALSE(eli_begin_menu_bar());
    eli_end();
    frame_end();

    /* A menu-bar window accepts it. */
    frame_begin();
    open_menu_win();
    ELI_ASSERT_TRUE(eli_begin_menu_bar());
    eli_end_menu_bar();
    eli_end();
    frame_end();

    menu_teardown(ctx);
}

ELI_TEST(click_bar_menu_opens_dropdown) {
    eli_context *ctx = menu_setup();

    /* Frame 1: bar menu closed; capture the "File" button rect. */
    eli_io_add_mouse_pos_event(2.0f, 2.0f);
    frame_begin();
    open_menu_win();
    eli_begin_menu_bar();
    /* Popup ids are window-relative (eli_begin seeds the id stack with the window
     * id), so capture the "File" menu id inside the window scope to query it after
     * eli_end when the current window seed is no longer active. */
    eli_id file_id = eli_get_id("File");
    ELI_ASSERT_FALSE(eli_begin_menu("File", true));
    ELI_ASSERT_FALSE(eli_is_popup_open_id(file_id, ELI_POPUP_NONE));
    eli_rect file_btn = ctx->last_item_rect;
    eli_end_menu_bar();
    eli_end();
    frame_end();
    eli_vec2 c = rect_center(file_btn);

    /* Frame 2: press left over "File" -> dropdown opens and its body is emitted. */
    eli_io_add_mouse_pos_event(c.x, c.y);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    frame_begin();
    open_menu_win();
    eli_begin_menu_bar();
    bool opened = eli_begin_menu("File", true);
    ELI_ASSERT_TRUE(opened);
    eli_menu_item("Open", "Ctrl+O", false, true);
    eli_end_menu();
    eli_end_menu_bar();
    eli_end();
    frame_end();
    ELI_ASSERT_TRUE(eli_is_popup_open_id(file_id, ELI_POPUP_NONE));

    menu_teardown(ctx);
}

ELI_TEST(menu_item_click_returns_true_and_closes_menu) {
    eli_context *ctx = menu_setup();
    eli_rect item_rect = eli_make_rect(0.0f, 0.0f, 0.0f, 0.0f);

    /* Frame 1: capture File button rect. */
    eli_io_add_mouse_pos_event(2.0f, 2.0f);
    frame_begin();
    open_menu_win();
    eli_begin_menu_bar();
    /* Window-relative "File" popup id, captured inside the window scope. */
    eli_id file_id = eli_get_id("File");
    eli_begin_menu("File", true);
    eli_rect file_btn = ctx->last_item_rect;
    eli_end_menu_bar();
    eli_end();
    frame_end();
    eli_vec2 fc = rect_center(file_btn);

    /* Frame 2: click File -> opens (dropdown appears tiny this frame). */
    eli_io_add_mouse_pos_event(fc.x, fc.y);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    frame_begin();
    open_menu_win();
    eli_begin_menu_bar();
    if (eli_begin_menu("File", true)) {
        eli_menu_item("Open", "Ctrl+O", false, true);
        eli_end_menu();
    }
    eli_end_menu_bar();
    eli_end();
    frame_end();
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, false);

    /* Frame 3: settle (mouse parked in empty body, no click) so the auto-resize
     * dropdown reaches full size; capture the "Open" item rect. */
    eli_io_add_mouse_pos_event(380.0f, 380.0f);
    frame_begin();
    open_menu_win();
    eli_begin_menu_bar();
    if (eli_begin_menu("File", true)) {
        eli_menu_item("Open", "Ctrl+O", false, true);
        item_rect = eli_get_item_rect();
        eli_end_menu();
    }
    eli_end_menu_bar();
    eli_end();
    frame_end();
    ELI_ASSERT_TRUE(eli_is_popup_open_id(file_id, ELI_POPUP_NONE));
    eli_vec2 ic = rect_center(item_rect);

    /* Frame 4: press left over the "Open" item (down edge, not yet chosen). */
    eli_io_add_mouse_pos_event(ic.x, ic.y);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    frame_begin();
    open_menu_win();
    eli_begin_menu_bar();
    bool chosen_down = false;
    if (eli_begin_menu("File", true)) {
        chosen_down = eli_menu_item("Open", "Ctrl+O", false, true);
        eli_end_menu();
    }
    eli_end_menu_bar();
    eli_end();
    frame_end();
    ELI_ASSERT_FALSE(chosen_down);
    ELI_ASSERT_TRUE(eli_is_popup_open_id(file_id, ELI_POPUP_NONE));

    /* Frame 5: release over the item -> menu item fires and the menu closes. */
    eli_io_add_mouse_pos_event(ic.x, ic.y);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, false);
    frame_begin();
    open_menu_win();
    eli_begin_menu_bar();
    bool chosen = false;
    if (eli_begin_menu("File", true)) {
        chosen = eli_menu_item("Open", "Ctrl+O", false, true);
        eli_end_menu();
    }
    eli_end_menu_bar();
    eli_end();
    frame_end();
    ELI_ASSERT_TRUE(chosen);
    ELI_ASSERT_FALSE(eli_is_popup_open_id(file_id, ELI_POPUP_NONE));

    menu_teardown(ctx);
}

ELI_TEST(menu_item_bool_toggles_and_shows_check) {
    eli_context *ctx = menu_setup();
    bool wrap = false;
    eli_rect item_rect = eli_make_rect(0.0f, 0.0f, 0.0f, 0.0f);

    /* Frame 1: capture File button rect. */
    eli_io_add_mouse_pos_event(2.0f, 2.0f);
    frame_begin();
    open_menu_win();
    eli_begin_menu_bar();
    eli_begin_menu("File", true);
    eli_rect file_btn = ctx->last_item_rect;
    eli_end_menu_bar();
    eli_end();
    frame_end();
    eli_vec2 fc = rect_center(file_btn);

    /* Frame 2: click File -> opens (dropdown appears tiny this frame). */
    eli_io_add_mouse_pos_event(fc.x, fc.y);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    frame_begin();
    open_menu_win();
    eli_begin_menu_bar();
    if (eli_begin_menu("File", true)) {
        eli_menu_item_bool("Wrap", NULL, &wrap, true);
        eli_end_menu();
    }
    eli_end_menu_bar();
    eli_end();
    frame_end();
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, false);

    /* Frame 3: settle so the dropdown reaches full size; capture "Wrap" rect. */
    eli_io_add_mouse_pos_event(380.0f, 380.0f);
    frame_begin();
    open_menu_win();
    eli_begin_menu_bar();
    if (eli_begin_menu("File", true)) {
        eli_menu_item_bool("Wrap", NULL, &wrap, true);
        item_rect = eli_get_item_rect();
        eli_end_menu();
    }
    eli_end_menu_bar();
    eli_end();
    frame_end();
    ELI_ASSERT_FALSE(wrap);
    eli_vec2 ic = rect_center(item_rect);

    /* Frame 4: press over the item (down edge). Menu must stay open. */
    eli_io_add_mouse_pos_event(ic.x, ic.y);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    frame_begin();
    open_menu_win();
    eli_begin_menu_bar();
    if (eli_begin_menu("File", true)) {
        eli_menu_item_bool("Wrap", NULL, &wrap, true);
        eli_end_menu();
    }
    eli_end_menu_bar();
    eli_end();
    frame_end();
    ELI_ASSERT_FALSE(wrap);

    /* Frame 5: release over the item -> bool toggles to true. */
    eli_io_add_mouse_pos_event(ic.x, ic.y);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, false);
    frame_begin();
    open_menu_win();
    eli_begin_menu_bar();
    if (eli_begin_menu("File", true)) {
        eli_menu_item_bool("Wrap", NULL, &wrap, true);
        eli_end_menu();
    }
    eli_end_menu_bar();
    eli_end();
    frame_end();
    ELI_ASSERT_TRUE(wrap);

    menu_teardown(ctx);
}

ELI_TEST(selected_item_draws_check_mark_geometry) {
    eli_context *ctx = menu_setup();

    /* Emit two identical-width menu rows in the full-size, unclipped host window
     * with the mouse parked far away (no hover highlight on either). The selected
     * row must add the two-segment check-mark geometry (more indices). */
    eli_io_add_mouse_pos_event(390.0f, 390.0f);
    frame_begin();
    open_menu_win();
    eli_draw_list *dl = eli_get_window_draw_list();

    uint32_t idx0 = dl->idx_count;
    eli_menu_item("A", NULL, false, true);
    uint32_t delta_plain = dl->idx_count - idx0;

    uint32_t idx1 = dl->idx_count;
    eli_menu_item("B", NULL, true, true);
    uint32_t delta_checked = dl->idx_count - idx1;

    eli_end();
    frame_end();

    ELI_ASSERT_GT((int)delta_checked, (int)delta_plain);

    menu_teardown(ctx);
}

ELI_TEST(shortcut_width_is_measured) {
    eli_context *ctx = menu_setup();

    frame_begin();
    open_menu_win();
    float without = eli_menu__calc_min_width("Open", NULL, false);
    float with = eli_menu__calc_min_width("Open", "Ctrl+O", false);
    float shortcut_w = eli_calc_text_size("Ctrl+O", NULL).x;
    eli_end();
    frame_end();

    /* Adding a shortcut widens the row by at least the shortcut's measured width. */
    ELI_ASSERT_GT(with, without);
    ELI_ASSERT_GE(with - without, shortcut_w);

    menu_teardown(ctx);
}

ELI_TEST_MAIN()
