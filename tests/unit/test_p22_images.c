/**
 * @file test_p22_images.c
 * @brief Unit tests for Phase 22 images:
 *   - eli_draw_list_add_image emits 4 vtx / 6 idx and binds the texture cmd
 *   - eli_draw_list_add_image_quad uses the 4 provided corner positions
 *   - eli_draw_list_add_image_rounded produces a convex fill (> 4 vtx, > 6 idx)
 *   - eli_image advances the cursor by the image size and matches eli_get_item_rect_size
 *   - eli_image_button returns true on mouse click-release over the button area
 *
 * @status Phase 22 image test coverage.
 * @issues None
 * @todo None
 */
#include "eli_test.h"

#include <eli/widgets/eli_image.h>
#include <eli/font/eli_font.h>

#define TEST_DT (1.0f / 60.0f)

/* ---------------------------------------------------------------------------
 * Frame-context helpers (shared by widget-level tests)
 * ------------------------------------------------------------------------- */

static eli_font_atlas *g_atlas = NULL;

static eli_context *img_setup(void)
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

static void img_teardown(eli_context *ctx)
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

/** Open a fixed-position window for widget tests. */
static void open_win(void)
{
    eli_set_next_window_pos(eli_make_vec2(0.0f, 0.0f), 0, eli_make_vec2(0.0f, 0.0f));
    eli_set_next_window_size(eli_make_vec2(400.0f, 400.0f), 0);
    eli_begin("ImgTestWin", NULL, 0);
}

/* ---------------------------------------------------------------------------
 * Draw-list level: add_image
 * ------------------------------------------------------------------------- */

ELI_TEST(add_image_vtx_idx_and_cmd_texture) {
    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);

    eli_vec2 p_min = {10.0f, 20.0f};
    eli_vec2 p_max = {110.0f, 120.0f};
    eli_vec2 uv_min = {0.25f, 0.10f};
    eli_vec2 uv_max = {0.75f, 0.90f};

    eli_draw_list_add_image(&dl, 42u, p_min, p_max, uv_min, uv_max, ELI_COL32_WHITE);

    /* Exactly 4 vertices and 6 indices. */
    ELI_ASSERT_EQ(dl.vtx_count, 4u);
    ELI_ASSERT_EQ(dl.idx_count, 6u);

    /* The first command must be bound to the requested texture. */
    ELI_ASSERT_GT(dl.cmd_count, 0u);
    ELI_ASSERT_EQ(dl.cmds[0].texture_id, 42u);
    ELI_ASSERT_EQ(dl.cmds[0].elem_count, 6u);

    /* Vertex UVs: TL, TR, BR, BL */
    ELI_ASSERT_FLT_NEAR(dl.vtx[0].u, 0.25f, 1e-4f);
    ELI_ASSERT_FLT_NEAR(dl.vtx[0].v, 0.10f, 1e-4f);
    ELI_ASSERT_FLT_NEAR(dl.vtx[1].u, 0.75f, 1e-4f);
    ELI_ASSERT_FLT_NEAR(dl.vtx[1].v, 0.10f, 1e-4f);
    ELI_ASSERT_FLT_NEAR(dl.vtx[2].u, 0.75f, 1e-4f);
    ELI_ASSERT_FLT_NEAR(dl.vtx[2].v, 0.90f, 1e-4f);
    ELI_ASSERT_FLT_NEAR(dl.vtx[3].u, 0.25f, 1e-4f);
    ELI_ASSERT_FLT_NEAR(dl.vtx[3].v, 0.90f, 1e-4f);

    /* All vertices have the passed tint color. */
    ELI_ASSERT_EQ(dl.vtx[0].col, ELI_COL32_WHITE);
    ELI_ASSERT_EQ(dl.vtx[3].col, ELI_COL32_WHITE);

    eli_draw_list_clear(&dl);
}

