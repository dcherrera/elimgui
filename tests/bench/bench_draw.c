/**
 * @file bench_draw.c
 * @brief Host benchmark for elimgui draw-list generation (Phase 33).
 *
 * Builds representative frames many times and reports wall-clock time plus the
 * vertex/index totals produced. Three workloads are measured:
 *
 *   1. prim-fill  - a large filled-rectangle stress into a bare draw list.
 *   2. text       - a long text run rasterized into glyph quads.
 *   3. frame      - a full context frame: a window with many widgets and a
 *                   multi-row table, assembled through eli_frame_begin/end.
 *
 * It also proves the allocation-reuse invariant the phase relies on: after a
 * short warm-up, the summed heap capacity of the frame's draw lists stops
 * growing, so steady-state frames perform zero geometry reallocations.
 *
 * This is a standalone native program, NOT a unit test (it lives outside
 * tests/unit so ./build.sh test skips it). Build and run it with:
 *
 *   cc -std=c11 -O2 -DELI_TEST_HOSTED -Iinclude -Ivendor \
 *      tests/bench/bench_draw.c -o /tmp/bench_draw -lm && /tmp/bench_draw
 *
 * @status Phase 33 benchmark harness.
 * @issues None
 * @todo None
 */
#include <stdio.h>
#include <time.h>

#include <eli/elimgui.h>
#include <eli/font/eli_font.h>

/* ---------------------------------------------------------------------------
 * Timing helper
 * ------------------------------------------------------------------------- */

/** @return Monotonic-ish seconds from the process clock. */
static double bench_now_seconds(void)
{
    return (double)clock() / (double)CLOCKS_PER_SEC;
}

/* ---------------------------------------------------------------------------
 * Workload 1: filled-primitive stress
 * ------------------------------------------------------------------------- */

static void bench_prim_fill(int iterations, int rects_per_iter)
{
    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);

    /* Warm the buffers so the timed loop measures steady-state fill cost. */
    for (int r = 0; r < rects_per_iter; r++) {
        float x = (float)(r % 500);
        eli_draw_list_add_rect_filled(&dl, eli_make_vec2(x, x), eli_make_vec2(x + 8, x + 8),
                                      ELI_COL32_WHITE, 0.0f, ELI_DRAW_NONE);
    }
    uint32_t cap_after_warm = dl.vtx_capacity + dl.idx_capacity;
    eli_draw_list_reset(&dl);

    int grow_events = 0;
    double t0 = bench_now_seconds();
    for (int i = 0; i < iterations; i++) {
        eli_draw_list_reset(&dl);
        uint32_t cap_before = dl.vtx_capacity + dl.idx_capacity;
        for (int r = 0; r < rects_per_iter; r++) {
            float x = (float)(r % 500);
            eli_draw_list_add_rect_filled(&dl, eli_make_vec2(x, x), eli_make_vec2(x + 8, x + 8),
                                          ELI_COL32_WHITE, 0.0f, ELI_DRAW_NONE);
        }
        if (dl.vtx_capacity + dl.idx_capacity != cap_before)
            grow_events++;
    }
    double t1 = bench_now_seconds();

    printf("prim-fill : %d iters x %d rects  %.3f ms/iter  vtx=%u idx=%u  "
           "grow-events(steady)=%d  warm-cap-elems=%u\n",
           iterations, rects_per_iter, (t1 - t0) * 1000.0 / iterations, dl.vtx_count,
           dl.idx_count, grow_events, cap_after_warm);
    eli_draw_list_clear(&dl);
}

/* ---------------------------------------------------------------------------
 * Workload 2: text stress
 * ------------------------------------------------------------------------- */

static void bench_text(int iterations, int lines)
{
    eli_font_atlas *atlas = eli_font_atlas_create();
    eli_font *font = eli_font_atlas_add_font_default(atlas, NULL);
    eli_font_atlas_build(atlas);

    static const char *line = "The quick brown fox jumps over the lazy dog 0123456789 ";

    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);

    /* Warm-up pass. */
    for (int l = 0; l < lines; l++)
        eli_draw_list_add_text_ex(&dl, font, font->font_size, eli_make_vec2(4.0f, (float)l * 12.0f),
                                  ELI_COL32_WHITE, line, NULL, 0.0f, NULL);
    eli_draw_list_reset(&dl);

    int grow_events = 0;
    double t0 = bench_now_seconds();
    for (int i = 0; i < iterations; i++) {
        eli_draw_list_reset(&dl);
        uint32_t cap_before = dl.vtx_capacity + dl.idx_capacity;
        for (int l = 0; l < lines; l++)
            eli_draw_list_add_text_ex(&dl, font, font->font_size,
                                      eli_make_vec2(4.0f, (float)l * 12.0f), ELI_COL32_WHITE, line,
                                      NULL, 0.0f, NULL);
        if (dl.vtx_capacity + dl.idx_capacity != cap_before)
            grow_events++;
    }
    double t1 = bench_now_seconds();

    printf("text      : %d iters x %d lines  %.3f ms/iter  vtx=%u idx=%u  "
           "grow-events(steady)=%d\n",
           iterations, lines, (t1 - t0) * 1000.0 / iterations, dl.vtx_count, dl.idx_count,
           grow_events);

    eli_draw_list_clear(&dl);
    eli_font_atlas_destroy(atlas);
}

