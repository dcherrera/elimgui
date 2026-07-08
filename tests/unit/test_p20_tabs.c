/**
 * @file test_p20_tabs.c
 * @brief Unit tests for Phase 20 tab bars: default-first selection, exactly-one
 *        selected tab, click-to-switch selection across frames, the per-tab close
 *        button clearing *p_open, and eli_set_tab_item_closed removing a tab.
 *        Interaction is driven across frames by feeding mouse events through the
 *        input backend, mirroring the browser event flow.
 *
 * @status Phase 20 tab-bar coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/widgets/eli_widgets.h>
#include <eli/widgets/eli_tab.h>
#include <eli/font/eli_font.h>

#define TEST_DT (1.0f / 60.0f)

static eli_font_atlas *g_atlas = NULL;

static eli_context *tab_setup(void)
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

static void tab_teardown(eli_context *ctx)
{
    eli_tab_shutdown();
    eli_destroy_context(ctx);
    eli_font_atlas_destroy(g_atlas);
    g_atlas = NULL;
}

static void frame_begin(void)
{
    eli_new_frame();
    eli_input_update_begin_frame();
    eli_window_new_frame();
    eli_tab_new_frame();
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
    eli_set_next_window_size(eli_make_vec2(400.0f, 400.0f), 0);
    eli_begin("W", NULL, 0);
}

/* Submit a three-tab bar; record which of A/B/C returned selected. */
static int submit_three(bool out_selected[3])
{
    int selected_count = 0;
    if (eli_begin_tab_bar("Tabs", 0)) {
        const char *labels[3] = {"Alpha", "Bravo", "Charlie"};
        for (int i = 0; i < 3; i++) {
            bool sel = eli_begin_tab_item(labels[i], NULL, 0);
            out_selected[i] = sel;
            if (sel) {
                selected_count++;
                eli_end_tab_item();
            }
        }
        eli_end_tab_bar();
    }
    return selected_count;
}

/* ------------------------------------------------------------------------- */

ELI_TEST(default_selection_is_first_tab) {
    eli_context *ctx = tab_setup();
    bool sel[3] = {false, false, false};

    /* Run a couple frames so layout settles past the appearing frame. */
    for (int frame = 0; frame < 3; frame++) {
        frame_begin();
        open_win();
        submit_three(sel);
        eli_end();
        frame_end();
    }

    ELI_ASSERT_TRUE(sel[0]);   /* first tab selected by default */
    ELI_ASSERT_FALSE(sel[1]);
    ELI_ASSERT_FALSE(sel[2]);
    tab_teardown(ctx);
}

ELI_TEST(exactly_one_tab_selected) {
    eli_context *ctx = tab_setup();
    bool sel[3] = {false, false, false};
    int count = 0;

    for (int frame = 0; frame < 3; frame++) {
        frame_begin();
        open_win();
        count = submit_three(sel);
        eli_end();
        frame_end();
    }

    ELI_ASSERT_EQ(count, 1);   /* one and only one tab reports selected */
    tab_teardown(ctx);
}

ELI_TEST(click_switches_selection_next_frame) {
    eli_context *ctx = tab_setup();
    bool sel[3] = {false, false, false};
    eli_rect bravo_rect = eli_make_rect(0.0f, 0.0f, 0.0f, 0.0f);

    /* Settle a few frames, capturing Bravo's tab rect for the click target. */
    for (int frame = 0; frame < 3; frame++) {
        eli_io_add_mouse_pos_event(5.0f, 5.0f);
        frame_begin();
        open_win();
        if (eli_begin_tab_bar("Tabs", 0)) {
            const char *labels[3] = {"Alpha", "Bravo", "Charlie"};
            for (int i = 0; i < 3; i++) {
                bool s = eli_begin_tab_item(labels[i], NULL, 0);
                if (i == 1)
                    bravo_rect = ctx->last_item_rect;
                sel[i] = s;
                if (s)
                    eli_end_tab_item();
            }
            eli_end_tab_bar();
        }
        eli_end();
        frame_end();
    }
    ELI_ASSERT_TRUE(sel[0]);
    ELI_ASSERT_FALSE(sel[1]);

    eli_vec2 c = eli_make_vec2(bravo_rect.x + bravo_rect.w * 0.5f,
                               bravo_rect.y + bravo_rect.h * 0.5f);

    /* Click Bravo: press then release. Selection flips on the following frame. */
    eli_io_add_mouse_pos_event(c.x, c.y);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    frame_begin();
    open_win();
    submit_three(sel);
    eli_end();
    frame_end();

    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, false);
    frame_begin();
    open_win();
    submit_three(sel);
    eli_end();
    frame_end();

    /* Next frame the queued selection is applied. */
    frame_begin();
    open_win();
    submit_three(sel);
    eli_end();
    frame_end();

    ELI_ASSERT_FALSE(sel[0]);
    ELI_ASSERT_TRUE(sel[1]);
    ELI_ASSERT_FALSE(sel[2]);
    tab_teardown(ctx);
}