ELI_TEST(add_image_transparent_is_noop) {
    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);

    /* Fully-transparent tint should produce no geometry. */
    eli_draw_list_add_image(&dl, 7u, (eli_vec2){0, 0}, (eli_vec2){100, 100},
                            (eli_vec2){0, 0}, (eli_vec2){1, 1}, 0x00000000u);
    ELI_ASSERT_EQ(dl.vtx_count, 0u);
    ELI_ASSERT_EQ(dl.idx_count, 0u);

    eli_draw_list_clear(&dl);
}

ELI_TEST(add_image_two_different_textures_two_cmds) {
    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);

    eli_draw_list_add_image(&dl, 10u, (eli_vec2){0, 0}, (eli_vec2){50, 50},
                            (eli_vec2){0, 0}, (eli_vec2){1, 1}, ELI_COL32_WHITE);
    eli_draw_list_add_image(&dl, 20u, (eli_vec2){60, 0}, (eli_vec2){110, 50},
                            (eli_vec2){0, 0}, (eli_vec2){1, 1}, ELI_COL32_WHITE);

    ELI_ASSERT_EQ(dl.vtx_count, 8u);
    ELI_ASSERT_EQ(dl.idx_count, 12u);
    /* At least two distinct commands — one per texture. */
    ELI_ASSERT_GE(dl.cmd_count, 2u);
    ELI_ASSERT_EQ(dl.cmds[0].texture_id, 10u);
    ELI_ASSERT_EQ(dl.cmds[1].texture_id, 20u);

    eli_draw_list_clear(&dl);
}

/* ---------------------------------------------------------------------------
 * Draw-list level: add_image_quad
 * ------------------------------------------------------------------------- */

ELI_TEST(add_image_quad_corner_positions) {
    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);

    eli_vec2 p1 = {5.0f,  5.0f};
    eli_vec2 p2 = {95.0f, 10.0f};   /* slightly non-rectangular */
    eli_vec2 p3 = {90.0f, 90.0f};
    eli_vec2 p4 = {8.0f,  85.0f};

    eli_draw_list_add_image_quad(&dl, 55u, p1, p2, p3, p4,
                                 (eli_vec2){0, 0}, (eli_vec2){1, 0},
                                 (eli_vec2){1, 1}, (eli_vec2){0, 1},
                                 ELI_COL32_WHITE);

    ELI_ASSERT_EQ(dl.vtx_count, 4u);
    ELI_ASSERT_EQ(dl.idx_count, 6u);
    ELI_ASSERT_EQ(dl.cmds[0].texture_id, 55u);

    /* Vertex positions must match the supplied corners exactly. */
    ELI_ASSERT_FLT_NEAR(dl.vtx[0].x,  5.0f, 1e-4f);
    ELI_ASSERT_FLT_NEAR(dl.vtx[0].y,  5.0f, 1e-4f);
    ELI_ASSERT_FLT_NEAR(dl.vtx[1].x, 95.0f, 1e-4f);
    ELI_ASSERT_FLT_NEAR(dl.vtx[1].y, 10.0f, 1e-4f);
    ELI_ASSERT_FLT_NEAR(dl.vtx[2].x, 90.0f, 1e-4f);
    ELI_ASSERT_FLT_NEAR(dl.vtx[2].y, 90.0f, 1e-4f);
    ELI_ASSERT_FLT_NEAR(dl.vtx[3].x,  8.0f, 1e-4f);
    ELI_ASSERT_FLT_NEAR(dl.vtx[3].y, 85.0f, 1e-4f);

    eli_draw_list_clear(&dl);
}

/* ---------------------------------------------------------------------------
 * Draw-list level: add_image_rounded
 * ------------------------------------------------------------------------- */