/* ---------------------------------------------------------------------------
 * Workload 3: full frame with widgets + table
 * ------------------------------------------------------------------------- */

/** Sum the heap capacity (in elements) of every draw list in the frame's data. */
static uint32_t bench_frame_capacity(eli_context *ctx)
{
    uint32_t total = 0;
    for (int i = 0; i < ctx->render_draw_lists_count; i++) {
        eli_draw_list *dl = ctx->render_draw_lists[i];
        total += dl->vtx_capacity + dl->idx_capacity + dl->cmd_capacity;
    }
    return total;
}

static void bench_build_frame(void)
{
    eli_set_next_window_pos(eli_make_vec2(20.0f, 20.0f), 0, eli_make_vec2(0, 0));
    eli_set_next_window_size(eli_make_vec2(600.0f, 700.0f), 0);
    if (eli_begin("Bench", NULL, 0)) {
        eli_text("Draw generation benchmark");
        eli_separator();
        for (int i = 0; i < 20; i++) {
            char label[32];
            snprintf(label, sizeof(label), "Button %d", i);
            eli_button(label);
            eli_same_line(0.0f, -1.0f);
            static float slider_v = 0.5f;
            eli_slider_float("slide", &slider_v, 0.0f, 1.0f, "%.2f", 0);
            static bool checked = true;
            eli_checkbox("chk", &checked);
        }
        eli_separator();
        if (eli_begin_table("grid", 4, 0)) {
            eli_table_setup_column("A", 0, 0.0f, 0);
            eli_table_setup_column("B", 0, 0.0f, 0);
            eli_table_setup_column("C", 0, 0.0f, 0);
            eli_table_setup_column("D", 0, 0.0f, 0);
            eli_table_headers_row();
            for (int row = 0; row < 40; row++) {
                eli_table_next_row();
                for (int col = 0; col < 4; col++) {
                    eli_table_next_column();
                    char cell[24];
                    snprintf(cell, sizeof(cell), "r%dc%d", row, col);
                    eli_text(cell);
                }
            }
            eli_end_table();
        }
    }
    eli_end();
}

static void bench_frame(int iterations)
{
    eli_context *ctx = eli_create_context();
    eli_style_colors_dark(&ctx->style);
    ctx->io.delta_time = 1.0f / 60.0f;
    ctx->io.display_size = eli_make_vec2(1280.0f, 720.0f);

    eli_font_atlas *atlas = eli_font_atlas_create();
    eli_font_atlas_add_font_default(atlas, NULL);
    eli_font_atlas_build(atlas);
    eli_font_atlas_set_tex_id(atlas, 1);
    eli_push_font(atlas->fonts[0]);
    ctx->io.fonts = atlas;
    ctx->io.font_default = atlas->fonts[0];

    const int warmup = 8;
    uint32_t last_cap = 0;
    int first_stable = -1, grow_after_warm = 0;
    int total_vtx = 0, total_idx = 0, cmd_lists = 0;

    double t0 = 0.0, t_end = 0.0;
    for (int i = 0; i < iterations + warmup; i++) {
        if (i == warmup)
            t0 = bench_now_seconds();
        eli_frame_begin();
        bench_build_frame();
        eli_frame_end();

        uint32_t cap = bench_frame_capacity(ctx);
        if (i >= warmup) {
            if (cap != last_cap)
                grow_after_warm++;
            else if (first_stable < 0)
                first_stable = i - warmup;
        }
        last_cap = cap;

        eli_draw_data *dd = eli_get_draw_data();
        total_vtx = dd->total_vtx_count;
        total_idx = dd->total_idx_count;
        cmd_lists = dd->cmd_lists_count;
    }
    t_end = bench_now_seconds();

    printf("frame     : %d iters  %.3f ms/frame  vtx=%d idx=%d cmd-lists=%d  "
           "capacity-grow-events(after %d warmup)=%d  stable@frame=%d\n",
           iterations, (t_end - t0) * 1000.0 / iterations, total_vtx, total_idx, cmd_lists, warmup,
           grow_after_warm, first_stable);

    eli_destroy_context(ctx);
    eli_font_atlas_destroy(atlas);
}

int main(void)
{
    printf("elimgui draw-generation benchmark\n");
    printf("---------------------------------\n");
    bench_prim_fill(2000, 2000);
    bench_text(2000, 40);
    bench_frame(2000);
    return 0;
}
