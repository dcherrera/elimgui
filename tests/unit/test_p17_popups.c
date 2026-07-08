/**
 * @file test_p17_popups.c
 * @brief Unit tests for Phase 17 popups and modals: open/query, begin/end and
 *        close-current-popup, click-outside close, Escape close, modal stays open
 *        until explicitly closed, and context-item open on right-click. Driven
 *        across frames through the input backend.
 *
 * @status Phase 17 popup/modal coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/widgets/eli_widgets.h>
#include <eli/widgets/eli_popup.h>
#include <eli/font/eli_font.h>

#define TEST_DT (1.0f / 60.0f)

static eli_font_atlas *g_atlas = NULL;

static eli_context *popup_setup(void)
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

static void popup_teardown(eli_context *ctx)
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

/* Host window W at (0,0)-(300,300). */
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

ELI_TEST(open_popup_sets_is_open) {
    eli_context *ctx = popup_setup();

    frame_begin();
    open_win();
    ELI_ASSERT_FALSE(eli_is_popup_open("x", ELI_POPUP_NONE));
    eli_open_popup("x", ELI_POPUP_NONE);
    ELI_ASSERT_TRUE(eli_is_popup_open("x", ELI_POPUP_NONE));
    ELI_ASSERT_TRUE(eli_is_popup_open(NULL, ELI_POPUP_ANY_POPUP));
    eli_end();
    frame_end();

    popup_teardown(ctx);
}

ELI_TEST(begin_popup_true_then_false_after_close) {
    eli_context *ctx = popup_setup();

    /* Frame 1: open + begin the same frame, close inside, verify state. */
    frame_begin();
    open_win();
    eli_open_popup("p", ELI_POPUP_NONE);
    ELI_ASSERT_TRUE(eli_is_popup_open("p", ELI_POPUP_NONE));
    bool opened = eli_begin_popup("p", 0);
    ELI_ASSERT_TRUE(opened);
    eli_close_current_popup();
    eli_end_popup();
    ELI_ASSERT_FALSE(eli_is_popup_open("p", ELI_POPUP_NONE));
    eli_end();
    frame_end();

    /* Frame 2: the popup is no longer open, so begin returns false. */
    frame_begin();
    open_win();
    ELI_ASSERT_FALSE(eli_begin_popup("p", 0));
    eli_end();
    frame_end();

    popup_teardown(ctx);
}

ELI_TEST(click_outside_closes_popup) {
    eli_context *ctx = popup_setup();

    /* Frame 1: open a popup anchored at (400,400) (mouse-on-open). */
    eli_io_add_mouse_pos_event(400.0f, 400.0f);
    frame_begin();
    open_win();
    eli_open_popup("cp", ELI_POPUP_NONE);
    ELI_ASSERT_TRUE(eli_begin_popup("cp", 0));
    eli_end_popup();
    eli_end();
    frame_end();

    /* Frame 2: mouse still over the popup, no click -> stays open. */
    eli_io_add_mouse_pos_event(405.0f, 405.0f);
    frame_begin();
    open_win();
    ELI_ASSERT_TRUE(eli_is_popup_open("cp", ELI_POPUP_NONE));
    ELI_ASSERT_TRUE(eli_begin_popup("cp", 0));
    eli_end_popup();
    eli_end();
    frame_end();

    /* Frame 3: click inside W but outside the popup -> popup closes. */
    eli_io_add_mouse_pos_event(150.0f, 150.0f);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    frame_begin();
    open_win();
    ELI_ASSERT_FALSE(eli_is_popup_open("cp", ELI_POPUP_NONE));
    ELI_ASSERT_FALSE(eli_begin_popup("cp", 0));
    eli_end();
    frame_end();

    popup_teardown(ctx);
}

ELI_TEST(escape_closes_popup) {
    eli_context *ctx = popup_setup();

    /* Frame 1: open + show the popup. */
    eli_io_add_mouse_pos_event(200.0f, 200.0f);
    frame_begin();
    open_win();
    eli_open_popup("ep", ELI_POPUP_NONE);
    ELI_ASSERT_TRUE(eli_begin_popup("ep", 0));
    eli_end_popup();
    eli_end();
    frame_end();

    /* Frame 2: press Escape -> popup closes in popup_new_frame. */
    eli_io_add_key_event(ELI_KEY_ESCAPE, true);
    frame_begin();
    open_win();
    ELI_ASSERT_FALSE(eli_is_popup_open("ep", ELI_POPUP_NONE));
    ELI_ASSERT_FALSE(eli_begin_popup("ep", 0));
    eli_end();
    frame_end();

    popup_teardown(ctx);
}

ELI_TEST(modal_stays_open_until_closed) {
    eli_context *ctx = popup_setup();

    /* Frame 1: open + begin the modal. */
    eli_io_add_mouse_pos_event(100.0f, 100.0f);
    frame_begin();
    open_win();
    eli_open_popup("M", ELI_POPUP_NONE);
    ELI_ASSERT_TRUE(eli_begin_popup_modal("M", NULL, 0));
    eli_end_popup();
    eli_end();
    frame_end();

    /* Frame 2: click outside the modal -> the dim backdrop blocks the click and
     * the modal stays open. Then close it explicitly. */
    eli_io_add_mouse_pos_event(10.0f, 10.0f);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    frame_begin();
    open_win();
    ELI_ASSERT_TRUE(eli_is_popup_open("M", ELI_POPUP_NONE));
    ELI_ASSERT_TRUE(eli_begin_popup_modal("M", NULL, 0));
    eli_close_current_popup();
    eli_end_popup();
    eli_end();
    frame_end();

    /* Frame 3: after explicit close the modal is gone. */
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, false);
    frame_begin();
    open_win();
    ELI_ASSERT_FALSE(eli_is_popup_open("M", ELI_POPUP_NONE));
    ELI_ASSERT_FALSE(eli_begin_popup_modal("M", NULL, 0));
    eli_end();
    frame_end();

    popup_teardown(ctx);
}

ELI_TEST(context_item_opens_on_right_click) {
    eli_context *ctx = popup_setup();

    /* Frame 1: establish the button rect; no click yet. */
    eli_io_add_mouse_pos_event(20.0f, 40.0f);
    frame_begin();
    open_win();
    eli_button("Btn");
    ELI_ASSERT_FALSE(eli_begin_popup_context_item("cim", ELI_POPUP_MOUSE_BUTTON_RIGHT));
    eli_end();
    frame_end();
    eli_vec2 c = rect_center(ctx->last_item_rect);

    /* Frame 2: press right button over the item (down edge, not released). */
    eli_io_add_mouse_pos_event(c.x, c.y);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_RIGHT, true);
    frame_begin();
    open_win();
    eli_button("Btn");
    ELI_ASSERT_FALSE(eli_begin_popup_context_item("cim", ELI_POPUP_MOUSE_BUTTON_RIGHT));
    eli_end();
    frame_end();

    /* Frame 3: release over the item -> the context popup opens and begins. */
    eli_io_add_mouse_pos_event(c.x, c.y);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_RIGHT, false);
    frame_begin();
    open_win();
    eli_button("Btn");
    bool opened = eli_begin_popup_context_item("cim", ELI_POPUP_MOUSE_BUTTON_RIGHT);
    ELI_ASSERT_TRUE(opened);
    eli_end_popup();
    ELI_ASSERT_TRUE(eli_is_popup_open("cim", ELI_POPUP_NONE));
    eli_end();
    frame_end();

    popup_teardown(ctx);
}

ELI_TEST_MAIN()
