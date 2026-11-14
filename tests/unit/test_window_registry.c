/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file test_window_registry.c
 * @brief Window registry behavior: by-id lookup, render order, focus index,
 *        hovered window, and allocation failure in window creation.
 *
 * The render-order and focus-index checks compare the library against
 * reference implementations kept here (the straightforward nested-loop /
 * linear-scan versions), across frames where windows appear, disappear, nest,
 * and change focus.
 *
 * @status Window registry tests.
 * @issues None
 * @todo None
 */
#include "eli_test.h"
#include "eli_test_ui.h"

/* ---------------------------------------------------------------------------
 * Reference implementations
 * ------------------------------------------------------------------------- */

/** Linear-scan focus index (the cached index must always agree). */
static int ref_focus_index(const eli_context *ctx, const eli_window *win)
{
    for (int i = 0; i < ctx->windows_focus_order_count; i++)
        if (ctx->windows_focus_order[i] == win)
            return i;
    return -1;
}

/** Nested-loop render order: roots by focus order, each root's descendants in pool order. */
static int ref_render_order(const eli_context *ctx, const eli_draw_list **out, int cap)
{
    int n = 0;
    for (int i = 0; i < ctx->windows_focus_order_count; i++) {
        eli_window *root = ctx->windows_focus_order[i];
        if (root->last_frame_active != ctx->frame_count)
            continue;
        if (n < cap)
            out[n] = &root->draw_list;
        n++;
        for (int j = 0; j < ctx->windows_count; j++) {
            eli_window *w = ctx->windows[j];
            if (w != root && w->last_frame_active == ctx->frame_count &&
                (w->root_window ? w->root_window : w) == root) {
                if (n < cap)
                    out[n] = &w->draw_list;
                n++;
            }
        }
    }
    return n;
}

/** The window draw lists the library emitted (background/foreground skipped). */
static int lib_render_order(const eli_context *ctx, const eli_draw_list **out, int cap)
{
    int n = 0;
    for (int i = 0; i < ctx->render_draw_lists_count; i++) {
        const eli_draw_list *dl = ctx->render_draw_lists[i];
        if (dl == ctx->background_draw_list || dl == ctx->foreground_draw_list)
            continue;
        if (n < cap)
            out[n] = dl;
        n++;
    }
    return n;
}

/* ---------------------------------------------------------------------------
 * Scenario: roots with children and grandchildren that come and go
 * ------------------------------------------------------------------------- */

#define REG_ROOTS 24

static unsigned g_reg_frame = 0;

/** Deterministic per-frame submission pattern. */
static bool reg_submits(int root, unsigned frame)
{
    return ((root * 7u + frame * 3u) % 5u) != 0u;
}

static void ui_registry_scenario(void)
{
    char name[32];
    for (int i = 0; i < REG_ROOTS; i++) {
        if (!reg_submits(i, g_reg_frame))
            continue;
        snprintf(name, sizeof(name), "Root %d", i);
        eli_set_next_window_pos(eli_make_vec2((float)(i % 6) * 110.0f, (float)(i / 6) * 120.0f),
                                ELI_COND_ONCE, eli_make_vec2(0.0f, 0.0f));
        eli_set_next_window_size(eli_make_vec2(150.0f, 140.0f), ELI_COND_ONCE);
        eli_window_flags flags = (i % 9 == 4) ? ELI_WINDOW_NO_BRING_TO_FRONT_ON_FOCUS : 0;
        eli_begin(name, NULL, flags);
        if (eli_begin_child("a", eli_make_vec2(100.0f, 80.0f), 0, 0)) {
            if (i % 2 == 0) {
                if (eli_begin_child("a-inner", eli_make_vec2(60.0f, 40.0f), 0, 0))
                    eli_text("grandchild");
                eli_end_child();
            }
        }
        eli_end_child();
        if ((g_reg_frame + (unsigned)i) % 3u != 0u) {
            if (eli_begin_child("b", eli_make_vec2(40.0f, 20.0f), 0, 0))
                eli_text("b");
            eli_end_child();
        }
        eli_end();
    }
}

/* ---------------------------------------------------------------------------
 * Tests
 * ------------------------------------------------------------------------- */

