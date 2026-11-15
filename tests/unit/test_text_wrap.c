/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file test_text_wrap.c
 * @brief Word wrap: the O(n) line
 *        breaker reproduces the earlier O(n^2) per-byte re-measure exactly,
 *        eli_text_wrapped draws identical geometry with a linear number of glyph
 *        lookups, and eli_draw_list_add_text_ex honors wrap_width.
 *
 * The reference breaker below is a verbatim copy of the previous O(n^2) loop in
 * eli_text__wrapped (re-measuring the line prefix at every byte with
 * eli_font_calc_text_size); every layout here is compared against it, widths
 * bit for bit, over fuzzed ASCII / multi-byte / malformed UTF-8.
 *
 * @status In use.
 * @issues None
 * @todo None
 */
#include <stdbool.h>

/* Count glyph lookups (must precede the library include). */
static long g_glyph_lookups;
#define ELI_FONT_GLYPH_LOOKUP_HOOK() (g_glyph_lookups++)

#include "eli_test.h"
#include "eli_test_ui.h"

#include <eli/font/eli_font.h>

/* ---------------------------------------------------------------------------
 * Reference: the previous O(n^2) breaker
 * ------------------------------------------------------------------------- */

#define REF_MAX_LINES 512

typedef struct ref_wrap_line {
    int start;     /* byte offset of the line's first byte */
    int end;       /* byte offset one past its last byte */
    float width;   /* eli_font_calc_text_size of [start, end) */
} ref_wrap_line;

/** The previous eli_text__wrapped break loop. @return Line count. */
static int ref_wrap_lines(const eli_font *f, float size, const char *text, const char *text_end,
                          float wrap_width, ref_wrap_line *out)
{
    int n = 0;
    const char *line_start = text;
    const char *p = text;
    const char *last_space = NULL;
    while (p <= text_end) {
        bool at_end = (p == text_end);
        char ch = at_end ? '\0' : *p;
        if (ch == ' ')
            last_space = p;
        bool overflow = (wrap_width > 0.0f && last_space != NULL && last_space > line_start &&
                         eli_font_calc_text_size(f, size, line_start, p).x > wrap_width);
        if (ch == '\n' || at_end || overflow) {
            const char *line_end = (ch == '\n' || at_end) ? p : last_space;
            const char *next_start = (ch == '\n' || at_end) ? p + 1 : last_space + 1;
            if (n < REF_MAX_LINES) {
                out[n].start = (int)(line_start - text);
                out[n].end = (int)(line_end - text);
                out[n].width = eli_font_calc_text_size(f, size, line_start, line_end).x;
                n++;
            }
            line_start = next_start;
            last_space = NULL;
            if (at_end)
                break;
            p = next_start;
            continue;
        }
        p++;
    }
    return n;
}

/* ---------------------------------------------------------------------------
 * Helpers
 * ------------------------------------------------------------------------- */

static eli_font *test_build_default(eli_font_atlas **out_atlas)
{
    eli_font_atlas *atlas = eli_font_atlas_create();
    eli_font *f = eli_font_atlas_add_font_default(atlas, NULL);
    eli_font_atlas_build(atlas);
    *out_atlas = atlas;
    return f;
}

static unsigned int g_rng = 12345u;

static unsigned int test_rand(void)
{
    g_rng = g_rng * 1103515245u + 12345u;
    return (g_rng >> 16) & 0x7FFFu;
}

/** Build fuzz text: words, runs of spaces, '\n', '\r', valid and malformed UTF-8. */
static int test_fuzz_text(char *buf, int cap)
{
    static const char *const tokens[] = {
        "a", "bb", "Wide", "iiii", " ", "  ", " ", "\n", "\r", "\xC3\xA9", "\xE2\x82\xAC",
        "\xF0\x9F\x98\x80", "\xC3", "\x80", "\xE2\x82", "word", " ", "x\xC3\xA9y",
    };
    int ntok = (int)(sizeof(tokens) / sizeof(tokens[0]));
    int target = (int)(test_rand() % (unsigned int)(cap - 8));
    int len = 0;
    while (len < target) {
        const char *t = tokens[test_rand() % (unsigned int)ntok];
        int tl = (int)strlen(t);
        if (len + tl >= cap - 1)
            break;
        memcpy(buf + len, t, (size_t)tl);
        len += tl;
    }
    buf[len] = '\0';
    return len;
}

