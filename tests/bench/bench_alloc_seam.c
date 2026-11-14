/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file bench_alloc_seam.c
 * @brief Counts how many of elimgui's heap calls a host allocator actually sees.
 *
 * malloc/calloc/realloc/free are redefined (before elimgui is included) as
 * counting wrappers, so every direct libc call from library code is tallied
 * as a "bypass". A counting allocator is installed through
 * eli_set_allocator_functions() to tally the calls that go through the seam.
 * The workload is a representative app: context + default font, 40 windows
 * with child panes, a table, a tab bar, a plot, a dock space with two docked
 * windows, overlay draw lists, and an ini save; then the context is destroyed.
 * Also reports steady-state seam allocations per frame after warm-up.
 *
 * Standalone native program (not a unit test). Build and run:
 *
 *   mkdir -p tests/build && cc -std=c11 -O2 -DELI_TEST_HOSTED -Iinclude -Ivendor \
 *      tests/bench/bench_alloc_seam.c -o tests/build/bench_alloc_seam -lm && \
 *      tests/build/bench_alloc_seam
 *
 * @status Allocation-seam coverage benchmark.
 * @issues None
 * @todo None
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------------------------------------------------------------------------
 * Counters (defined before the redirect so they reach the real libc)
 * ------------------------------------------------------------------------- */

static long g_seam_allocs = 0;
static long g_seam_frees = 0;
static long g_bypass_malloc = 0;
static long g_bypass_calloc = 0;
static long g_bypass_realloc = 0;
static long g_bypass_free = 0;

static void *bench_seam_alloc(size_t sz, void *user_data)
{
    (void)user_data;
    g_seam_allocs++;
    return malloc(sz);
}

static void bench_seam_free(void *ptr, void *user_data)
{
    (void)user_data;
    if (ptr != NULL)
        g_seam_frees++;
    free(ptr);
}

__attribute__((unused)) static void *bench_bypass_malloc(size_t n) { g_bypass_malloc++; return malloc(n); }
__attribute__((unused)) static void *bench_bypass_calloc(size_t n, size_t s) { g_bypass_calloc++; return calloc(n, s); }
__attribute__((unused)) static void *bench_bypass_realloc(void *p, size_t n) { g_bypass_realloc++; return realloc(p, n); }
__attribute__((unused)) static void bench_bypass_free(void *p) { if (p != NULL) g_bypass_free++; free(p); }

#define malloc(n)     bench_bypass_malloc(n)
#define calloc(n, s)  bench_bypass_calloc(n, s)
#define realloc(p, n) bench_bypass_realloc(p, n)
#define free(p)       bench_bypass_free(p)

#include <eli/elimgui.h>

/* ---------------------------------------------------------------------------
 * Workload
 * ------------------------------------------------------------------------- */