ELI_TEST(add_image_rounded_convex_fill) {
    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);

    /* A 100×100 rect with 10px corner rounding should produce far more than 4
     * vertices (the path arc segments add many points per corner). */
    eli_draw_list_add_image_rounded(&dl, 99u,
                                    (eli_vec2){0, 0}, (eli_vec2){100, 100},
                                    (eli_vec2){0, 0}, (eli_vec2){1, 1},
                                    ELI_COL32_WHITE, 10.0f,
                                    ELI_DRAW_ROUND_CORNERS_ALL);

    ELI_ASSERT_GT(dl.vtx_count, 4u);
    ELI_ASSERT_GT(dl.idx_count, 6u);
    ELI_ASSERT_EQ(dl.cmds[0].texture_id, 99u);

    /* UV remapping: the top-left vertex (first path point) should map close
     * to (0,0) and the bottom-right (near corner) should map close to (1,1). */
    for (uint32_t i = 0; i < dl.vtx_count; i++) {
        ELI_ASSERT_GE(dl.vtx[i].u, 0.0f);
        ELI_ASSERT_LE(dl.vtx[i].u, 1.0f);
        ELI_ASSERT_GE(dl.vtx[i].v, 0.0f);
        ELI_ASSERT_LE(dl.vtx[i].v, 1.0f);
    }

    eli_draw_list_clear(&dl);
}

ELI_TEST(add_image_rounded_fallback_to_rect) {
    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);

    /* With rounding < 0.5 the function falls back to a plain rect. */
    eli_draw_list_add_image_rounded(&dl, 3u,
                                    (eli_vec2){0, 0}, (eli_vec2){64, 64},
                                    (eli_vec2){0, 0}, (eli_vec2){1, 1},
                                    ELI_COL32_WHITE, 0.0f,
                                    ELI_DRAW_ROUND_CORNERS_ALL);

    ELI_ASSERT_EQ(dl.vtx_count, 4u);
    ELI_ASSERT_EQ(dl.idx_count, 6u);

    eli_draw_list_clear(&dl);
}

/* ---------------------------------------------------------------------------
 * Widget level: eli_image cursor advance
 * ------------------------------------------------------------------------- */

ELI_TEST(eli_image_item_rect_size) {
    eli_context *ctx = img_setup();

    frame_begin();
    open_win();

    eli_vec2 img_sz = {64.0f, 48.0f};
    eli_image(1u, img_sz,
              (eli_vec2){0.0f, 0.0f}, (eli_vec2){1.0f, 1.0f},
              (eli_vec4){1.0f, 1.0f, 1.0f, 1.0f},   /* tint: white */
              (eli_vec4){0.0f, 0.0f, 0.0f, 0.0f});   /* no border */

    eli_vec2 rect_sz = eli_get_item_rect_size();
    ELI_ASSERT_FLT_NEAR(rect_sz.x, 64.0f, 1e-3f);
    ELI_ASSERT_FLT_NEAR(rect_sz.y, 48.0f, 1e-3f);

    eli_end();
    frame_end();

    img_teardown(ctx);
}

ELI_TEST(eli_image_with_border_adds_padding) {
    eli_context *ctx = img_setup();

    frame_begin();
    open_win();

    eli_vec2 img_sz = {32.0f, 32.0f};
    eli_image(2u, img_sz,
              (eli_vec2){0.0f, 0.0f}, (eli_vec2){1.0f, 1.0f},
              (eli_vec4){1.0f, 1.0f, 1.0f, 1.0f},
              (eli_vec4){1.0f, 0.0f, 0.0f, 1.0f});   /* red border */

    /* With a 1-pixel border on all sides, rect should be image_size + 2. */
    eli_vec2 rect_sz = eli_get_item_rect_size();
    ELI_ASSERT_FLT_NEAR(rect_sz.x, 34.0f, 1e-3f);
    ELI_ASSERT_FLT_NEAR(rect_sz.y, 34.0f, 1e-3f);

    eli_end();
    frame_end();

    img_teardown(ctx);
}

/* ---------------------------------------------------------------------------
 * Widget level: eli_image_button click detection
 * ------------------------------------------------------------------------- */