/** Compare the new breaker's lines with the reference. @return true if identical. */
static bool test_lines_match(const eli_font *f, float size, const char *text, const char *text_end,
                             float wrap_width)
{
    ref_wrap_line ref[REF_MAX_LINES];
    int ref_n = ref_wrap_lines(f, size, text, text_end, wrap_width, ref);
    int n = 0;
    for (const char *line = text;; n++) {
        float w;
        const char *end = eli_font_calc_word_wrap_line(f, size, line, text_end, wrap_width, &w);
        if (n >= ref_n || ref[n].start != (int)(line - text) || ref[n].end != (int)(end - text) ||
            ref[n].width != w)
            return false;
        if (end == text_end)
            break;
        line = end + 1;
    }
    return n + 1 == ref_n;
}

/** @return true if two draw lists hold bit-identical vertices and indices. */
static bool test_geometry_equal(const eli_draw_list *a, const eli_draw_list *b)
{
    if (a->vtx_count != b->vtx_count || a->idx_count != b->idx_count)
        return false;
    for (uint32_t i = 0; i < a->vtx_count; i++) {
        const eli_draw_vert *va = &a->vtx[i], *vb = &b->vtx[i];
        if (va->x != vb->x || va->y != vb->y || va->u != vb->u ||
            va->v != vb->v || va->col != vb->col)
            return false;
    }
    return memcmp(a->idx, b->idx, a->idx_count * sizeof(*a->idx)) == 0;
}

/* ---------------------------------------------------------------------------
 * Line breaker parity
 * ------------------------------------------------------------------------- */

ELI_TEST(word_wrap_line_matches_reference_on_fixed_cases) {
    eli_font_atlas *atlas;
    eli_font *f = test_build_default(&atlas);
    static const char *const cases[] = {
        "", " ", "aaa bbb ccc", "aaa bbbb ccc", "a\nb\n\nc", "   leading spaces",
        "trailing   ", "averyveryverylongwordthatcannotfit next",
        "x\xC3\xA9y x\xC3\xA9y x\xC3\xA9y",
        "aaaaaa\xE2\x82\xAC bb", "aaaaa\xF0\x9F\x98\x80 cc", "bad \xC3 lead \x80 cont",
    };
    static const float widths[] = {-1.0f, 0.0f,  1.0f,  7.0f,  21.0f,
                                   42.0f, 48.9f, 49.0f, 49.1f, 300.0f};
    int mismatches = 0;
    for (size_t c = 0; c < sizeof(cases) / sizeof(cases[0]); c++) {
        const char *t = cases[c];
        for (size_t w = 0; w < sizeof(widths) / sizeof(widths[0]); w++)
            if (!test_lines_match(f, f->font_size, t, t + strlen(t), widths[w]))
                mismatches++;
    }
    ELI_ASSERT_EQ(mismatches, 0);
    eli_font_atlas_destroy(atlas);
}

ELI_TEST(word_wrap_line_matches_reference_fuzzed) {
    eli_font_atlas *atlas;
    eli_font *f = test_build_default(&atlas);
    static const float sizes_mul[] = {1.0f, 1.3307f, 0.77f};
    char buf[200];
    int mismatches = 0, cases = 0;
    g_rng = 12345u;
    for (int iter = 0; iter < 3000; iter++) {
        int len = test_fuzz_text(buf, (int)sizeof(buf));
        /* Also cut the text short, possibly inside a multi-byte sequence. */
        int cut = len > 0 ? (int)(test_rand() % (unsigned int)(len + 1)) : 0;
        float size = f->font_size * sizes_mul[iter % 3];
        float wrap = (float)(test_rand() % 160) - 5.0f + (float)(test_rand() % 4) * 0.25f;
        if (!test_lines_match(f, size, buf, buf + len, wrap))
            mismatches++;
        if (!test_lines_match(f, size, buf, buf + cut, wrap))
            mismatches++;
        cases += 2;
    }
    ELI_ASSERT_EQ(cases, 6000);
    ELI_ASSERT_EQ(mismatches, 0);
    eli_font_atlas_destroy(atlas);
}

/* ---------------------------------------------------------------------------
 * eli_text_wrapped: identical geometry, linear glyph lookups
 * ------------------------------------------------------------------------- */

static const char *g_wrap_text;
static eli_vec2 g_wrap_pos;
static float g_wrap_width;
static uint32_t g_wrap_vtx_begin, g_wrap_vtx_end;
static long g_wrap_lookups;
static eli_vec2 g_wrap_item_size;

