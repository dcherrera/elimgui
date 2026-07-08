/**
 * @file test_p14_combo.c
 * @brief Unit tests for Phase 14 combos: eli_combo opens its dropdown popup on a
 *        click and updates the current index (returning true) when a popup item is
 *        chosen, and the preview reflects the current item. Interaction is driven
 *        across frames through the input backend.
 *
 * @status Phase 14 combo coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/widgets/eli_widgets.h>
#include <eli/widgets/eli_combo.h>
#include <eli/font/eli_font.h>

#define TEST_DT (1.0f / 60.0f)

static eli_font_atlas *g_atlas = NULL;
static const char *const g_items[] = {"Apple", "Banana", "Cherry"};

static eli_context *combo_setup(void)
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

static void combo_teardown(eli_context *ctx)
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

/* ------------------------------------------------------------------------- */

ELI_TEST(combo_opens_and_selects) {
    eli_context *ctx = combo_setup();
    int current = 0;

    /* Frame 1: popup closed -> combo returns false and the last item is the combo
     * frame itself (no list emitted). Capture a point inside the preview frame. */
    eli_io_add_mouse_pos_event(900.0f, 900.0f);
    frame_begin();
    open_win();
    ELI_ASSERT_FALSE(eli_combo("C", &current, g_items, 3, -1));
    eli_end();
    frame_end();
    eli_rect frame_rect = ctx->last_item_rect;
    eli_vec2 frame_click = eli_make_vec2(frame_rect.x + 4.0f, frame_rect.y + frame_rect.h * 0.5f);
    ELI_ASSERT_EQ(g_eli_open_popup_count, 0);

    /* Frame 2: press the preview frame (down edge, not yet released). */
    eli_io_add_mouse_pos_event(frame_click.x, frame_click.y);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    frame_begin();
    open_win();
    ELI_ASSERT_FALSE(eli_combo("C", &current, g_items, 3, -1));
    eli_end();
    frame_end();

    /* Frame 3: release over the preview -> the dropdown popup opens. */
    eli_io_add_mouse_pos_event(frame_click.x, frame_click.y);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, false);
    frame_begin();
    open_win();
    ELI_ASSERT_FALSE(eli_combo("C", &current, g_items, 3, -1));
    eli_end();
    frame_end();
    ELI_ASSERT_GT(g_eli_open_popup_count, 0);

    /* Frame 4 (settle): popup stays open; the last item emitted is index 2, so its
     * rect is the last-item rect after eli_combo returns. */
    eli_io_add_mouse_pos_event(900.0f, 900.0f);
    frame_begin();
    open_win();
    ELI_ASSERT_FALSE(eli_combo("C", &current, g_items, 3, -1));
    eli_end();
    frame_end();
    ELI_ASSERT_GT(g_eli_open_popup_count, 0);
    eli_vec2 item2 = rect_center(ctx->last_item_rect);

    /* Frame 5: press item 2 in the popup (down edge). */
    eli_io_add_mouse_pos_event(item2.x, item2.y);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    frame_begin();
    open_win();
    ELI_ASSERT_FALSE(eli_combo("C", &current, g_items, 3, -1));
    eli_end();
    frame_end();

    /* Frame 6: release over item 2 -> selection changes to 2, combo returns true,
     * and the popup auto-closes. */
    eli_io_add_mouse_pos_event(item2.x, item2.y);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, false);
    frame_begin();
    open_win();
    bool changed = eli_combo("C", &current, g_items, 3, -1);
    eli_end();
    frame_end();
    ELI_ASSERT_TRUE(changed);
    ELI_ASSERT_EQ(current, 2);
    ELI_ASSERT_EQ(g_eli_open_popup_count, 0);

    combo_teardown(ctx);
}

ELI_TEST(combo_preview_reflects_current_item) {
    eli_context *ctx = combo_setup();
    int current = 0;

    /* cur = 0 -> preview "Apple". Measure the vertex delta of the combo call. */
    eli_io_add_mouse_pos_event(900.0f, 900.0f);
    frame_begin();
    open_win();
    eli_draw_list *dl = eli_get_window_draw_list();
    uint32_t before = dl->vtx_count;
    eli_combo("C", &current, g_items, 3, -1);
    uint32_t delta_apple = dl->vtx_count - before;
    eli_end();
    frame_end();

    /* cur = 2 -> preview "Cherry" (a different glyph count) yields a different
     * vertex delta, proving the preview tracks the current item. */
    current = 2;
    frame_begin();
    open_win();
    dl = eli_get_window_draw_list();
    before = dl->vtx_count;
    eli_combo("C", &current, g_items, 3, -1);
    uint32_t delta_cherry = dl->vtx_count - before;
    eli_end();
    frame_end();

    ELI_ASSERT_NE(delta_apple, delta_cherry);

    combo_teardown(ctx);
}

ELI_TEST(begin_combo_toggles_popup) {
    eli_context *ctx = combo_setup();

    /* Frame 1: closed dropdown -> begin_combo returns false. Capture frame click. */
    eli_io_add_mouse_pos_event(900.0f, 900.0f);
    frame_begin();
    open_win();
    ELI_ASSERT_FALSE(eli_begin_combo("Combo", "Preview", ELI_COMBO_NONE));
    eli_end();
    frame_end();
    eli_rect fr = ctx->last_item_rect;
    eli_vec2 click = eli_make_vec2(fr.x + 4.0f, fr.y + fr.h * 0.5f);

    /* Frame 2: press; Frame 3: release -> opens. */
    eli_io_add_mouse_pos_event(click.x, click.y);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    frame_begin();
    open_win();
    if (eli_begin_combo("Combo", "Preview", ELI_COMBO_NONE))
        eli_end_combo();
    eli_end();
    frame_end();

    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, false);
    frame_begin();
    open_win();
    bool opened = eli_begin_combo("Combo", "Preview", ELI_COMBO_NONE);
    if (opened) {
        eli_selectable("One", false, 0, eli_make_vec2(0.0f, 0.0f));
        eli_end_combo();
    }
    eli_end();
    frame_end();
    ELI_ASSERT_TRUE(opened);

    combo_teardown(ctx);
}

ELI_TEST_MAIN()