ELI_TEST(eli_image_button_returns_true_on_click_release) {
    eli_context *ctx = img_setup();

    /* Run one "warm-up" frame so the window and context are fully initialized. */
    frame_begin();
    open_win();
    eli_image_button("btn##warmup", 5u,
                     (eli_vec2){64.0f, 64.0f},
                     (eli_vec2){0.0f, 0.0f}, (eli_vec2){1.0f, 1.0f},
                     (eli_vec4){0.0f, 0.0f, 0.0f, 0.0f},
                     (eli_vec4){1.0f, 1.0f, 1.0f, 1.0f});
    eli_end();
    frame_end();

    /* Frame 1: press mouse inside the button.
     * The button sits at the window's cursor_start_pos. With default dark
     * style (window padding 8, title bar ~19 px) the button top-left is
     * approximately (8, 27). Use a safe interior point. */
    eli_io_add_mouse_pos_event(44.0f, 62.0f);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    frame_begin();
    open_win();
    bool pressed_f1 = eli_image_button("btn##test", 5u,
                                        (eli_vec2){64.0f, 64.0f},
                                        (eli_vec2){0.0f, 0.0f}, (eli_vec2){1.0f, 1.0f},
                                        (eli_vec4){0.0f, 0.0f, 0.0f, 0.0f},
                                        (eli_vec4){1.0f, 1.0f, 1.0f, 1.0f});
    eli_end();
    frame_end();
    /* Press-on-release policy: should NOT fire yet. */
    ELI_ASSERT_FALSE(pressed_f1);

    /* Frame 2: release mouse — button fires. */
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, false);
    frame_begin();
    open_win();
    bool pressed_f2 = eli_image_button("btn##test", 5u,
                                        (eli_vec2){64.0f, 64.0f},
                                        (eli_vec2){0.0f, 0.0f}, (eli_vec2){1.0f, 1.0f},
                                        (eli_vec4){0.0f, 0.0f, 0.0f, 0.0f},
                                        (eli_vec4){1.0f, 1.0f, 1.0f, 1.0f});
    eli_end();
    frame_end();
    ELI_ASSERT_TRUE(pressed_f2);

    img_teardown(ctx);
}

ELI_TEST(eli_image_button_no_fire_on_release_outside) {
    eli_context *ctx = img_setup();

    /* Warm-up. */
    frame_begin();
    open_win();
    eli_image_button("btn2##wm", 6u,
                     (eli_vec2){32.0f, 32.0f},
                     (eli_vec2){0, 0}, (eli_vec2){1, 1},
                     (eli_vec4){0, 0, 0, 0},
                     (eli_vec4){1, 1, 1, 1});
    eli_end();
    frame_end();

    /* Press inside. */
    eli_io_add_mouse_pos_event(30.0f, 50.0f);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, true);
    frame_begin();
    open_win();
    eli_image_button("btn2##test", 6u,
                     (eli_vec2){32.0f, 32.0f},
                     (eli_vec2){0, 0}, (eli_vec2){1, 1},
                     (eli_vec4){0, 0, 0, 0},
                     (eli_vec4){1, 1, 1, 1});
    eli_end();
    frame_end();

    /* Move mouse far outside, then release — should NOT fire. */
    eli_io_add_mouse_pos_event(900.0f, 900.0f);
    eli_io_add_mouse_button_event(ELI_MOUSE_BUTTON_LEFT, false);
    frame_begin();
    open_win();
    bool pressed = eli_image_button("btn2##test", 6u,
                                    (eli_vec2){32.0f, 32.0f},
                                    (eli_vec2){0, 0}, (eli_vec2){1, 1},
                                    (eli_vec4){0, 0, 0, 0},
                                    (eli_vec4){1, 1, 1, 1});
    eli_end();
    frame_end();
    ELI_ASSERT_FALSE(pressed);

    img_teardown(ctx);
}

ELI_TEST_MAIN()
