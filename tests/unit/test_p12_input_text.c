/**
 * @file test_p12_input_text.c
 * @brief Phase 12 tests for eli_input_text: click activation (active id set),
 *        character insertion from the IO queue, backspace deletion, the edited
 *        flag / return-on-enter, and the placeholder hint. Interaction is driven
 *        across frames by feeding mouse/key/char events through the input backend.
 *
 * @status Phase 12 text-input coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/widgets/eli_input_text_widget.h>
#include <eli/font/eli_font.h>

#define TEST_DT (1.0f / 60.0f)

static eli_font_atlas *g_atlas = NULL;

static eli_context *setup(void)
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

static void teardown(eli_context *ctx)
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

static void type_string(const char *s)
{
    for (const char *p = s; *p != '\0'; p++)
        eli_io_add_input_character((unsigned int)(unsigned char)*p);
}

/* ------------------------------------------------------------------------- */

ELI_TEST(input_text_click_activates_field) {
    eli_context *ctx = setup();
    char buf[32] = "";

    /* Frame 1: submit to establish the frame rect. */
    eli_io_add_mouse_pos_event(20.0f, 40.0f);
    frame_begin();
    open_win();
    eli_input_text("Name", buf, sizeof(buf), 0, NULL, NULL);
    eli_rect r = ctx->last_item_rect;
    eli_end();
    frame_end();
    eli_vec2 c = rect_center(r);
    ELI_ASSERT_EQ(eli_get_active_id(), 0u);

    /* Frame 2: click inside -> becomes the active item. */
    eli_io_add_mouse_pos_event(c.x, c.y);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    frame_begin();
    open_win();
    eli_input_text("Name", buf, sizeof(buf), 0, NULL, NULL);
    ELI_ASSERT_TRUE(eli_is_item_active());
    ELI_ASSERT_NE(eli_get_active_id(), 0u);
    eli_end();
    frame_end();

    teardown(ctx);
}

ELI_TEST(input_text_typing_inserts_and_marks_edited) {
    eli_context *ctx = setup();
    char buf[32] = "";

    eli_io_add_mouse_pos_event(20.0f, 40.0f);
    frame_begin();
    open_win();
    eli_input_text("Name", buf, sizeof(buf), 0, NULL, NULL);
    eli_vec2 c = rect_center(ctx->last_item_rect);
    eli_end();
    frame_end();

    /* Activate. */
    eli_io_add_mouse_pos_event(c.x, c.y);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    frame_begin();
    open_win();
    eli_input_text("Name", buf, sizeof(buf), 0, NULL, NULL);
    eli_end();
    frame_end();

    /* Release then type "Hi": both characters land, edited flag fires. */
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, false);
    frame_begin();
    open_win();
    type_string("Hi");
    bool ret = eli_input_text("Name", buf, sizeof(buf), 0, NULL, NULL);
    ELI_ASSERT_TRUE(eli_is_item_edited());
    ELI_ASSERT_TRUE(ret);          /* default policy returns true on edit */
    eli_end();
    frame_end();

    ELI_ASSERT_STR_EQ(buf, "Hi");
    teardown(ctx);
}

ELI_TEST(input_text_backspace_deletes) {
    eli_context *ctx = setup();
    char buf[32] = "";

    eli_io_add_mouse_pos_event(20.0f, 40.0f);
    frame_begin();
    open_win();
    eli_input_text("Name", buf, sizeof(buf), 0, NULL, NULL);
    eli_vec2 c = rect_center(ctx->last_item_rect);
    eli_end();
    frame_end();

    eli_io_add_mouse_pos_event(c.x, c.y);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    frame_begin();
    open_win();
    eli_input_text("Name", buf, sizeof(buf), 0, NULL, NULL);
    eli_end();
    frame_end();

    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, false);
    frame_begin();
    open_win();
    type_string("Hi");
    eli_input_text("Name", buf, sizeof(buf), 0, NULL, NULL);
    eli_end();
    frame_end();
    ELI_ASSERT_STR_EQ(buf, "Hi");

    /* Backspace removes the last character. */
    eli_io_add_key_event(ELI_KEY_BACKSPACE, true);
    frame_begin();
    open_win();
    eli_input_text("Name", buf, sizeof(buf), 0, NULL, NULL);
    eli_end();
    frame_end();

    ELI_ASSERT_STR_EQ(buf, "H");
    teardown(ctx);
}