static void ui_wrapped_text(void)
{
    eli_set_next_window_pos(eli_make_vec2(10.0f, 10.0f), 0, eli_make_vec2(0, 0));
    eli_set_next_window_size(eli_make_vec2(400.0f, 500.0f), 0);
    if (eli_begin("Wrap", NULL, 0)) {
        eli_draw_list *dl = eli_get_window_draw_list();
        g_wrap_pos = eli_get_cursor_screen_pos();
        g_wrap_width = eli_get_content_region_avail().x;
        g_wrap_vtx_begin = dl->vtx_count;
        long before = g_glyph_lookups;
        eli_text_wrapped("%s", g_wrap_text);
        g_wrap_lookups = g_glyph_lookups - before;
        g_wrap_vtx_end = dl->vtx_count;
        g_wrap_item_size = eli_get_item_rect_size();
    }
    eli_end();
}

static const char g_paragraph[] =
    "This window is built every frame from plain C calls: no retained widget tree, no "
    "callbacks.\nEach widget reads and writes your variables directly, and the library turns "
    "the calls into a vertex buffer that any renderer can draw. Caf\xC3\xA9 costs 3\xE2\x82\xAC.";

ELI_TEST(text_wrapped_geometry_matches_reference) {
    eli_context *ctx = test_ui_setup();
    g_wrap_text = g_paragraph;
    test_ui_frames(ui_wrapped_text, 2);

    eli_draw_list *dl = &ctx->windows[0]->draw_list;
    const eli_font *f = ctx->font;
    ref_wrap_line ref[REF_MAX_LINES];
    int n = ref_wrap_lines(f, ctx->font_size, g_paragraph, g_paragraph + strlen(g_paragraph),
                           g_wrap_width, ref);
    ELI_ASSERT_GT(n, 3);

    /* Expected: each reference line drawn unwrapped, one text line height apart. */
    eli_draw_list want;
    eli_draw_list_init(&want, ELI_DRAW_LIST_NONE);
    want.tex_uv_white_pixel = dl->tex_uv_white_pixel; /* non-glyph shapes (tofu) sample it */
    float y = g_wrap_pos.y, max_w = 0.0f;
    for (int i = 0; i < n; i++) {
        if (ref[i].end > ref[i].start)
            eli_draw_list_add_text(&want, eli_make_vec2(g_wrap_pos.x, y),
                                   dl->vtx[g_wrap_vtx_begin].col, g_paragraph + ref[i].start,
                                   g_paragraph + ref[i].end);
        y += eli_get_text_line_height();
        if (ref[i].width > max_w)
            max_w = ref[i].width;
    }
    ELI_ASSERT_EQ(g_wrap_vtx_end - g_wrap_vtx_begin, want.vtx_count);
    bool same = true;
    for (uint32_t i = 0; i < want.vtx_count && same; i++) {
        const eli_draw_vert *a = &dl->vtx[g_wrap_vtx_begin + i], *b = &want.vtx[i];
        same = a->x == b->x && a->y == b->y && a->u == b->u &&
               a->v == b->v && a->col == b->col;
    }
    ELI_ASSERT_TRUE(same);
    ELI_ASSERT_TRUE(g_wrap_item_size.x == max_w);
    ELI_ASSERT_TRUE(g_wrap_item_size.y == y - g_wrap_pos.y);

    eli_draw_list_clear(&want);
    test_ui_teardown(ctx);
}

ELI_TEST(text_wrapped_glyph_lookups_are_linear) {
    /* ~1000 bytes of words: the old per-byte re-measure costs tens of thousands of
     * lookups (twice: measure + render); one pass + render is a few per byte. */
    static char text[1000];
    int n = (int)sizeof(g_paragraph) - 1;
    for (int i = 0; i < (int)sizeof(text) - 1; i++)
        text[i] = g_paragraph[i % n] == '\n' ? ' ' : g_paragraph[i % n];
    text[sizeof(text) - 1] = '\0';

    eli_context *ctx = test_ui_setup();
    g_wrap_text = text;
    test_ui_frames(ui_wrapped_text, 2);
    long len = (long)strlen(text);
    ELI_ASSERT_GT(g_wrap_lookups, len);          /* the hook is live */
    ELI_ASSERT_LE(g_wrap_lookups, 4 * len);
    test_ui_teardown(ctx);
}

/* ---------------------------------------------------------------------------
 * add_text_ex honors wrap_width
 * ------------------------------------------------------------------------- */

