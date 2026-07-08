/**
 * @file test_p26_list_clipper.c
 * @brief Unit tests for the Phase 26 list clipper: visible-range computation for a
 *        large fixed-height list inside a short window, scroll-driven range shifting,
 *        full content-height advancement (correct scrollbar), forced include ranges,
 *        and seek_cursor_for_item positioning.
 *
 * @status Phase 26 list clipper coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/util/eli_list_clipper.h>
#include <eli/window/eli_window.h>
#include <eli/layout/eli_layout_helpers.h>

#define TEST_DT     (1.0f / 60.0f)
#define ITEM_COUNT  10000
#define ITEM_HEIGHT 20.0f

static eli_context *clipper_setup(void)
{
    eli_context *ctx = eli_create_context();
    ctx->io.delta_time = TEST_DT;
    ctx->io.display_size = eli_make_vec2(1024.0f, 768.0f);
    return ctx;
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

/* A short 200x150 window whose content size is left to be measured from layout, so
 * the clipper drives the scroll range. */
static void open_list_window(void)
{
    eli_set_next_window_pos(eli_make_vec2(100.0f, 120.0f), 0, eli_make_vec2(0.0f, 0.0f));
    eli_set_next_window_size(eli_make_vec2(200.0f, 150.0f), 0);
    eli_begin("List", NULL, 0);
}

/* Run a fixed-height clipper, emitting a dummy item per rendered index so the cursor
 * advances like real widgets would. Returns the union [out_start, out_end) across all
 * steps and the number of items actually emitted. */
static int run_clipper_union(eli_list_clipper *c, int *out_start, int *out_end)
{
    int total_start = ITEM_COUNT;
    int total_end = 0;
    int emitted = 0;
    eli_list_clipper_begin(c, ITEM_COUNT, ITEM_HEIGHT);
    while (eli_list_clipper_step(c)) {
        if (c->display_start < total_start)
            total_start = c->display_start;
        if (c->display_end > total_end)
            total_end = c->display_end;
        for (int i = c->display_start; i < c->display_end; i++) {
            eli_dummy(eli_make_vec2(0.0f, ITEM_HEIGHT));
            emitted++;
        }
    }
    eli_list_clipper_end(c);
    *out_start = total_start;
    *out_end = total_end;
    return emitted;
}

/* Visible range for a short window is a tiny window into a 10000-item list, and it
 * brackets the window's clip region. */
ELI_TEST(clipper_visible_range_brackets_viewport) {
    eli_context *ctx = clipper_setup();

    frame_begin();
    open_list_window();
    eli_window *w = eli_get_current_window();
    float start_pos_y = w->cursor_pos.y;
    eli_rect clip = w->clip_rect;

    eli_list_clipper c;
    int ds, de;
    run_clipper_union(&c, &ds, &de);

    /* Far smaller than the full list. */
    ELI_ASSERT_GE(ds, 0);
    ELI_ASSERT_LE(de, ITEM_COUNT);
    ELI_ASSERT_LT(de - ds, 100);
    ELI_ASSERT_GT(de - ds, 0);

    /* First visible item starts at or above the clip top (within one item), and the
     * last covers the clip bottom: the range brackets the visible region. */
    float first_top = start_pos_y + (float)ds * ITEM_HEIGHT;
    float last_bottom = start_pos_y + (float)de * ITEM_HEIGHT;
    ELI_ASSERT_LE(first_top, clip.y + ITEM_HEIGHT);
    ELI_ASSERT_GE(last_bottom, clip.y + clip.h);

    eli_end();
    frame_end();
    eli_destroy_context(ctx);
}

/* After the clipper finishes, the layout cursor has advanced past all 10000 items so
 * the measured content height (and hence the scrollbar) reflects the whole list. */
ELI_TEST(clipper_advances_full_content_height) {
    eli_context *ctx = clipper_setup();

    frame_begin();
    open_list_window();
    eli_window *w = eli_get_current_window();

    eli_list_clipper c;
    int ds, de;
    run_clipper_union(&c, &ds, &de);

    eli_end();
    frame_end();

    /* content_size_measured is set in eli_end from the layout cursor extent. Full
     * height is ITEM_COUNT*ITEM_HEIGHT minus one trailing item spacing (4px). */
    float expected = (float)ITEM_COUNT * ITEM_HEIGHT;
    ELI_ASSERT_FLT_NEAR(w->content_size_measured.y, expected, 10.0f);

    eli_destroy_context(ctx);
}

/* Scrolling the window moves the visible index range down the list. */
ELI_TEST(clipper_scroll_shifts_visible_range) {
    eli_context *ctx = clipper_setup();

    /* Frame 1: run the clipper so the window measures its full content height. */
    frame_begin();
    open_list_window();
    eli_list_clipper c1;
    int ds1, de1;
    run_clipper_union(&c1, &ds1, &de1);
    eli_end();
    frame_end();

    /* Frame 2: apply the scroll at Begin (so the cursor origin reflects it), then
     * re-clip. The scroll range is real now that frame 1 measured the content. */
    frame_begin();
    eli_set_next_window_pos(eli_make_vec2(100.0f, 120.0f), 0, eli_make_vec2(0.0f, 0.0f));
    eli_set_next_window_size(eli_make_vec2(200.0f, 150.0f), 0);
    eli_set_next_window_scroll(eli_make_vec2(-1.0f, 2000.0f));
    eli_begin("List", NULL, 0);
    ELI_ASSERT_FLT_NEAR(eli_get_scroll_y(), 2000.0f, 1.0f);

    eli_list_clipper c2;
    int ds2, de2;
    run_clipper_union(&c2, &ds2, &de2);
    eli_end();
    frame_end();

    /* Top of frame 1 shows item 0; after scrolling 2000px (~100 items) the visible
     * range starts far down the list. */
    ELI_ASSERT_LT(ds1, 5);
    ELI_ASSERT_GT(ds2, 80);
    ELI_ASSERT_LT(de2 - ds2, 100);

    eli_destroy_context(ctx);
}