ELI_TEST(input_text_enter_returns_true_and_deactivates) {
    eli_context *ctx = setup();
    char buf[32] = "";

    eli_io_add_mouse_pos_event(20.0f, 40.0f);
    frame_begin();
    open_win();
    eli_input_text("Name", buf, sizeof(buf), ELI_INPUT_TEXT_ENTER_RETURNS_TRUE, NULL, NULL);
    eli_vec2 c = rect_center(ctx->last_item_rect);
    eli_end();
    frame_end();

    eli_io_add_mouse_pos_event(c.x, c.y);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    frame_begin();
    open_win();
    /* Editing but no Enter: does not return true under ENTER_RETURNS_TRUE. */
    type_string("Yo");
    bool r1 = eli_input_text("Name", buf, sizeof(buf), ELI_INPUT_TEXT_ENTER_RETURNS_TRUE, NULL, NULL);
    ELI_ASSERT_FALSE(r1);
    eli_end();
    frame_end();
    ELI_ASSERT_STR_EQ(buf, "Yo");

    /* Press Enter: returns true and the field deactivates. */
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, false);
    eli_io_add_key_event(ELI_KEY_ENTER, true);
    frame_begin();
    open_win();
    bool r2 = eli_input_text("Name", buf, sizeof(buf), ELI_INPUT_TEXT_ENTER_RETURNS_TRUE, NULL, NULL);
    ELI_ASSERT_TRUE(r2);
    ELI_ASSERT_TRUE(eli_is_item_deactivated());
    eli_end();
    frame_end();

    ELI_ASSERT_EQ(eli_get_active_id(), 0u);
    teardown(ctx);
}

ELI_TEST(input_text_with_hint_renders_when_empty) {
    eli_context *ctx = setup();
    char buf[32] = "";

    /* No hint: empty + inactive draws only the frame (no text glyphs). */
    frame_begin();
    open_win();
    eli_input_text("A", buf, sizeof(buf), 0, NULL, NULL);
    uint32_t no_hint_vtx = eli_get_window_draw_list()->vtx_count;
    eli_end();
    frame_end();

    /* With hint: the placeholder glyphs add vertices over the frame-only case. */
    frame_begin();
    open_win();
    eli_input_text_with_hint("B", "type here", buf, sizeof(buf), 0, NULL, NULL);
    uint32_t hint_vtx = eli_get_window_draw_list()->vtx_count;
    eli_end();
    frame_end();

    ELI_ASSERT_GT(hint_vtx, no_hint_vtx);
    teardown(ctx);
}

ELI_TEST(input_text_filter_helper_rejects_and_rewrites) {
    /* Control characters are rejected; uppercase mapping applies. */
    ELI_ASSERT_EQ(eli_input_text__filter((uint16_t)'\n', ELI_INPUT_TEXT_NONE), 0);
    ELI_ASSERT_EQ(eli_input_text__filter((uint16_t)'a', ELI_INPUT_TEXT_CHARS_UPPERCASE),
                  (uint16_t)'A');
    ELI_ASSERT_EQ(eli_input_text__filter((uint16_t)'z', ELI_INPUT_TEXT_CHARS_DECIMAL), 0);
    ELI_ASSERT_EQ(eli_input_text__filter((uint16_t)'5', ELI_INPUT_TEXT_CHARS_DECIMAL),
                  (uint16_t)'5');
}

ELI_TEST_MAIN()
