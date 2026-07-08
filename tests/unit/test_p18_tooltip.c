/**
 * @file test_p18_tooltip.c
 * @brief Unit tests for Phase 18 tooltips: begin/end tooltip emits text geometry
 *        into a tooltip window, set_tooltip does the same, the tooltip is placed
 *        near the cursor, and begin_item_tooltip appears only after the previous
 *        item has been hovered past the hover delay (advancing frames on
 *        io.delta_time). Driven across frames through the input backend.
 *
 * @status Phase 18 tooltip coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/widgets/eli_widgets.h>
#include <eli/widgets/eli_tooltip.h>
#include <eli/font/eli_font.h>

#define TEST_DT (1.0f / 60.0f)

static eli_font_atlas *g_atlas = NULL;

static eli_context *tip_setup(void)
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

static void tip_teardown(eli_context *ctx)
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

/* Host window W at (0,0)-(300,300). */
static void open_win(void)
{
    eli_set_next_window_pos(eli_make_vec2(0.0f, 0.0f), 0, eli_make_vec2(0.0f, 0.0f));
    eli_set_next_window_size(eli_make_vec2(300.0f, 300.0f), 0);
    eli_begin("W", NULL, 0);
}

static eli_window *find_tooltip(eli_context *ctx, int index)
{
    char name[32];
    snprintf(name, sizeof name, "##Tooltip_%02d", index);
    return eli_find_window_by_name(ctx, name);
}

static eli_vec2 rect_center(eli_rect r)
{
    return eli_make_vec2(r.x + r.w * 0.5f, r.y + r.h * 0.5f);
}

/* ------------------------------------------------------------------------- */

ELI_TEST(begin_tooltip_emits_text_geometry) {
    eli_context *ctx = tip_setup();
    eli_io_add_mouse_pos_event(400.0f, 400.0f);

    /* Two frames: the auto-resized tooltip window needs a warmup frame to size
     * itself from the measured text before the text is unclipped and drawn. */
    for (int frame = 0; frame < 2; frame++) {
        eli_io_add_mouse_pos_event(400.0f, 400.0f);
        frame_begin();
        open_win();
        bool shown = eli_begin_tooltip();
        ELI_ASSERT_TRUE(shown);
        eli_text("Hello tooltip");
        eli_end_tooltip();
        eli_end();
        frame_end();
    }

    eli_window *tip = find_tooltip(ctx, 0);
    ELI_ASSERT_NOT_NULL(tip);
    ELI_ASSERT_GT((int)tip->draw_list.vtx_count, 0);

    tip_teardown(ctx);
}

ELI_TEST(set_tooltip_emits_text_geometry) {
    eli_context *ctx = tip_setup();

    for (int frame = 0; frame < 2; frame++) {
        eli_io_add_mouse_pos_event(200.0f, 150.0f);
        frame_begin();
        open_win();
        eli_set_tooltip("Value: %d", 42);
        eli_end();
        frame_end();
    }

    eli_window *tip = find_tooltip(ctx, 0);
    ELI_ASSERT_NOT_NULL(tip);
    ELI_ASSERT_GT((int)tip->draw_list.vtx_count, 0);

    tip_teardown(ctx);
}

ELI_TEST(tooltip_positioned_near_mouse) {
    eli_context *ctx = tip_setup();
    float mx = 400.0f, my = 300.0f;

    for (int frame = 0; frame < 2; frame++) {
        eli_io_add_mouse_pos_event(mx, my);
        frame_begin();
        open_win();
        eli_set_tooltip("near cursor");
        eli_end();
        frame_end();
    }

    eli_window *tip = find_tooltip(ctx, 0);
    ELI_ASSERT_NOT_NULL(tip);
    /* Placed just off the cursor by the default mouse offset (16, 10). */
    ELI_ASSERT_FLT_NEAR(tip->pos.x, mx + 16.0f, 0.5f);
    ELI_ASSERT_FLT_NEAR(tip->pos.y, my + 10.0f, 0.5f);

    tip_teardown(ctx);
}

ELI_TEST(item_tooltip_waits_for_hover_delay) {
    eli_context *ctx = tip_setup();

    /* Frame 1: submit the button (no hover yet) to learn its rect. */
    eli_io_add_mouse_pos_event(5.0f, 5.0f);
    frame_begin();
    open_win();
    eli_button("Btn");
    ELI_ASSERT_FALSE(eli_begin_item_tooltip());
    eli_end();
    frame_end();
    eli_vec2 c = rect_center(ctx->last_item_rect);

    /* Frames 2-3: mouse now over the button but the delay has not elapsed. */
    for (int frame = 0; frame < 2; frame++) {
        eli_io_add_mouse_pos_event(c.x, c.y);
        frame_begin();
        open_win();
        eli_button("Btn");
        ELI_ASSERT_FALSE(eli_begin_item_tooltip());
        eli_end();
        frame_end();
    }

    /* Keep hovering with a stationary mouse until well past the delay. */
    bool shown = false;
    for (int frame = 0; frame < 40 && !shown; frame++) {
        eli_io_add_mouse_pos_event(c.x, c.y);
        frame_begin();
        open_win();
        eli_button("Btn");
        shown = eli_begin_item_tooltip();
        if (shown) {
            eli_text("tip");
            eli_end_tooltip();
        }
        eli_end();
        frame_end();
    }
    ELI_ASSERT_TRUE(shown);

    tip_teardown(ctx);
}

ELI_TEST(item_tooltip_not_shown_when_not_hovered) {
    eli_context *ctx = tip_setup();

    /* Mouse parked far away from the button for many stationary frames: the item
     * is never hovered, so the item tooltip never appears. */
    bool ever_shown = false;
    for (int frame = 0; frame < 40; frame++) {
        eli_io_add_mouse_pos_event(900.0f, 700.0f);
        frame_begin();
        open_win();
        eli_button("Btn");
        if (eli_begin_item_tooltip()) {
            ever_shown = true;
            eli_end_tooltip();
        }
        eli_end();
        frame_end();
    }
    ELI_ASSERT_FALSE(ever_shown);

    tip_teardown(ctx);
}

ELI_TEST_MAIN()