static void bench_ui(void)
{
    char name[32];
    for (int i = 0; i < 40; i++) {
        snprintf(name, sizeof(name), "Window %d", i);
        eli_set_next_window_pos(eli_make_vec2((float)(i * 7), (float)(i * 5)), ELI_COND_ONCE,
                                eli_make_vec2(0.0f, 0.0f));
        eli_set_next_window_size(eli_make_vec2(120.0f, 90.0f), ELI_COND_ONCE);
        eli_begin(name, NULL, 0);
        if (eli_begin_child("pane", eli_make_vec2(60.0f, 40.0f), 0, 0))
            eli_text("child %d", i);
        eli_end_child();
        eli_end();
    }

    eli_begin("Widgets", NULL, 0);
    if (eli_begin_table("Grid", 3, ELI_TABLE_SORTABLE | ELI_TABLE_BORDERS)) {
        for (int r = 0; r < 20; r++) {
            eli_table_next_row();
            for (int c = 0; c < 3; c++) {
                eli_table_next_column();
                eli_text("%d,%d", r, c);
            }
        }
        eli_end_table();
    }
    if (eli_begin_tab_bar("Bar", 0)) {
        for (int i = 0; i < 12; i++) {
            snprintf(name, sizeof(name), "Tab %d", i);
            if (eli_begin_tab_item(name, NULL, 0))
                eli_end_tab_item();
        }
        eli_end_tab_bar();
    }
    static float vals[300];
    for (int i = 0; i < 300; i++)
        vals[i] = (float)(i % 17);
    eli_plot_lines("##lines", vals, 300, 0, NULL, ELI_PLOT_SCALE_AUTO, ELI_PLOT_SCALE_AUTO,
                   eli_make_vec2(200.0f, 100.0f), 0);
    eli_end();

    eli_set_next_window_pos(eli_make_vec2(10.0f, 10.0f), 0, eli_make_vec2(0.0f, 0.0f));
    eli_set_next_window_size(eli_make_vec2(700.0f, 500.0f), 0);
    eli_begin("Host", NULL, ELI_WINDOW_NO_DOCKING);
    eli_dock_space(0x00A0D0C5u, eli_make_vec2(600.0f, 400.0f), 0);
    eli_end();
    eli_set_next_window_dock_id(0x00A0D0C5u, 0);
    eli_begin("Docked A", NULL, 0);
    eli_end();
    eli_set_next_window_dock_id(0x00A0D0C5u, 0);
    eli_begin("Docked B", NULL, 0);
    eli_end();

    eli_draw_list_add_rect_filled(eli_get_background_draw_list(), eli_make_vec2(0, 0),
                                  eli_make_vec2(10, 10), ELI_COL32_WHITE, 0.0f, ELI_DRAW_NONE);
    eli_draw_list_add_rect_filled(eli_get_foreground_draw_list(), eli_make_vec2(0, 0),
                                  eli_make_vec2(10, 10), ELI_COL32_WHITE, 0.0f, ELI_DRAW_NONE);
}

static void bench_frame(void)
{
    eli_get_io()->delta_time = 1.0f / 60.0f;
    eli_frame_begin();
    bench_ui();
    eli_frame_end();
}

static long bench_bypass_total(void)
{
    return g_bypass_malloc + g_bypass_calloc + g_bypass_realloc + g_bypass_free;
}

int main(void)
{
    eli_set_allocator_functions(bench_seam_alloc, bench_seam_free, NULL);

    eli_context *ctx = eli_create_context();
    ctx->io.display_size = eli_make_vec2(1280.0f, 800.0f);
    eli_font_atlas *atlas = eli_font_atlas_create();
    eli_font *font = eli_font_atlas_add_font_default(atlas, NULL);
    eli_font_atlas_build(atlas);
    eli_push_font(font);

    for (int i = 0; i < 5; i++)
        bench_frame();
    long warm_seam = g_seam_allocs, warm_bypass = bench_bypass_total();
    for (int i = 0; i < 100; i++)
        bench_frame();
    long steady_seam = g_seam_allocs - warm_seam;
    long steady_bypass = bench_bypass_total() - warm_bypass;

    size_t ini_len = 0;
    char *ini = eli_save_ini_settings_to_memory(&ini_len);
    eli_mem_free(ini);

    eli_destroy_context(ctx);
    eli_font_atlas_destroy(atlas);
    eli_set_allocator_functions(NULL, NULL, NULL);

    long seen = g_seam_allocs;
    long missed = g_bypass_malloc + g_bypass_calloc + g_bypass_realloc;
    printf("allocations seen by host allocator : %ld\n", seen);
    printf("allocations bypassing it           : %ld  (malloc %ld, calloc %ld, realloc %ld)\n",
           missed, g_bypass_malloc, g_bypass_calloc, g_bypass_realloc);
    printf("frees seen / bypassing             : %ld / %ld\n", g_seam_frees, g_bypass_free);
    printf("host allocator coverage            : %.1f%%\n",
           seen + missed > 0 ? 100.0 * (double)seen / (double)(seen + missed) : 100.0);
    printf("steady-state per frame (100 frames): seam %.2f, bypass %.2f\n",
           (double)steady_seam / 100.0, (double)steady_bypass / 100.0);
    return 0;
}