ELI_TEST(close_button_clears_p_open) {
    eli_context *ctx = tab_setup();
    bool open_b = true;
    eli_rect bravo_rect = eli_make_rect(0.0f, 0.0f, 0.0f, 0.0f);

    /* Settle so the tab renders (past the appearing frame) and grab its rect. */
    for (int frame = 0; frame < 3; frame++) {
        eli_io_add_mouse_pos_event(5.0f, 5.0f);
        frame_begin();
        open_win();
        if (eli_begin_tab_bar("Tabs", 0)) {
            if (eli_begin_tab_item("Alpha", NULL, 0))
                eli_end_tab_item();
            bool s = eli_begin_tab_item("Bravo", &open_b, 0);
            bravo_rect = ctx->last_item_rect;
            if (s)
                eli_end_tab_item();
            eli_end_tab_bar();
        }
        eli_end();
        frame_end();
    }
    ELI_ASSERT_TRUE(open_b);

    /* Close button sits at the right edge of the tab, one font-size square. */
    float fp_x = ctx->style.frame_padding.x;
    float fp_y = ctx->style.frame_padding.y;
    float bsz = eli_get_font_size();
    float close_x = eli_max_f(bravo_rect.x, bravo_rect.x + bravo_rect.w - fp_x - bsz) + bsz * 0.5f;
    float close_y = bravo_rect.y + fp_y + bsz * 0.5f;

    /* Press then release over the close button -> *p_open cleared. */
    eli_io_add_mouse_pos_event(close_x, close_y);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    frame_begin();
    open_win();
    if (eli_begin_tab_bar("Tabs", 0)) {
        if (eli_begin_tab_item("Alpha", NULL, 0))
            eli_end_tab_item();
        if (eli_begin_tab_item("Bravo", &open_b, 0))
            eli_end_tab_item();
        eli_end_tab_bar();
    }
    eli_end();
    frame_end();

    eli_io_add_mouse_pos_event(close_x, close_y);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, false);
    frame_begin();
    open_win();
    if (eli_begin_tab_bar("Tabs", 0)) {
        if (eli_begin_tab_item("Alpha", NULL, 0))
            eli_end_tab_item();
        if (eli_begin_tab_item("Bravo", &open_b, 0))
            eli_end_tab_item();
        eli_end_tab_bar();
    }
    eli_end();
    frame_end();

    ELI_ASSERT_FALSE(open_b);   /* the close button cleared the flag */
    tab_teardown(ctx);
}

ELI_TEST(set_tab_item_closed_removes_tab) {
    eli_context *ctx = tab_setup();
    bool sel[3] = {false, false, false};

    /* Warm up with three tabs. */
    for (int frame = 0; frame < 2; frame++) {
        frame_begin();
        open_win();
        submit_three(sel);
        eli_end();
        frame_end();
    }

    /* Confirm three tabs are pooled before closing. */
    frame_begin();
    open_win();
    int count_before = 0;
    if (eli_begin_tab_bar("Tabs", 0)) {
        submit_three(sel);
        count_before = eli_tab__current()->tab_count;
        eli_end_tab_bar();
    }
    eli_end();
    frame_end();
    ELI_ASSERT_EQ(count_before, 3);

    /* Close Charlie: call set_tab_item_closed and stop submitting it. The next
     * layout (this frame's, at the first begin_tab_item) garbage-collects it. */
    frame_begin();
    open_win();
    int count_after = 3;
    if (eli_begin_tab_bar("Tabs", 0)) {
        eli_set_tab_item_closed("Charlie");
        if (eli_begin_tab_item("Alpha", NULL, 0))
            eli_end_tab_item();
        if (eli_begin_tab_item("Bravo", NULL, 0))
            eli_end_tab_item();
        count_after = eli_tab__current()->tab_count;
        eli_end_tab_bar();
    }
    eli_end();
    frame_end();

    ELI_ASSERT_EQ(count_after, 2);   /* Charlie was removed by set_tab_item_closed */
    tab_teardown(ctx);
}

ELI_TEST_MAIN()