ELI_TEST(add_text_ex_wraps_onto_next_line) {
    eli_font_atlas *atlas;
    eli_font *f = test_build_default(&atlas);
    eli_draw_list got, want;
    eli_draw_list_init(&got, ELI_DRAW_LIST_NONE);
    eli_draw_list_init(&want, ELI_DRAW_LIST_NONE);

    /* 7 px monospace: "aaa bbb" is 49 px, so a 49 px wrap moves "ccc" down a line. */
    eli_vec2 pos = eli_make_vec2(5.0f, 8.0f);
    eli_draw_list_add_text_ex(&got, f, f->font_size, pos, ELI_COL32_WHITE, "aaa bbb ccc", NULL,
                              49.0f, NULL);
    eli_draw_list_add_text_ex(&want, f, f->font_size, pos, ELI_COL32_WHITE, "aaa bbb", NULL, 0.0f,
                              NULL);
    eli_draw_list_add_text_ex(&want, f, f->font_size, eli_make_vec2(pos.x, pos.y + f->line_height),
                              ELI_COL32_WHITE, "ccc", NULL, 0.0f, NULL);
    ELI_ASSERT_EQ(got.vtx_count, 9u * 4u);
    ELI_ASSERT_TRUE(test_geometry_equal(&got, &want));
    ELI_ASSERT_EQ(got.cmds[got.cmd_count - 1].elem_count, got.idx_count);

    eli_draw_list_clear(&got);
    eli_draw_list_clear(&want);
    eli_font_atlas_destroy(atlas);
}

ELI_TEST(add_text_ex_wrap_matches_reference_lines) {
    eli_font_atlas *atlas;
    eli_font *f = test_build_default(&atlas);
    eli_draw_list got, want;
    eli_draw_list_init(&got, ELI_DRAW_LIST_NONE);
    eli_draw_list_init(&want, ELI_DRAW_LIST_NONE);
    char buf[200];
    int mismatches = 0;
    g_rng = 777u;
    for (int iter = 0; iter < 500; iter++) {
        int len = test_fuzz_text(buf, (int)sizeof(buf));
        float size = f->font_size * (iter % 2 ? 1.3307f : 1.0f);
        float wrap = 10.0f + (float)(test_rand() % 150);
        float line_h = f->line_height * (size / f->font_size);
        eli_vec2 pos = eli_make_vec2(3.0f, 4.0f);

        eli_draw_list_reset(&got);
        eli_draw_list_reset(&want);
        eli_draw_list_add_text_ex(&got, f, size, pos, ELI_COL32_WHITE, buf, buf + len, wrap, NULL);
        ref_wrap_line ref[REF_MAX_LINES];
        int n = ref_wrap_lines(f, size, buf, buf + len, wrap, ref);
        float max_w = 0.0f;
        eli_vec2 line_pos = pos;
        for (int i = 0; i < n; i++) {
            eli_draw_list_add_text_ex(&want, f, size, line_pos, ELI_COL32_WHITE, buf + ref[i].start,
                                      buf + ref[i].end, 0.0f, NULL);
            line_pos.y += line_h;
            if (ref[i].width > max_w)
                max_w = ref[i].width;
        }
        eli_vec2 sz = eli_font_calc_text_size_wrapped(f, size, buf, buf + len, wrap);
        if (!test_geometry_equal(&got, &want) || sz.x != max_w ||
            fabsf(sz.y - line_h * (float)n) > 1e-3f)
            mismatches++;
    }
    ELI_ASSERT_EQ(mismatches, 0);

    eli_draw_list_clear(&got);
    eli_draw_list_clear(&want);
    eli_font_atlas_destroy(atlas);
}

ELI_TEST(add_text_ex_zero_wrap_is_unwrapped) {
    eli_font_atlas *atlas;
    eli_font *f = test_build_default(&atlas);
    eli_draw_list dl;
    eli_draw_list_init(&dl, ELI_DRAW_LIST_NONE);

    /* wrap_width 0: one line, the last glyph's quad sits 10 advances right. */
    eli_draw_list_add_text_ex(&dl, f, f->font_size, eli_make_vec2(0, 0), ELI_COL32_WHITE,
                              "aaa bbb ccc", NULL, 0.0f, NULL);
    const eli_font_glyph *c = eli_font_find_glyph(f, 'c');
    ELI_ASSERT_FLT_NEAR(dl.vtx[dl.vtx_count - 4].x, 10.0f * 7.0f + c->x0, 1e-4);
    ELI_ASSERT_FLT_NEAR(dl.vtx[dl.vtx_count - 4].y, c->y0, 1e-4);

    eli_draw_list_clear(&dl);
    eli_font_atlas_destroy(atlas);
}

ELI_TEST_MAIN()