ELI_TEST(registry_find_by_id_matches_every_pooled_window)
{
    eli_context *ctx = test_ui_setup();
    test_ui_frames(ui_registry_scenario, 4);
    ELI_ASSERT_GT(ctx->windows_count, REG_ROOTS);
    for (int i = 0; i < ctx->windows_count; i++) {
        eli_window *w = ctx->windows[i];
        ELI_ASSERT_TRUE(eli_find_window_by_id(ctx, w->id) == w);
    }
    ELI_ASSERT_TRUE(eli_find_window_by_name(ctx, "Root 3") == eli_find_window_by_id(ctx, eli_hash_str("Root 3", 0)));
    ELI_ASSERT_NOT_NULL(eli_find_window_by_name(ctx, "Root 3"));
    ELI_ASSERT_NULL(eli_find_window_by_name(ctx, "never submitted"));
    ELI_ASSERT_NULL(eli_find_window_by_id(ctx, 0x12345678u));
    test_ui_teardown(ctx);
}

ELI_TEST(registry_lookup_is_fresh_after_context_recreate)
{
    eli_context *ctx = test_ui_setup();
    test_ui_frames(ui_registry_scenario, 2);
    test_ui_teardown(ctx);
    ctx = test_ui_setup();
    ELI_ASSERT_EQ(ctx->windows_count, 0);
    ELI_ASSERT_NULL(eli_find_window_by_name(ctx, "Root 3"));
    test_ui_teardown(ctx);
}

ELI_TEST(registry_render_order_matches_nested_loop)
{
    enum { CAP = 256 };
    const eli_draw_list *want[CAP], *got[CAP];
    eli_context *ctx = test_ui_setup();
    int compared_frames = 0;
    for (g_reg_frame = 0; g_reg_frame < 40; g_reg_frame++) {
        /* Click a different root every few frames to shuffle the z-order. */
        if (g_reg_frame % 4 == 1) {
            float x = (float)((g_reg_frame * 37u) % 600u) + 20.0f;
            float y = (float)((g_reg_frame * 53u) % 400u) + 20.0f;
            eli_io_add_mouse_pos_event(x, y);
            eli_io_add_mouse_button_event(0, true);
        } else if (g_reg_frame % 4 == 2) {
            eli_io_add_mouse_button_event(0, false);
        }
        test_ui_frame(ui_registry_scenario);
        int n_want = ref_render_order(ctx, want, CAP);
        int n_got = lib_render_order(ctx, got, CAP);
        ELI_ASSERT_EQ(n_got, n_want);
        ELI_ASSERT_TRUE(n_want <= CAP);
        ELI_ASSERT_GT(n_want, REG_ROOTS / 2);
        for (int i = 0; i < n_want && i < CAP; i++)
            ELI_ASSERT_TRUE(got[i] == want[i]);
        compared_frames++;
    }
    ELI_ASSERT_EQ(compared_frames, 40);
    test_ui_teardown(ctx);
}

ELI_TEST(registry_render_is_idempotent_within_a_frame)
{
    enum { CAP = 256 };
    const eli_draw_list *first[CAP], *second[CAP];
    eli_context *ctx = test_ui_setup();
    g_reg_frame = 7;
    test_ui_frames(ui_registry_scenario, 3);
    int n1 = lib_render_order(ctx, first, CAP);
    eli_window_render();
    int n2 = lib_render_order(ctx, second, CAP);
    ELI_ASSERT_EQ(n1, n2);
    for (int i = 0; i < n1 && i < CAP; i++)
        ELI_ASSERT_TRUE(first[i] == second[i]);
    test_ui_teardown(ctx);
}

ELI_TEST(registry_focus_index_cache_matches_linear_scan)
{
    eli_context *ctx = test_ui_setup();
    for (g_reg_frame = 0; g_reg_frame < 30; g_reg_frame++) {
        test_ui_frame(ui_registry_scenario);
        eli_window *target = ctx->windows[(g_reg_frame * 11u) % (unsigned)ctx->windows_count];
        eli_window_focus(ctx, target);
        for (int i = 0; i < ctx->windows_count; i++) {
            eli_window *w = ctx->windows[i];
            ELI_ASSERT_EQ(eli_window_focus_index(ctx, w), ref_focus_index(ctx, w));
        }
    }
    test_ui_teardown(ctx);
}

