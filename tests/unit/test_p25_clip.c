/**
 * @file test_p25_clip.c
 * @brief Phase 25 clipping tests: eli_push_clip_rect (intersect) shrinks the
 *        window draw-list clip rect and mirrors it into the window clip rect so an
 *        item lying fully outside is culled by eli_item_add / eli_is_item_visible;
 *        eli_pop_clip_rect restores both and the same item becomes visible.
 *
 * @status Phase 25 clipping coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/interaction/eli_interaction.h>
#include <eli/layout/eli_layout.h>
#include <eli/widgets/eli_item_status.h>

#define TEST_DT (1.0f / 60.0f)

static eli_context *setup(void)
{
    eli_context *ctx = eli_create_context();
    ctx->io.delta_time = TEST_DT;
    ctx->io.display_size = eli_make_vec2(1024.0f, 768.0f);
    ctx->font_size = 13.0f;
    return ctx;
}

static void frame_begin(void)
{
    eli_new_frame();
    eli_input_update_begin_frame();
    eli_window_new_frame();
}

static void open_window(void)
{
    eli_set_next_window_pos(eli_make_vec2(100.0f, 100.0f), 0, eli_make_vec2(0.0f, 0.0f));
    eli_set_next_window_size(eli_make_vec2(400.0f, 300.0f), 0);
    eli_begin("ClipWin", NULL, 0);
}

ELI_TEST(push_clip_rect_shrinks_draw_list_clip) {
    eli_context *ctx = setup();
    frame_begin();
    open_window();

    eli_draw_list *dl = eli_get_window_draw_list();
    eli_vec2 max0 = eli_draw_list_get_clip_rect_max(dl);

    eli_push_clip_rect(eli_make_vec2(110.0f, 130.0f), eli_make_vec2(150.0f, 160.0f), true);
    eli_vec2 mn = eli_draw_list_get_clip_rect_min(dl);
    eli_vec2 mx = eli_draw_list_get_clip_rect_max(dl);
    ELI_ASSERT_FLT_NEAR(mn.x, 110.0f, 0.01f);
    ELI_ASSERT_FLT_NEAR(mn.y, 130.0f, 0.01f);
    ELI_ASSERT_FLT_NEAR(mx.x, 150.0f, 0.01f);
    ELI_ASSERT_FLT_NEAR(mx.y, 160.0f, 0.01f);
    ELI_ASSERT_LT(mx.x, max0.x);

    /* Window clip rect mirrors the draw-list clip. */
    ELI_ASSERT_FLT_NEAR(ctx->current_window->clip_rect.x, 110.0f, 0.01f);
    ELI_ASSERT_FLT_NEAR(ctx->current_window->clip_rect.w, 40.0f, 0.01f);

    eli_pop_clip_rect();
    eli_vec2 mxr = eli_draw_list_get_clip_rect_max(dl);
    ELI_ASSERT_FLT_NEAR(mxr.x, max0.x, 0.01f);

    eli_end();
    eli_destroy_context(ctx);
}

ELI_TEST(item_outside_clip_is_culled_then_restored) {
    eli_context *ctx = setup();
    frame_begin();
    open_window();

    eli_rect bb = eli_make_rect(300.0f, 300.0f, 10.0f, 10.0f);

    /* Inside the default (content) clip: visible. */
    ELI_ASSERT_TRUE(eli_item_add(0x100u, bb, 0));
    ELI_ASSERT_TRUE(eli_is_item_visible());

    /* Shrink the clip to a small region far from bb: culled. */
    eli_push_clip_rect(eli_make_vec2(110.0f, 130.0f), eli_make_vec2(150.0f, 160.0f), true);
    ELI_ASSERT_FALSE(eli_item_add(0x101u, bb, 0));
    ELI_ASSERT_FALSE(eli_is_item_visible());

    /* An item inside the shrunken clip is visible. */
    eli_rect inside = eli_make_rect(120.0f, 135.0f, 8.0f, 8.0f);
    ELI_ASSERT_TRUE(eli_item_add(0x102u, inside, 0));
    ELI_ASSERT_TRUE(eli_is_item_visible());

    /* Restore: the original item is visible again. */
    eli_pop_clip_rect();
    ELI_ASSERT_TRUE(eli_item_add(0x103u, bb, 0));
    ELI_ASSERT_TRUE(eli_is_item_visible());

    eli_end();
    eli_destroy_context(ctx);
}

ELI_TEST_MAIN()
