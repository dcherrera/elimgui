/**
 * @file test_p21_drag_drop.c
 * @brief Unit tests for Phase 21 drag & drop: opening a source on a held+dragged
 *        item and setting a payload, peeking the active payload, delivering it to a
 *        matching target on release-over, rejecting a mismatched type, and clearing
 *        the payload once the drag ends. Driven across frames via the input backend.
 *
 * @status Phase 21 drag & drop coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/widgets/eli_widgets.h>
#include <eli/widgets/eli_drag_drop.h>
#include <eli/font/eli_font.h>

#define TEST_DT (1.0f / 60.0f)

static eli_font_atlas *g_atlas = NULL;

static eli_context *dd_setup(void)
{
    eli_context *ctx = eli_create_context();
    ctx->io.delta_time = TEST_DT;
    ctx->io.display_size = eli_make_vec2(1024.0f, 768.0f);
    g_atlas = eli_font_atlas_create();
    eli_font *f = eli_font_atlas_add_font_default(g_atlas, NULL);
    eli_font_atlas_build(g_atlas);
    eli_push_font(f);
    /* Reset the module-static drag state so tests do not leak into each other. */
    eli_clear_drag_drop();
    return ctx;
}

static void dd_teardown(eli_context *ctx)
{
    eli_clear_drag_drop();
    eli_destroy_context(ctx);
    eli_font_atlas_destroy(g_atlas);
    g_atlas = NULL;
}

static void frame_begin(void)
{
    eli_new_frame();
    eli_drag_drop_new_frame();
    eli_input_update_begin_frame();
    eli_window_new_frame();
}

static void frame_end(void)
{
    eli_window_render();
    eli_render();
    eli_drag_drop_end_frame();
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

/* Submit the source + target buttons; return true from the source scope. Fills the
 * payload with `value` when a drag is active. Records the two rects into the outs. */
static void submit_widgets(int value, const char *type,
                           eli_rect *out_src, eli_rect *out_dst,
                           bool *out_source_open)
{
    eli_context *ctx = eli_get_current_context();
    eli_button("SRC");
    if (out_src) *out_src = ctx->last_item_rect;
    bool open = eli_begin_drag_drop_source(ELI_DRAG_DROP_NONE);
    if (open) {
        eli_set_drag_drop_payload(type, &value, sizeof(value), ELI_COND_ALWAYS);
        eli_end_drag_drop_source();
    }
    if (out_source_open) *out_source_open = open;

    eli_button("DST");
    if (out_dst) *out_dst = ctx->last_item_rect;
}

/* ------------------------------------------------------------------------- */

ELI_TEST(source_sets_payload_and_get_returns_it) {
    eli_context *ctx = dd_setup();
    eli_rect src = {0}, dst = {0};

    /* Frame 1: establish rects. */
    eli_io_add_mouse_pos_event(5.0f, 5.0f);
    frame_begin();
    open_win();
    submit_widgets(0, "INT", &src, &dst, NULL);
    eli_end();
    frame_end();
    eli_vec2 sc = rect_center(src);
    eli_vec2 dc = rect_center(dst);

    /* Frame 2: press on SRC -> becomes the active item. */
    eli_io_add_mouse_pos_event(sc.x, sc.y);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    frame_begin();
    open_win();
    submit_widgets(0, "INT", NULL, NULL, NULL);
    ELI_ASSERT_TRUE(eli_is_item_active() == false || eli_get_active_id() != 0u);
    eli_end();
    frame_end();

    /* Frame 3: hold + drag to DST -> source opens, payload set. */
    int payload_value = 4242;
    bool source_open = false;
    eli_io_add_mouse_pos_event(dc.x, dc.y);
    frame_begin();
    open_win();
    submit_widgets(payload_value, "INT", NULL, NULL, &source_open);
    ELI_ASSERT_TRUE(source_open);

    const eli_payload *p = eli_get_drag_drop_payload();
    ELI_ASSERT_NOT_NULL(p);
    ELI_ASSERT_STR_EQ(p->data_type, "INT");
    ELI_ASSERT_EQ(p->data_size, (int)sizeof(int));
    ELI_ASSERT_NOT_NULL(p->data);
    ELI_ASSERT_EQ(*(const int *)p->data, payload_value);
    eli_end();
    frame_end();

    dd_teardown(ctx);
}

ELI_TEST(matching_target_accepts_on_release) {
    eli_context *ctx = dd_setup();
    eli_rect src = {0}, dst = {0};
    int payload_value = 7;

    /* Frame 1: rects. */
    eli_io_add_mouse_pos_event(5.0f, 5.0f);
    frame_begin();
    open_win();
    submit_widgets(0, "INT", &src, &dst, NULL);
    eli_end();
    frame_end();
    eli_vec2 sc = rect_center(src);
    eli_vec2 dc = rect_center(dst);

    /* Frame 2: press SRC. */
    eli_io_add_mouse_pos_event(sc.x, sc.y);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    frame_begin();
    open_win();
    submit_widgets(0, "INT", NULL, NULL, NULL);
    eli_end();
    frame_end();

    /* Frame 3: drag over DST (still held) -> target hovers, not yet delivered. */
    eli_io_add_mouse_pos_event(dc.x, dc.y);
    frame_begin();
    open_win();
    {
        bool open = false;
        eli_button("SRC");
        open = eli_begin_drag_drop_source(ELI_DRAG_DROP_NONE);
        if (open) {
            eli_set_drag_drop_payload("INT", &payload_value, sizeof(payload_value),
                                      ELI_COND_ALWAYS);
            eli_end_drag_drop_source();
        }
        ELI_ASSERT_TRUE(open);
        eli_button("DST");
        ELI_ASSERT_TRUE(eli_begin_drag_drop_target());
        const eli_payload *hover = eli_accept_drag_drop_payload("INT", ELI_DRAG_DROP_NONE);
        ELI_ASSERT_NULL(hover); /* not delivered while the button is still down */
        eli_end_drag_drop_target();
    }
    eli_end();
    frame_end();

    /* Frame 4: release over DST -> delivery, payload returned with data. */
    eli_io_add_mouse_pos_event(dc.x, dc.y);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, false);
    frame_begin();
    open_win();
    {
        eli_button("SRC");
        (void)eli_begin_drag_drop_source(ELI_DRAG_DROP_NONE);
        eli_button("DST");
        ELI_ASSERT_TRUE(eli_begin_drag_drop_target());
        const eli_payload *drop = eli_accept_drag_drop_payload("INT", ELI_DRAG_DROP_NONE);
        ELI_ASSERT_NOT_NULL(drop);
        ELI_ASSERT_TRUE(drop->delivery);
        ELI_ASSERT_EQ(drop->data_size, (int)sizeof(int));
        ELI_ASSERT_EQ(*(const int *)drop->data, payload_value);
        eli_end_drag_drop_target();
    }
    eli_end();
    frame_end();

    /* After delivery the drag is cleared. */
    ELI_ASSERT_NULL(eli_get_drag_drop_payload());

    dd_teardown(ctx);
}

