/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file bench_font_atlas.c
 * @brief Host benchmark for the font atlas: a desktop app's font set (body, heading,
 *        mono, terminal + system fallback faces at 2x density) built three ways.
 *
 *   legacy     every face baked up front into one atlas; fallback glyphs baked as
 *              extra fonts and copied in after the build (the pre-merge-mode workaround).
 *   merge      the same faces as merge-mode fallback chains, baked up front.
 *   on-demand  merge chains, glyphs rasterized on first use into pages.
 *
 * Reports build time (median of N), texture size and bytes (A8 and the RGBA32 the
 * SDL3 backend used to upload), then draws a sample of typical UI + terminal text and
 * reports the bytes the on-demand pages hold after it. Builds against the base API
 * too (only `legacy` runs there), so before/after numbers come from the same file.
 *
 *   cc -std=c11 -O2 -DELI_TEST_HOSTED -Iinclude -Ivendor tests/bench/bench_font_atlas.c \
 *      -o tests/build/bench_font_atlas -lm && tests/build/bench_font_atlas <dir with Roboto-Regular.ttf
 *      and JetBrainsMono-Regular.ttf>
 *
 * @status In use.
 * @issues Fallback faces are read from macOS system paths; absent ones are skipped.
 * @todo None
 */
#include <stdio.h>
#include <time.h>

#include <eli/elimgui.h>
#include <eli/font/eli_font.h>

#define BENCH_RUNS 5
#define BENCH_DENSITY 2.0f

/* The app's glyph ranges: UI faces, their fallback blocks, the terminal and its fallbacks. */
static const uint16_t kGlyphRanges[] = {
    0x0020, 0x024F, 0x0370, 0x03FF, 0x0400, 0x04FF, 0x2000, 0x206F, 0x2070, 0x20CF,
    0x2100, 0x214F, 0x2190, 0x21FF, 0x2200, 0x22FF, 0x2300, 0x23FF, 0x2500, 0x259F,
    0x25A0, 0x25FF, 0x2600, 0x26FF, 0x2700, 0x27BF, 0xE0A0, 0xE0B3, 0xFFFD, 0xFFFD, 0,
};
static const uint16_t kFallbackRanges[] = {
    0x2190, 0x21FF, 0x2200, 0x22FF, 0x2300, 0x23FF, 0x2500, 0x259F, 0x25A0, 0x25FF,
    0x2600, 0x26FF, 0x2700, 0x27BF, 0xE0A0, 0xE0B3, 0,
};
static const uint16_t kTermGlyphRanges[] = {
    0x0020, 0x024F, 0x0370, 0x03FF, 0x0400, 0x04FF, 0x2000, 0x206F, 0x2070, 0x20CF,
    0x2100, 0x214F, 0x2190, 0x21FF, 0x2200, 0x22FF, 0x2300, 0x23FF, 0x2440, 0x245F,
    0x2500, 0x259F, 0x25A0, 0x25FF, 0x2600, 0x26FF, 0x2700, 0x27BF, 0x27C0, 0x27FF,
    0x2800, 0x28FF, 0x2900, 0x297F, 0x2980, 0x29FF, 0x2B00, 0x2BFF, 0xE0A0, 0xE0B3,
    0xFFFD, 0xFFFD, 0,
};
static const uint16_t kTermFallbackRanges[] = {
    0x2000, 0x206F, 0x2070, 0x20CF, 0x2100, 0x214F, 0x2190, 0x21FF, 0x2200, 0x22FF,
    0x2300, 0x23FF, 0x2440, 0x245F, 0x2500, 0x259F, 0x25A0, 0x25FF, 0x2600, 0x26FF,
    0x2700, 0x27BF, 0x27C0, 0x27FF, 0x2800, 0x28FF, 0x2900, 0x297F, 0x2980, 0x29FF,
    0x2B00, 0x2BFF, 0,
};

static const char *const kFallbackPaths[] = {
    "/System/Library/Fonts/Menlo.ttc",
    "/System/Library/Fonts/Apple Symbols.ttf",
};
#define FALLBACK_COUNT (int)(sizeof(kFallbackPaths) / sizeof(kFallbackPaths[0]))

/* Extra atlas flags for every build (main sets NO_POWER_OF_TWO_HEIGHT for a second pass
 * that shows the height the glyphs really use). */
static eli_font_atlas_flags g_bench_flags;
static int g_bench_page_size; /* on-demand page side (0 = the atlas default) */

typedef struct BenchFace {
    unsigned char *data;
    int size;
} BenchFace;

typedef struct BenchFaces {
    BenchFace body, mono;
    BenchFace fallback[FALLBACK_COUNT];
} BenchFaces;

typedef struct BenchFonts {
    eli_font *body, *heading, *mono, *term;
} BenchFonts;

