/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file bench_window_registry.c
 * @brief Host benchmark for the window registry at many-window scale.
 *
 * For N top-level windows, each with one child pane (2N windows total), it
 * times:
 *   - lookup : eli_find_window_by_id over every window id (ns per lookup)
 *   - frame  : a full frame (eli_frame_begin, N begin/child/end, eli_frame_end)
 *              with the mouse over the window stack, so hovered-window
 *              detection, by-id lookup in eli_begin and render-order assembly
 *              all run at scale (us per frame)
 *
 * Standalone native program (not a unit test). Build and run:
 *
 *   mkdir -p tests/build && cc -std=c11 -O2 -DELI_TEST_HOSTED -Iinclude -Ivendor \
 *      tests/bench/bench_window_registry.c -o tests/build/bench_window_registry -lm && \
 *      tests/build/bench_window_registry
 *
 * @status Window-registry benchmark.
 * @issues None
 * @todo None
 */
#include <stdio.h>
#include <time.h>

#include <eli/elimgui.h>

/** @return Monotonic time in seconds. */
static double bench_now_seconds(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

static int g_bench_window_count = 0;

static void bench_registry_ui(void)
{
    char name[32];
    for (int i = 0; i < g_bench_window_count; i++) {
        snprintf(name, sizeof(name), "W%d", i);
        eli_set_next_window_pos(eli_make_vec2((float)(i % 50) * 12.0f, (float)(i / 50) * 9.0f),
                                ELI_COND_ONCE, eli_make_vec2(0.0f, 0.0f));
        eli_set_next_window_size(eli_make_vec2(160.0f, 120.0f), ELI_COND_ONCE);
        eli_begin(name, NULL, 0);
        if (eli_begin_child("pane", eli_make_vec2(80.0f, 50.0f), 0, 0))
            eli_text("%d", i);
        eli_end_child();
        eli_end();
    }
}

static void bench_registry_frame(void)
{
    eli_get_io()->delta_time = 1.0f / 60.0f;
    eli_frame_begin();
    bench_registry_ui();
    eli_frame_end();
}

static void bench_registry_at(int roots)
{
    eli_context *ctx = eli_create_context();
    ctx->io.display_size = eli_make_vec2(1280.0f, 800.0f);
    eli_font_atlas *atlas = eli_font_atlas_create();
    eli_font *font = eli_font_atlas_add_font_default(atlas, NULL);
    eli_font_atlas_build(atlas);
    eli_push_font(font);

    g_bench_window_count = roots;
    eli_io_add_mouse_pos_event(300.0f, 200.0f);
    for (int i = 0; i < 3; i++)
        bench_registry_frame();

    int windows = ctx->windows_count;
    int lookup_rounds = 2000000 / windows + 1;
    volatile uintptr_t sink = 0;
    double t0 = bench_now_seconds();
    for (int r = 0; r < lookup_rounds; r++)
        for (int i = 0; i < windows; i++)
            sink += (uintptr_t)eli_find_window_by_id(ctx, ctx->windows[i]->id);
    double t1 = bench_now_seconds();
    double ns_per_lookup = (t1 - t0) * 1e9 / ((double)lookup_rounds * windows);

    int frames = roots >= 1024 ? 20 : 200;
    double t2 = bench_now_seconds();
    for (int i = 0; i < frames; i++)
        bench_registry_frame();
    double t3 = bench_now_seconds();

    printf("windows=%5d  lookup %8.1f ns  frame %10.1f us\n", windows, ns_per_lookup,
           (t3 - t2) * 1e6 / frames);
    (void)sink;

    eli_destroy_context(ctx);
    eli_font_atlas_destroy(atlas);
}

int main(void)
{
    const int sizes[] = { 8, 64, 256, 1024, 2048 };
    for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); i++)
        bench_registry_at(sizes[i]);
    return 0;
}