ELI_TEST(mismatched_type_is_not_accepted) {
    eli_context *ctx = dd_setup();
    eli_rect src = {0}, dst = {0};
    int payload_value = 99;

    eli_io_add_mouse_pos_event(5.0f, 5.0f);
    frame_begin();
    open_win();
    submit_widgets(0, "INT", &src, &dst, NULL);
    eli_end();
    frame_end();
    eli_vec2 sc = rect_center(src);
    eli_vec2 dc = rect_center(dst);

    eli_io_add_mouse_pos_event(sc.x, sc.y);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    frame_begin();
    open_win();
    submit_widgets(0, "INT", NULL, NULL, NULL);
    eli_end();
    frame_end();

    /* Frame 3: hover DST with a "INT" payload but ask for "STR". */
    eli_io_add_mouse_pos_event(dc.x, dc.y);
    frame_begin();
    open_win();
    {
        eli_button("SRC");
        bool open = eli_begin_drag_drop_source(ELI_DRAG_DROP_NONE);
        if (open) {
            eli_set_drag_drop_payload("INT", &payload_value, sizeof(payload_value),
                                      ELI_COND_ALWAYS);
            eli_end_drag_drop_source();
        }
        eli_button("DST");
        ELI_ASSERT_TRUE(eli_begin_drag_drop_target());
        ELI_ASSERT_NULL(eli_accept_drag_drop_payload("STR", ELI_DRAG_DROP_NONE));
        eli_end_drag_drop_target();
    }
    eli_end();
    frame_end();

    /* Frame 4: release still asking for the wrong type -> never delivered. */
    eli_io_add_mouse_pos_event(dc.x, dc.y);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, false);
    frame_begin();
    open_win();
    {
        eli_button("SRC");
        (void)eli_begin_drag_drop_source(ELI_DRAG_DROP_NONE);
        eli_button("DST");
        ELI_ASSERT_TRUE(eli_begin_drag_drop_target());
        ELI_ASSERT_NULL(eli_accept_drag_drop_payload("STR", ELI_DRAG_DROP_NONE));
        eli_end_drag_drop_target();
    }
    eli_end();
    frame_end();

    dd_teardown(ctx);
}

ELI_TEST(payload_cleared_after_drag_ends) {
    eli_context *ctx = dd_setup();
    eli_rect src = {0}, dst = {0};

    eli_io_add_mouse_pos_event(5.0f, 5.0f);
    frame_begin();
    open_win();
    submit_widgets(0, "INT", &src, &dst, NULL);
    eli_end();
    frame_end();
    eli_vec2 sc = rect_center(src);

    eli_io_add_mouse_pos_event(sc.x, sc.y);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    frame_begin();
    open_win();
    submit_widgets(0, "INT", NULL, NULL, NULL);
    eli_end();
    frame_end();

    /* Frame 3: drag -> payload active. */
    int value = 55;
    bool open = false;
    eli_io_add_mouse_pos_event(sc.x + 80.0f, sc.y + 80.0f);
    frame_begin();
    open_win();
    submit_widgets(value, "INT", NULL, NULL, &open);
    ELI_ASSERT_TRUE(open);
    ELI_ASSERT_NOT_NULL(eli_get_drag_drop_payload());
    eli_end();
    frame_end();

    /* Frame 4: release with no target. The payload was last refreshed while
     * dragging, so it survives this frame (mirrors ImGui's one-frame grace). */
    eli_io_add_mouse_pos_event(sc.x + 80.0f, sc.y + 80.0f);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, false);
    frame_begin();
    open_win();
    submit_widgets(value, "INT", NULL, NULL, &open);
    ELI_ASSERT_FALSE(open); /* released: no longer a live source */
    eli_end();
    frame_end();

    /* Frame 5: no source refresh + button up -> drag expires, payload cleared. */
    eli_io_add_mouse_pos_event(sc.x + 80.0f, sc.y + 80.0f);
    frame_begin();
    open_win();
    submit_widgets(value, "INT", NULL, NULL, &open);
    eli_end();
    frame_end();

    ELI_ASSERT_NULL(eli_get_drag_drop_payload());

    dd_teardown(ctx);
}

ELI_TEST_MAIN()