static void ui_two_overlapping(void)
{
    eli_set_next_window_pos(eli_make_vec2(50.0f, 50.0f), ELI_COND_ONCE, eli_make_vec2(0.0f, 0.0f));
    eli_set_next_window_size(eli_make_vec2(200.0f, 200.0f), ELI_COND_ONCE);
    eli_begin("Lower", NULL, 0);
    eli_end();
    eli_set_next_window_pos(eli_make_vec2(150.0f, 150.0f), ELI_COND_ONCE, eli_make_vec2(0.0f, 0.0f));
    eli_set_next_window_size(eli_make_vec2(200.0f, 200.0f), ELI_COND_ONCE);
    eli_begin("Upper", NULL, 0);
    eli_end();
}

ELI_TEST(registry_hovered_window_is_topmost_by_focus_order)
{
    eli_context *ctx = test_ui_setup();
    test_ui_frames(ui_two_overlapping, 2);
    eli_io_add_mouse_pos_event(180.0f, 180.0f);   /* inside both */
    test_ui_frames(ui_two_overlapping, 2);
    ELI_ASSERT_TRUE(ctx->hovered_window == eli_find_window_by_name(ctx, "Upper"));
    test_ui_click(ui_two_overlapping, eli_make_vec2(60.0f, 60.0f));   /* raise Lower */
    eli_io_add_mouse_pos_event(180.0f, 180.0f);
    test_ui_frames(ui_two_overlapping, 2);
    ELI_ASSERT_TRUE(ctx->hovered_window == eli_find_window_by_name(ctx, "Lower"));
    test_ui_teardown(ctx);
}

/* Allocator that fails every request once its budget of successes runs out
 * (budget < 0: unlimited). */
static int g_reg_alloc_budget = -1;

static void *reg_failing_alloc(size_t sz, void *user_data)
{
    (void)user_data;
    if (g_reg_alloc_budget == 0)
        return NULL;
    if (g_reg_alloc_budget > 0)
        g_reg_alloc_budget--;
    return malloc(sz);
}

static void reg_failing_free(void *ptr, void *user_data)
{
    (void)user_data;
    free(ptr);
}

static void ui_eight_windows(void)
{
    char name[16];
    for (int i = 0; i < 8; i++) {
        snprintf(name, sizeof(name), "Fill %d", i);
        eli_begin(name, NULL, 0);
        eli_end();
    }
}

static bool g_reg_oom_window_shown = false;

static void ui_eight_plus_new(void)
{
    ui_eight_windows();
    g_reg_oom_window_shown = eli_begin("Born under OOM", NULL, 0);
    if (g_reg_oom_window_shown)
        eli_end();
}

/*
 * Creating a window allocates the window, its name, then (when full) grows the
 * pool and the id map. With exactly 8 windows both the pool and the map are at
 * capacity, so a budget of 0..3 successes fails each step in turn; every
 * failure must leave the registry unchanged and the window absent.
 */
ELI_TEST(registry_window_creation_oom_leaves_registry_unchanged)
{
    for (int budget = 0; budget <= 3; budget++) {
        g_reg_alloc_budget = -1;
        eli_set_allocator_functions(reg_failing_alloc, reg_failing_free, NULL);
        eli_context *ctx = test_ui_setup();
        test_ui_frames(ui_eight_windows, 2);
        ELI_ASSERT_EQ(ctx->windows_count, 8);
        ELI_ASSERT_EQ(ctx->windows_capacity, 8);
        ELI_ASSERT_EQ(ctx->windows_by_id.size, ctx->windows_by_id.capacity);

        g_reg_alloc_budget = budget;
        test_ui_frame(ui_eight_plus_new);
        g_reg_alloc_budget = -1;

        ELI_ASSERT_FALSE(g_reg_oom_window_shown);
        ELI_ASSERT_EQ(ctx->windows_count, 8);
        ELI_ASSERT_EQ(ctx->windows_by_id.size, 8);
        ELI_ASSERT_NULL(eli_find_window_by_name(ctx, "Born under OOM"));

        test_ui_frame(ui_eight_plus_new);   /* allocator healthy again */
        ELI_ASSERT_TRUE(g_reg_oom_window_shown);
        ELI_ASSERT_EQ(ctx->windows_count, 9);
        ELI_ASSERT_NOT_NULL(eli_find_window_by_name(ctx, "Born under OOM"));
        test_ui_teardown(ctx);
        eli_set_allocator_functions(NULL, NULL, NULL);
    }
}

ELI_TEST_MAIN()
