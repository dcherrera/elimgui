/**
 * @file test_widget_status.c
 * @brief Unit tests for Phase 10 item-status queries: the activation edges
 *        (activated / active / deactivated / deactivated-after-edit), the any-item
 *        aggregates while an item is held, item visibility, and the last-item id +
 *        rect accessors. Driven across frames via the input backend.
 *
 * @status Phase 10 item-status coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/widgets/eli_widgets.h>
#include <eli/font/eli_font.h>

#define TEST_DT (1.0f / 60.0f)

static eli_font_atlas *g_atlas = NULL;

static eli_context *status_setup(void)
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

static void status_teardown(eli_context *ctx)
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
    eli_set_next_window_size(eli_make_vec2(300.0f, 300.0f), 0);
    eli_begin("W", NULL, 0);
}

static eli_vec2 rect_center(eli_rect r)
{
    return eli_make_vec2(r.x + r.w * 0.5f, r.y + r.h * 0.5f);
}

/* ------------------------------------------------------------------------- */

ELI_TEST(activation_and_deactivation_edges) {
    eli_context *ctx = status_setup();

    /* Frame 1: establish the button rect. */
    eli_io_add_mouse_pos_event(20.0f, 40.0f);
    frame_begin();
    open_win();
    eli_button("E");
    eli_end();
    frame_end();
    eli_vec2 c = rect_center(ctx->last_item_rect);

    /* Frame 2: press -> activated edge + active + any-item-active. */
    eli_io_add_mouse_pos_event(c.x, c.y);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    frame_begin();
    open_win();
    eli_button("E");
    ELI_ASSERT_TRUE(eli_is_item_activated());
    ELI_ASSERT_TRUE(eli_is_item_active());
    ELI_ASSERT_TRUE(eli_is_any_item_active());
    ELI_ASSERT_FALSE(eli_is_item_deactivated());
    eli_end();
    frame_end();

    /* Frame 3: hold (button still down) -> active but not a fresh activation. */
    eli_io_add_mouse_pos_event(c.x, c.y);
    frame_begin();
    open_win();
    eli_button("E");
    ELI_ASSERT_FALSE(eli_is_item_activated());
    ELI_ASSERT_TRUE(eli_is_item_active());
    ELI_ASSERT_TRUE(eli_is_any_item_active());
    eli_end();
    frame_end();

    /* Frame 4: release -> deactivated edge, no longer active. */
    eli_io_add_mouse_pos_event(c.x, c.y);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, false);
    frame_begin();
    open_win();
    eli_button("E");
    ELI_ASSERT_TRUE(eli_is_item_deactivated());
    ELI_ASSERT_FALSE(eli_is_item_active());
    ELI_ASSERT_FALSE(eli_is_any_item_active());
    eli_end();
    frame_end();

    status_teardown(ctx);
}

ELI_TEST(checkbox_deactivated_after_edit) {
    eli_context *ctx = status_setup();
    bool v = false;

    eli_io_add_mouse_pos_event(20.0f, 40.0f);
    frame_begin();
    open_win();
    eli_checkbox("C", &v);
    eli_end();
    frame_end();
    eli_vec2 c = rect_center(ctx->last_item_rect);

    /* Press (activate, no edit yet). */
    eli_io_add_mouse_pos_event(c.x, c.y);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    frame_begin();
    open_win();
    eli_checkbox("C", &v);
    ELI_ASSERT_FALSE(eli_is_item_deactivated_after_edit());
    eli_end();
    frame_end();

    /* Release inside -> edit + deactivate the same frame. */
    eli_io_add_mouse_pos_event(c.x, c.y);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, false);
    frame_begin();
    open_win();
    eli_checkbox("C", &v);
    ELI_ASSERT_TRUE(eli_is_item_edited());
    ELI_ASSERT_TRUE(eli_is_item_deactivated());
    ELI_ASSERT_TRUE(eli_is_item_deactivated_after_edit());
    eli_end();
    frame_end();

    status_teardown(ctx);
}

ELI_TEST(item_id_rect_and_visibility_accessors) {
    eli_context *ctx = status_setup();

    frame_begin();
    open_win();
    eli_vec2 pos_before = eli_get_cursor_screen_pos();
    eli_button("Named");
    eli_id id = eli_get_item_id();
    eli_vec2 mn = eli_get_item_rect_min();
    eli_vec2 mx = eli_get_item_rect_max();
    eli_vec2 sz = eli_get_item_rect_size();

    ELI_ASSERT_EQ(id, eli_get_id("Named"));
    ELI_ASSERT_NE(id, 0u);
    ELI_ASSERT_FLT_NEAR(mn.x, pos_before.x, 0.01f);
    ELI_ASSERT_FLT_NEAR(mn.y, pos_before.y, 0.01f);
    ELI_ASSERT_FLT_NEAR(mx.x - mn.x, sz.x, 0.01f);
    ELI_ASSERT_FLT_NEAR(mx.y - mn.y, sz.y, 0.01f);
    ELI_ASSERT_TRUE(eli_is_item_visible());
    eli_end();
    frame_end();

    status_teardown(ctx);
}

ELI_TEST(text_item_is_non_interactive) {
    eli_context *ctx = status_setup();

    frame_begin();
    open_win();
    eli_text("plain");
    /* Non-interactive text has id 0 and is never active/hovered-as-item... */
    ELI_ASSERT_EQ(eli_get_item_id(), 0u);
    ELI_ASSERT_FALSE(eli_is_item_active());
    ELI_ASSERT_TRUE(eli_is_item_visible());
    eli_end();
    frame_end();

    status_teardown(ctx);
}

ELI_TEST_MAIN()
