/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file bench_hotpath.c
 * @brief Host + WASM benchmark for the C-core hot paths (word wrap, arcs).
 *
 * Workloads, each a plain function so the same file serves two builds:
 *
 *   wrap-frame   - full frames with ten 250-char eli_text_wrapped paragraphs
 *                  (the demo About-tab shape) in a 400 px window.
 *   wrap-long    - full frames with one ~1000-char wrapped paragraph.
 *   wrap-drawlist- eli_draw_list_add_text_ex with wrap_width over 4 KB of text.
 *   round-rect   - 1000 filled + 1000 stroked rects, rounding 5 (arc_to_fast).
 *   circles      - 1000 auto-segment bullets (r 3) + 1000 16-segment radios (r 7).
 *
 * Native: main() times each workload with CLOCK_MONOTONIC (best of 5) and prints
 * us/iter plus glyph lookups/iter (ELI_FONT_GLYPH_LOOKUP_HOOK).
 *   cc -std=c11 -O2 -DELI_TEST_HOSTED -Iinclude -Ivendor tests/bench/bench_hotpath.c \
 *      -o tests/build/bench_hotpath -lm && tests/build/bench_hotpath
 * WASM (JAClibc software trig, the browser path): tests/bench/bench_hotpath.mjs
 * builds this file with clang --target=wasm32 and times the exported bench_*
 * functions from Node.
 *
 * Each bench_* returns a checksum (vertex/index totals) so the work is not elided.
 *
 * @status In use (numbers in docs/optimization.md).
 * @issues None
 * @todo None
 */
#ifdef __wasm__
#define NDEBUG
#endif

/* Count glyph lookups (a no-op hook unless defined before the library include). */
static long g_bench_glyph_lookups;
#define ELI_FONT_GLYPH_LOOKUP_HOOK() (g_bench_glyph_lookups++)

#include <eli/elimgui.h>
#include <eli/font/eli_font.h>

#ifdef __wasm__
#include "../../examples/demo/eli_wasm_runtime.h"
#define BENCH_EXPORT __attribute__((visibility("default")))
#else
#include <stdio.h>
#include <time.h>
#define BENCH_EXPORT
#endif

/* ---------------------------------------------------------------------------
 * Shared state
 * ------------------------------------------------------------------------- */

static eli_context *g_bench_ctx;
static eli_font_atlas *g_bench_atlas;
static eli_draw_list g_bench_list;

static const char g_bench_paragraph[] =
    "This window is built every frame from plain C calls: no retained widget tree, "
    "no callbacks. Each widget reads and writes your variables directly, and the "
    "library turns the calls into a vertex buffer that any renderer can draw.";

/** Fill `out` with `len` bytes of wrapped-paragraph text (NUL-terminated). */
static void bench_fill_text(char *out, int len)
{
    int n = (int)sizeof(g_bench_paragraph) - 1;
    for (int i = 0; i < len; i++)
        out[i] = g_bench_paragraph[i % n];
    out[len] = '\0';
}

static char g_bench_long_text[1000];
static char g_bench_drawlist_text[4097];

/** Create the context, font atlas and scratch draw list once. */
BENCH_EXPORT void bench_setup(void)
{
    g_bench_ctx = eli_create_context();
    eli_style_colors_dark(&g_bench_ctx->style);
    g_bench_ctx->io.delta_time = 1.0f / 60.0f;
    g_bench_ctx->io.display_size = eli_make_vec2(1280.0f, 800.0f);
    g_bench_atlas = eli_font_atlas_create();
    eli_font_atlas_add_font_default(g_bench_atlas, NULL);
    eli_font_atlas_build(g_bench_atlas);
    eli_font_atlas_set_tex_id(g_bench_atlas, 1);
    eli_push_font(g_bench_atlas->fonts[0]);
    g_bench_ctx->io.fonts = g_bench_atlas;
    g_bench_ctx->io.font_default = g_bench_atlas->fonts[0];
    eli_draw_list_init(&g_bench_list, ELI_DRAW_LIST_NONE);
    bench_fill_text(g_bench_long_text, (int)sizeof(g_bench_long_text) - 1);
    bench_fill_text(g_bench_drawlist_text, (int)sizeof(g_bench_drawlist_text) - 1);
}

/** @return Total vertices of the frame just ended. */
static int bench_frame_vertices(void)
{
    return eli_get_draw_data()->total_vtx_count;
}

/* ---------------------------------------------------------------------------
 * Workloads
 * ------------------------------------------------------------------------- */

/** Frames with ten demo-sized wrapped paragraphs. */
BENCH_EXPORT int bench_wrap_frame(int iterations)
{
    int checksum = 0;
    for (int i = 0; i < iterations; i++) {
        eli_frame_begin();
        eli_set_next_window_pos(eli_make_vec2(10.0f, 10.0f), 0, eli_make_vec2(0, 0));
        eli_set_next_window_size(eli_make_vec2(400.0f, 780.0f), 0);
        if (eli_begin("Wrap", NULL, 0)) {
            for (int p = 0; p < 10; p++)
                eli_text_wrapped("%s", g_bench_paragraph);
        }
        eli_end();
        eli_frame_end();
        checksum += bench_frame_vertices();
    }
    return checksum;
}