/* include_item_by_index forces an out-of-view index into a rendered range. */
ELI_TEST(clipper_include_forces_offscreen_item) {
    eli_context *ctx = clipper_setup();
    const int forced = 5000;

    frame_begin();
    open_list_window();

    eli_list_clipper c;
    eli_list_clipper_begin(&c, ITEM_COUNT, ITEM_HEIGHT);
    eli_list_clipper_include_item_by_index(&c, forced);

    bool saw_forced = false;
    bool saw_visible = false;
    while (eli_list_clipper_step(&c)) {
        if (forced >= c.display_start && forced < c.display_end)
            saw_forced = true;
        if (c.display_start == 0)
            saw_visible = true;
        for (int i = c.display_start; i < c.display_end; i++)
            eli_dummy(eli_make_vec2(0.0f, ITEM_HEIGHT));
    }
    eli_list_clipper_end(&c);

    ELI_ASSERT_TRUE(saw_forced);   /* offscreen item was rendered */
    ELI_ASSERT_TRUE(saw_visible);  /* the normal visible range still rendered too */

    eli_end();
    frame_end();
    eli_destroy_context(ctx);
}

/* include_items_by_index forces a contiguous offscreen block into rendered ranges. */
ELI_TEST(clipper_include_range_forces_block) {
    eli_context *ctx = clipper_setup();
    const int block_begin = 3000;
    const int block_end = 3004;

    frame_begin();
    open_list_window();

    eli_list_clipper c;
    eli_list_clipper_begin(&c, ITEM_COUNT, ITEM_HEIGHT);
    eli_list_clipper_include_items_by_index(&c, block_begin, block_end);

    int forced_seen = 0;
    while (eli_list_clipper_step(&c)) {
        for (int i = c.display_start; i < c.display_end; i++) {
            if (i >= block_begin && i < block_end)
                forced_seen++;
            eli_dummy(eli_make_vec2(0.0f, ITEM_HEIGHT));
        }
    }
    eli_list_clipper_end(&c);

    ELI_ASSERT_EQ(forced_seen, block_end - block_begin);

    eli_end();
    frame_end();
    eli_destroy_context(ctx);
}

/* seek_cursor_for_item positions the layout cursor at the exact item top. */
ELI_TEST(clipper_seek_cursor_positions_exactly) {
    eli_context *ctx = clipper_setup();

    frame_begin();
    open_list_window();
    eli_window *w = eli_get_current_window();

    eli_list_clipper c;
    eli_list_clipper_begin(&c, ITEM_COUNT, ITEM_HEIGHT);
    float start_pos_y = c.start_pos_y;

    eli_list_clipper_seek_cursor_for_item(&c, 250);
    float expected = start_pos_y + 250.0f * ITEM_HEIGHT;
    ELI_ASSERT_FLT_NEAR(w->cursor_pos.y, expected, 0.5f);

    /* Prev-line geometry is set up so scroll-here style queries keep working. */
    ELI_ASSERT_FLT_NEAR(w->cursor_pos_prev_line.y, expected - ITEM_HEIGHT, 0.5f);

    eli_list_clipper_seek_cursor_for_item(&c, 0);
    ELI_ASSERT_FLT_NEAR(w->cursor_pos.y, start_pos_y, 0.5f);

    eli_list_clipper_end(&c);
    eli_end();
    frame_end();
    eli_destroy_context(ctx);
}

/* Measured item height: with items_height <= 0, the first step submits one item, the
 * clipper infers the height, and the remaining range clips correctly. */
ELI_TEST(clipper_measures_item_height) {
    eli_context *ctx = clipper_setup();

    frame_begin();
    open_list_window();

    eli_list_clipper c;
    eli_list_clipper_begin(&c, ITEM_COUNT, -1.0f);

    int step = 0;
    int total_emitted = 0;
    int max_index = -1;
    while (eli_list_clipper_step(&c)) {
        if (step == 0) {
            /* First step submits exactly item 0 for measurement. */
            ELI_ASSERT_EQ(c.display_start, 0);
            ELI_ASSERT_EQ(c.display_end, 1);
        }
        for (int i = c.display_start; i < c.display_end; i++) {
            eli_dummy(eli_make_vec2(0.0f, ITEM_HEIGHT));
            total_emitted++;
            if (i > max_index)
                max_index = i;
        }
        step++;
    }
    eli_list_clipper_end(&c);

    /* Height inferred from the measured first item: rendered item height plus the
     * one item spacing eli_dummy adds (the clipper measures the full row pitch). */
    float expected_h = ITEM_HEIGHT + ctx->style.item_spacing.y;
    ELI_ASSERT_FLT_NEAR(c.items_height, expected_h, 0.5f);
    /* Only a small visible slice rendered, not the whole list. */
    ELI_ASSERT_LT(total_emitted, 100);
    ELI_ASSERT_LT(max_index, 100);

    eli_end();
    frame_end();
    eli_destroy_context(ctx);
}

/* Zero-item list renders nothing. */
ELI_TEST(clipper_zero_items_renders_nothing) {
    eli_context *ctx = clipper_setup();

    frame_begin();
    open_list_window();

    eli_list_clipper c;
    eli_list_clipper_begin(&c, 0, ITEM_HEIGHT);
    int steps = 0;
    while (eli_list_clipper_step(&c))
        steps++;
    eli_list_clipper_end(&c);

    ELI_ASSERT_EQ(steps, 0);

    eli_end();
    frame_end();
    eli_destroy_context(ctx);
}

ELI_TEST_MAIN()