/** @return monotonic seconds. */
static double bench_now(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

/** Read a whole file; {NULL, 0} when absent. */
static BenchFace bench_read_face(const char *path)
{
    BenchFace face = { NULL, 0 };
    FILE *fp = fopen(path, "rb");
    if (!fp)
        return face;
    fseek(fp, 0, SEEK_END);
    long n = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    face.data = n > 0 ? (unsigned char *)malloc((size_t)n) : NULL;
    if (face.data && fread(face.data, 1, (size_t)n, fp) == (size_t)n)
        face.size = (int)n;
    fclose(fp);
    return face;
}

/** Add one face at `px` points with `ranges` (density 2, merge flags from `tmpl`). */
static eli_font *bench_add(eli_font_atlas *atlas, const BenchFace *face, float px,
                           const uint16_t *ranges, const eli_font_config *tmpl)
{
    eli_font_config cfg = tmpl ? *tmpl : eli_font_config_default();
    cfg.rasterizer_density = BENCH_DENSITY;
    return eli_font_atlas_add_font_from_memory_ttf(atlas, face->data, face->size, px, &cfg,
                                                   ranges);
}

/* ---- legacy: bake fallback faces as fonts, copy their glyphs in afterwards ----- */

static void bench_borrow(eli_font *dst, const eli_font *sym, bool cell_fit)
{
    const eli_font_glyph *m = eli_font_find_glyph_no_fallback(dst, 'M');
    float cell = m ? m->advance_x : 0.0f;
    for (int i = 0; i < sym->glyph_count; i++) {
        eli_font_glyph g = sym->glyphs[i];
        if (eli_font_find_glyph_no_fallback(dst, g.codepoint))
            continue;
        float dy = dst->ascent - sym->ascent, w = g.x1 - g.x0;
        g.y0 += dy;
        g.y1 += dy;
        if (cell_fit) {
            if (w > cell) {
                float k = cell / w, cy = (g.y0 + g.y1) * 0.5f;
                g.y0 = cy + (g.y0 - cy) * k;
                g.y1 = cy + (g.y1 - cy) * k;
                w = cell;
            }
            g.x0 = (cell - w) * 0.5f;
            g.x1 = g.x0 + w;
            g.advance_x = cell;
        }
        eli_font_add_glyph(dst, &g);
    }
}

static eli_font_atlas *bench_build_legacy(const BenchFaces *f, BenchFonts *out)
{
    eli_font_atlas *atlas = eli_font_atlas_create();
    atlas->flags |= g_bench_flags;
    out->body = bench_add(atlas, &f->body, 13.0f, kGlyphRanges, NULL);
    out->heading = bench_add(atlas, &f->body, 13.0f * 1.35f, kGlyphRanges, NULL);
    out->mono = bench_add(atlas, &f->mono, 12.0f, kGlyphRanges, NULL);
    eli_font *sym_body = bench_add(atlas, &f->mono, 13.0f, kFallbackRanges, NULL);
    eli_font *sym_heading = bench_add(atlas, &f->mono, 13.0f * 1.35f, kFallbackRanges, NULL);
    out->term = bench_add(atlas, &f->mono, 12.0f, kTermGlyphRanges, NULL);
    eli_font *sym_term[FALLBACK_COUNT] = { 0 };
    for (int i = 0; i < FALLBACK_COUNT; i++)
        if (f->fallback[i].data)
            sym_term[i] = bench_add(atlas, &f->fallback[i], 12.0f, kTermFallbackRanges, NULL);
    if (!eli_font_atlas_build(atlas))
        return atlas;
    bench_borrow(out->body, sym_body, false);
    bench_borrow(out->heading, sym_heading, false);
    for (int i = 0; i < FALLBACK_COUNT; i++)
        if (sym_term[i])
            bench_borrow(out->term, sym_term[i], true);
    return atlas;
}

#ifdef ELI_FONT_HAS_PAGES
/* ---- merge / on-demand: fallback chains (+ pages) ------------------------------ */

static eli_font_atlas *bench_build_chains(const BenchFaces *f, BenchFonts *out, bool on_demand)
{
    eli_font_atlas *atlas = eli_font_atlas_create();
    atlas->flags |= g_bench_flags;
    if (on_demand)
        atlas->flags |= ELI_FONT_ATLAS_ON_DEMAND;
    atlas->page_size = g_bench_page_size;
    eli_font_config merge = eli_font_config_default();
    merge.merge_mode = true;
    eli_font_config merge_cell = merge;
    merge_cell.merge_match_advance = true;
    out->body = bench_add(atlas, &f->body, 13.0f, kGlyphRanges, NULL);
    bench_add(atlas, &f->mono, 13.0f, kFallbackRanges, &merge);
    out->heading = bench_add(atlas, &f->body, 13.0f * 1.35f, kGlyphRanges, NULL);
    bench_add(atlas, &f->mono, 13.0f * 1.35f, kFallbackRanges, &merge);
    out->mono = bench_add(atlas, &f->mono, 12.0f, kGlyphRanges, NULL);
    out->term = bench_add(atlas, &f->mono, 12.0f, kTermGlyphRanges, NULL);
    for (int i = 0; i < FALLBACK_COUNT; i++)
        if (f->fallback[i].data)
            bench_add(atlas, &f->fallback[i], 12.0f, kTermFallbackRanges, &merge_cell);
    eli_font_atlas_build(atlas);
    return atlas;
}
#endif

/* ---- sample text -------------------------------------------------------------- */

static const char *const kSampleUi[] = {
    "Sessions  Cards  Settings  Connect  Decisions",
    "The quick brown fox jumps over the lazy dog 0123456789",
    "Plan: step 2 memory budget \xE2\x80\x94 \xE2\x80\x9Cquoted\xE2\x80\x9D \xE2\x86\x92 next \xE2\x9C\x93",
    "{}[]()<>=+-*/%&|^~!?@#$_;:,.'\"`\\",
};
static const char *const kSampleTerm[] = {
    "\xE2\x8F\xBA Read(src/main.c)",
    "  \xE2\x8E\xBF  Read 409 lines (ctrl+r to expand)",
    "\xE2\x94\x8C\xE2\x94\x80\xE2\x94\x80\xE2\x94\x90 \xE2\x94\x82 \xE2\x94\x94\xE2\x94\x80\xE2\x94\x98 $ make -j8 tests && ./build/app",
    "\xE2\x9C\xBB Thinking\xE2\x80\xA6 (12s \xC2\xB7 esc to interrupt)",
};

/** Draw the sample text through each font; returns the glyph quads emitted. */
static unsigned bench_draw_sample(const BenchFonts *fonts)
{
    eli_draw_list list;
    eli_draw_list_init(&list, 0);
    eli_draw_list_reset(&list);
    eli_draw_list_push_clip_rect_full_screen(&list);
    eli_font *ui[3] = { fonts->body, fonts->heading, fonts->mono };
    for (int f = 0; f < 3; f++)
        for (size_t i = 0; i < sizeof(kSampleUi) / sizeof(kSampleUi[0]); i++)
            eli_draw_list_add_text_ex(&list, ui[f], ui[f]->font_size, eli_make_vec2(0, 0),
                                      0xFFFFFFFFu, kSampleUi[i], NULL, 0.0f, NULL);
    for (size_t i = 0; i < sizeof(kSampleTerm) / sizeof(kSampleTerm[0]); i++)
        eli_draw_list_add_text_ex(&list, fonts->term, fonts->term->font_size, eli_make_vec2(0, 0),
                                  0xFFFFFFFFu, kSampleTerm[i], NULL, 0.0f, NULL);
    unsigned quads = list.vtx_count / 4;
    eli_draw_list_clear(&list);
    return quads;
}

/* ---- report ------------------------------------------------------------------- */

typedef eli_font_atlas *(*BenchBuildFn)(const BenchFaces *, BenchFonts *, bool);

static eli_font_atlas *bench_legacy_adapter(const BenchFaces *f, BenchFonts *out, bool unused)
{
    (void)unused;
    return bench_build_legacy(f, out);
}

static int bench_cmp_double(const void *a, const void *b)
{
    double x = *(const double *)a, y = *(const double *)b;
    return x < y ? -1 : x > y;
}

/** Bytes of A8 texture the atlas holds (all pages when paged). */
static long bench_atlas_bytes(const eli_font_atlas *atlas, int *pages)
{
#ifdef ELI_FONT_HAS_PAGES
    *pages = eli_font_atlas_live_page_count(atlas);
    return (long)eli_font_atlas_texture_bytes(atlas);
#else
    *pages = 1;
    return (long)atlas->tex_width * atlas->tex_height;
#endif
}

static void bench_run(const char *name, BenchBuildFn build, const BenchFaces *faces, bool flag)
{
    double times[BENCH_RUNS];
    for (int r = 0; r < BENCH_RUNS; r++) {
        BenchFonts fonts;
        double t0 = bench_now();
        eli_font_atlas *atlas = build(faces, &fonts, flag);
        times[r] = bench_now() - t0;
        eli_font_atlas_destroy(atlas);
    }
    qsort(times, BENCH_RUNS, sizeof(times[0]), bench_cmp_double);

    BenchFonts fonts;
    eli_font_atlas *atlas = build(faces, &fonts, flag);
    int pages = 0;
    long bytes_built = bench_atlas_bytes(atlas, &pages);
    double t0 = bench_now();
    unsigned quads = bench_draw_sample(&fonts);
    double draw_ms = (bench_now() - t0) * 1e3;
    long bytes_drawn = bench_atlas_bytes(atlas, &pages);
    printf("%-10s build %7.2f ms (median of %d) | after build: %4dx%-5d A8 %8ld B, RGBA %8ld B"
           " | sample text: %u quads, %.2f ms, A8 %ld B in %d page(s)\n",
           name, times[BENCH_RUNS / 2] * 1e3, BENCH_RUNS, atlas->tex_width, atlas->tex_height,
           bytes_built, bytes_built * 4, quads, draw_ms, bytes_drawn, pages);
    eli_font_atlas_destroy(atlas);
}

#ifdef ELI_FONT_HAS_PAGES
/** Count codepoints whose glyph (presence, advance, quad) differs between two fonts. */
static int bench_diff_font(const char *name, eli_font *a, eli_font *b)
{
    int diffs = 0, present = 0;
    for (uint32_t cp = 0x20; cp <= 0xFFFF; cp++) {
        const eli_font_glyph *ga = eli_font_find_glyph_no_fallback(a, cp);
        eli_font_glyph g1 = ga ? *ga : (eli_font_glyph){ 0 };
        const eli_font_glyph *gb = eli_font_find_glyph_no_fallback(b, cp);
        bool same = (ga != NULL) == (gb != NULL);
        if (ga && gb)
            same = g1.advance_x == gb->advance_x && g1.visible == gb->visible &&
                   g1.x0 == gb->x0 && g1.y0 == gb->y0 && g1.x1 == gb->x1 && g1.y1 == gb->y1;
        present += ga ? 1 : 0;
        if (!same && diffs++ < 3)
            printf("    %s U+%04X differs\n", name, (unsigned)cp);
    }
    printf("  %-8s %5d glyphs, %d differ\n", name, present, diffs);
    return diffs;
}

/** Legacy borrow vs one chains build: every glyph must match exactly. */
static int bench_equivalence(const BenchFaces *faces, bool on_demand)
{
    BenchFonts lf, cf;
    eli_font_atlas *legacy = bench_build_legacy(faces, &lf);
    eli_font_atlas *chains = bench_build_chains(faces, &cf, on_demand);
    printf("-- glyph equivalence, legacy borrow vs %s:\n", on_demand ? "on-demand" : "merge");
    int diffs = bench_diff_font("body", lf.body, cf.body) +
                bench_diff_font("heading", lf.heading, cf.heading) +
                bench_diff_font("mono", lf.mono, cf.mono) + bench_diff_font("term", lf.term, cf.term);
    eli_font_atlas_destroy(legacy);
    eli_font_atlas_destroy(chains);
    return diffs;
}
#endif

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s <dir with Roboto-Regular.ttf and JetBrainsMono-Regular.ttf>\n",
                argv[0]);
        return 2;
    }
    const char *dir = argv[1];
    char path[1024];
    BenchFaces faces;
    memset(&faces, 0, sizeof(faces));
    snprintf(path, sizeof(path), "%s/Roboto-Regular.ttf", dir);
    faces.body = bench_read_face(path);
    snprintf(path, sizeof(path), "%s/JetBrainsMono-Regular.ttf", dir);
    faces.mono = bench_read_face(path);
    if (!faces.body.data || !faces.mono.data) {
        fprintf(stderr, "bench_font_atlas: Roboto/JetBrains Mono not found in %s\n", dir);
        return 1;
    }
    for (int i = 0; i < FALLBACK_COUNT; i++)
        faces.fallback[i] = bench_read_face(kFallbackPaths[i]);

    for (int pass = 0; pass < 2; pass++) {
        g_bench_flags = pass ? ELI_FONT_ATLAS_NO_POWER_OF_TWO_HEIGHT : 0;
        printf(pass ? "-- exact height (no power-of-two rounding):\n" : "-- default flags:\n");
        bench_run("legacy", bench_legacy_adapter, &faces, false);
#ifdef ELI_FONT_HAS_PAGES
        bench_run("merge", bench_build_chains, &faces, false);
        for (int size = 1024; !pass && size >= 256; size /= 2) {
            char label[32];
            snprintf(label, sizeof(label), "od-%d", size);
            g_bench_page_size = size;
            bench_run(label, bench_build_chains, &faces, true);
        }
        g_bench_page_size = 0;
#endif
    }
#ifdef ELI_FONT_HAS_PAGES
    g_bench_flags = 0;
    int diffs = bench_equivalence(&faces, false) + bench_equivalence(&faces, true);
#else
    int diffs = 0;
#endif
    free(faces.body.data);
    free(faces.mono.data);
    for (int i = 0; i < FALLBACK_COUNT; i++)
        free(faces.fallback[i].data);
    return diffs ? 1 : 0;
}