/** Frames with one ~1000-char wrapped paragraph. */
BENCH_EXPORT int bench_wrap_long(int iterations)
{
    int checksum = 0;
    for (int i = 0; i < iterations; i++) {
        eli_frame_begin();
        eli_set_next_window_pos(eli_make_vec2(10.0f, 10.0f), 0, eli_make_vec2(0, 0));
        eli_set_next_window_size(eli_make_vec2(400.0f, 780.0f), 0);
        if (eli_begin("Wrap", NULL, 0))
            eli_text_wrapped("%s", g_bench_long_text);
        eli_end();
        eli_frame_end();
        checksum += bench_frame_vertices();
    }
    return checksum;
}

/** add_text_ex over 4 KB of text with a 300 px wrap width. */
BENCH_EXPORT int bench_wrap_drawlist(int iterations)
{
    eli_font *font = g_bench_atlas->fonts[0];
    int checksum = 0;
    for (int i = 0; i < iterations; i++) {
        eli_draw_list_reset(&g_bench_list);
        eli_draw_list_add_text_ex(&g_bench_list, font, font->font_size, eli_make_vec2(4, 4),
                                  ELI_COL32_WHITE, g_bench_drawlist_text, NULL, 300.0f, NULL);
        checksum += (int)g_bench_list.vtx_count;
    }
    return checksum;
}

/** 1000 filled + 1000 stroked rounded rects (four arc_to_fast corners each). */
BENCH_EXPORT int bench_round_rect(int iterations)
{
    int checksum = 0;
    for (int i = 0; i < iterations; i++) {
        eli_draw_list_reset(&g_bench_list);
        for (int r = 0; r < 1000; r++) {
            float x = (float)(r % 50) * 20.0f, y = (float)(r / 50) * 20.0f;
            eli_vec2 a = eli_make_vec2(x, y), b = eli_make_vec2(x + 18.0f, y + 16.0f);
            eli_draw_list_add_rect_filled(&g_bench_list, a, b, ELI_COL32_WHITE, 5.0f,
                                          ELI_DRAW_NONE);
            eli_draw_list_add_rect(&g_bench_list, a, b, ELI_COL32_BLACK, 5.0f, ELI_DRAW_NONE,
                                   1.0f);
        }
        checksum += (int)g_bench_list.vtx_count;
    }
    return checksum;
}

/** 1000 auto-segment bullets + 1000 16-segment radio circles. */
BENCH_EXPORT int bench_circles(int iterations)
{
    int checksum = 0;
    for (int i = 0; i < iterations; i++) {
        eli_draw_list_reset(&g_bench_list);
        for (int r = 0; r < 1000; r++) {
            eli_vec2 c = eli_make_vec2((float)(r % 50) * 20.0f, (float)(r / 50) * 20.0f);
            eli_draw_list_add_circle_filled(&g_bench_list, c, 3.0f, ELI_COL32_WHITE, 0);
            eli_draw_list_add_circle_filled(&g_bench_list, c, 7.0f, ELI_COL32_WHITE, 16);
        }
        checksum += (int)g_bench_list.vtx_count;
    }
    return checksum;
}

/* ---------------------------------------------------------------------------
 * Native driver
 * ------------------------------------------------------------------------- */

#ifndef __wasm__
/** @return Monotonic seconds. */
static double bench_now(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

/**
 * Warm up, then time `iterations` calls five times; print the best microseconds
 * per iteration and the glyph lookups per iteration.
 */
static void bench_run(const char *name, int (*fn)(int), int iterations)
{
    fn(iterations / 10 + 1);
    double best = 1e30;
    long lookups = 0;
    int checksum = 0;
    for (int rep = 0; rep < 5; rep++) {
        long lookups_before = g_bench_glyph_lookups;
        double t0 = bench_now();
        checksum = fn(iterations);
        double t1 = bench_now();
        lookups = g_bench_glyph_lookups - lookups_before;
        if (t1 - t0 < best)
            best = t1 - t0;
    }
    printf("%-14s %8.2f us/iter  %8ld glyph lookups/iter  (%d iters, checksum %d)\n", name,
           best * 1e6 / iterations, lookups / iterations, iterations, checksum);
}

int main(void)
{
    bench_setup();
    bench_run("wrap-frame", bench_wrap_frame, 2000);
    bench_run("wrap-long", bench_wrap_long, 2000);
    bench_run("wrap-drawlist", bench_wrap_drawlist, 2000);
    bench_run("round-rect", bench_round_rect, 2000);
    bench_run("circles", bench_circles, 2000);
    return 0;
}
#endif
