/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* Copyright (C) 2026 David (Corey) Herrera */
/**
 * @file eli_font_text.h
 * @brief Text measurement and rendering plus the context font stack.
 *
 * Emits glyph quads into an eli_draw_list from a baked eli_font, measures text
 * extents (eli_calc_text_size), word-wraps in one O(n) pass
 * (eli_font_calc_word_wrap_line, shared by eli_text_wrapped and add_text_ex), and
 * manages the current-font stack on the active context (eli_push_font/eli_pop_font
 * and the get_font accessors). Text is UTF-8 decoded; whitespace glyphs advance
 * the pen without emitting geometry.
 *
 * @status Phase 3 text + font stack in use.
 * @issues None
 * @todo None
 */
#ifndef ELI_FONT_ELI_FONT_TEXT_H
#define ELI_FONT_ELI_FONT_TEXT_H

#include "eli_font_types.h"
#include "eli_font_atlas.h"

#include "../core/eli_platform.h"
#include "../core/eli_types.h"
#include "../core/eli_context.h"
#include "../draw/eli_draw.h"

/* ---------------------------------------------------------------------------
 * UTF-8 decode
 * ------------------------------------------------------------------------- */

/**
 * Decode one UTF-8 code point.
 *
 * @param s        Start of the byte sequence.
 * @param end      One past the last readable byte.
 * @param out_cp   Receives the decoded codepoint (0xFFFD on malformed input).
 * @return         Number of bytes consumed (always >= 1).
 *
 * Thread-safe: yes  Reentrant: yes
 */
static inline int eli_text_utf8_decode(const char *s, const char *end, uint32_t *out_cp)
{
    const unsigned char *u = (const unsigned char *)s;
    unsigned char c = u[0];
    if (c < 0x80) {
        *out_cp = c;
        return 1;
    }
    if ((c & 0xE0) == 0xC0 && s + 1 < end + 1 && (u[1] & 0xC0) == 0x80) {
        *out_cp = (uint32_t)((c & 0x1F) << 6) | (u[1] & 0x3F);
        return 2;
    }
    if ((c & 0xF0) == 0xE0 && s + 2 < end + 1 && (u[1] & 0xC0) == 0x80 && (u[2] & 0xC0) == 0x80) {
        *out_cp = (uint32_t)((c & 0x0F) << 12) | (uint32_t)((u[1] & 0x3F) << 6) | (u[2] & 0x3F);
        return 3;
    }
    if ((c & 0xF8) == 0xF0 && s + 3 < end + 1 && (u[1] & 0xC0) == 0x80 && (u[2] & 0xC0) == 0x80 &&
        (u[3] & 0xC0) == 0x80) {
        *out_cp = (uint32_t)((c & 0x07) << 18) | (uint32_t)((u[1] & 0x3F) << 12) |
                  (uint32_t)((u[2] & 0x3F) << 6) | (u[3] & 0x3F);
        return 4;
    }
    *out_cp = 0xFFFD;
    return 1;
}

/* ---------------------------------------------------------------------------
 * Text measurement
 * ------------------------------------------------------------------------- */

/**
 * Measure the pixel extent of a UTF-8 string in a given font/size.
 *
 * @param font         Baked font.
 * @param font_size    Rendering size (scales the baked metrics).
 * @param text_begin   Start of text.
 * @param text_end     End of text, or NULL to use strlen.
 * @return             {width, height}: widest line and total line height.
 *
 * Thread-safe: yes (read-only)  Reentrant: yes
 */
static inline eli_vec2 eli_font_calc_text_size(const eli_font *font, float font_size,
                                               const char *text_begin, const char *text_end)
{
    if (!font || !text_begin || font->font_size <= 0.0f)
        return eli_make_vec2(0.0f, 0.0f);
    if (text_end == NULL)
        text_end = text_begin + strlen(text_begin);

    float scale = font_size / font->font_size;
    float line_h = font->line_height * scale;
    float max_w = 0.0f;
    float line_w = 0.0f;
    float total_h = line_h;

    const char *s = text_begin;
    while (s < text_end) {
        uint32_t cp;
        s += eli_text_utf8_decode(s, text_end, &cp);
        if (cp == '\n') {
            if (line_w > max_w)
                max_w = line_w;
            line_w = 0.0f;
            total_h += line_h;
            continue;
        }
        if (cp == '\r')
            continue;
        const eli_font_glyph *g = eli_font_find_glyph(font, cp);
        if (g)
            line_w += g->advance_x * scale;
    }
    if (line_w > max_w)
        max_w = line_w;
    return eli_make_vec2(max_w, total_h);
}

/* ---------------------------------------------------------------------------
 * Word wrap
 * ------------------------------------------------------------------------- */

/** Add one decoded codepoint's advance to a running line width ('\r' adds nothing). */
static inline float eli_font__add_advance(const eli_font *font, float scale, uint32_t cp,
                                          float width)
{
    if (font == NULL || cp == '\r')
        return width;
    const eli_font_glyph *g = eli_font_find_glyph(font, cp);
    return g ? width + g->advance_x * scale : width;
}

/**
 * Find where one greedily word-wrapped line ends, in O(line length).
 *
 * Breaks on '\n', at the end of the text, or at the last space once the line
 * grows wider than `wrap_width`. A word wider than the whole line overflows. The
 * width is kept as a running sum (one glyph lookup per codepoint, Dear ImGui's
 * CalcWordWrapPositionA shape) and reproduces the earlier per-byte re-measure
 * exactly: the overflow test runs at every byte, and a byte inside a multi-byte
 * sequence sees the width eli_font_calc_text_size would report for the prefix
 * ending there.
 *
 * @param font        Baked font (NULL measures every glyph as zero width).
 * @param font_size   Rendering size.
 * @param line_start  First byte of the line.
 * @param text_end    End of the whole text.
 * @param wrap_width  Wrap width in pixels (<= 0 breaks only on '\n').
 * @param out_width   Receives the line's width (excluding the break character).
 * @return            One past the line's last byte. Equal to `text_end` for the last
 *                    line; otherwise it points at the '\n' or space that ends the line
 *                    and the next line starts one byte later.
 *
 * Thread-safe: yes (read-only)  Reentrant: yes
 */
static inline const char *eli_font_calc_word_wrap_line(const eli_font *font, float font_size,
                                                       const char *line_start,
                                                       const char *text_end, float wrap_width,
                                                       float *out_width)
{
    if (font != NULL && font->font_size <= 0.0f)
        font = NULL;
    float scale = font ? font_size / font->font_size : 0.0f;

    const char *boundary = line_start;   /* last codepoint boundary at or before p */
    float boundary_w = 0.0f;              /* width of [line_start, boundary) */
    const char *last_space = NULL;
    float last_space_w = 0.0f;

    for (const char *p = line_start;; p++) {
        /* Advance over every codepoint that ends at or before p. */
        while (boundary < p) {
            uint32_t cp;
            int len = eli_text_utf8_decode(boundary, text_end, &cp);
            if (boundary + len > p)
                break;
            boundary_w = eli_font__add_advance(font, scale, cp, boundary_w);
            boundary += len;
        }
        /* p inside a sequence: decode the tail as the prefix measure would (end = p). */
        float width = boundary_w;
        for (const char *s = boundary; s < p;) {
            uint32_t cp;
            s += eli_text_utf8_decode(s, p, &cp);
            width = eli_font__add_advance(font, scale, cp, width);
        }

        bool at_end = (p == text_end);
        char ch = at_end ? '\0' : *p;
        if (ch == ' ') {
            last_space = p;
            last_space_w = width;
        }
        if (ch == '\n' || at_end) {
            *out_width = width;
            return p;
        }
        if (wrap_width > 0.0f && last_space != NULL && last_space > line_start &&
            width > wrap_width) {
            *out_width = last_space_w;
            return last_space;
        }
    }
}

/**
 * Measure word-wrapped text: the widest line and the total height.
 *
 * @param font        Baked font.
 * @param font_size   Rendering size.
 * @param text_begin  Start of text.
 * @param text_end    End of text, or NULL to use strlen.
 * @param wrap_width  Wrap width in pixels (<= 0 breaks only on '\n').
 * @return            {width, height}: widest wrapped line, line count x line height.
 *
 * Thread-safe: yes (read-only)  Reentrant: yes
 */
static inline eli_vec2 eli_font_calc_text_size_wrapped(const eli_font *font, float font_size,
                                                       const char *text_begin,
                                                       const char *text_end, float wrap_width)
{
    if (!font || !text_begin || font->font_size <= 0.0f)
        return eli_make_vec2(0.0f, 0.0f);
    if (text_end == NULL)
        text_end = text_begin + strlen(text_begin);

    float line_h = font->line_height * (font_size / font->font_size);
    float max_w = 0.0f;
    float total_h = 0.0f;
    for (const char *line = text_begin;;) {
        float w;
        const char *line_end =
            eli_font_calc_word_wrap_line(font, font_size, line, text_end, wrap_width, &w);
        if (w > max_w)
            max_w = w;
        total_h += line_h;
        if (line_end == text_end)
            break;
        line = line_end + 1;
    }
    return eli_make_vec2(max_w, total_h);
}

/* ---------------------------------------------------------------------------
 * Text rendering
 * ------------------------------------------------------------------------- */

/**
 * Emit glyph quads for one run of text starting at `pos`; '\n' returns the pen to
 * pos.x one line down. Capacity must already be reserved (one quad per byte); the
 * caller charges the returned index count to the current command.
 *
 * @return Number of indices written.
 */
static inline int eli_font__emit_glyph_run(eli_draw_list *list, const eli_font *font, float scale,
                                           float line_h, eli_vec2 pos, eli_col32 col,
                                           const char *text_begin, const char *text_end,
                                           const eli_vec4 *cpu_fine_clip_rect)
{
    float x = pos.x;
    float y = pos.y;
    int idx_written = 0;

    const char *s = text_begin;
    while (s < text_end) {
        uint32_t cp;
        s += eli_text_utf8_decode(s, text_end, &cp);
        if (cp == '\n') {
            x = pos.x;
            y += line_h;
            continue;
        }
        if (cp == '\r')
            continue;
        const eli_font_glyph *g = eli_font_find_glyph(font, cp);
        if (!g)
            continue;
        if (g->visible) {
            float x0 = x + g->x0 * scale;
            float y0 = y + g->y0 * scale;
            float x1 = x + g->x1 * scale;
            float y1 = y + g->y1 * scale;
            bool skip = false;
            if (cpu_fine_clip_rect) {
                const eli_vec4 *c = cpu_fine_clip_rect;
                if (x1 < c->x || x0 > c->z || y1 < c->y || y0 > c->w)
                    skip = true;
            }
            if (!skip) {
                eli_draw_list_prim_rect_uv(list, eli_make_vec2(x0, y0), eli_make_vec2(x1, y1),
                                           eli_make_vec2(g->u0, g->v0), eli_make_vec2(g->u1, g->v1),
                                           col);
                idx_written += 6;
            }
        }
        x += g->advance_x * scale;
    }
    return idx_written;
}

/**
 * Append a UTF-8 string's glyph quads to a draw list. The caller is responsible
 * for having the font atlas texture bound on the draw list.
 *
 * @param list       Target draw list.
 * @param font       Baked font (no-op if NULL).
 * @param font_size  Rendering size in pixels.
 * @param pos        Top-left pen position of the first line.
 * @param col        Text color (transparent is a no-op).
 * @param text_begin Start of text.
 * @param text_end   End of text, or NULL for strlen.
 * @param wrap_width Word-wrap width in pixels; <= 0 breaks only on '\n'. Lines
 *                   break as eli_font_calc_word_wrap_line decides and stack one
 *                   font line height apart (measure with eli_font_calc_text_size_wrapped).
 * @param cpu_fine_clip_rect Optional (min.x,min.y,max.x,max.y); glyphs fully
 *                   outside are skipped. NULL disables the extra test.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_draw_list_add_text_ex(eli_draw_list *list, const eli_font *font,
                                             float font_size, eli_vec2 pos, eli_col32 col,
                                             const char *text_begin, const char *text_end,
                                             float wrap_width, const eli_vec4 *cpu_fine_clip_rect)
{
    if (!list || !font || !text_begin || (col & ELI_COL32_A_MASK) == 0)
        return;
    if (text_end == NULL)
        text_end = text_begin + strlen(text_begin);

    float scale = font_size / font->font_size;
    float line_h = font->line_height * scale;

    /* Batch the geometry reservation for the whole run: at most one quad (4
     * vertices, 6 indices) per input byte, since a UTF-8 sequence spans >= 1
     * byte and newlines/carriage-returns/invisible glyphs emit nothing. A single
     * up-front capacity reserve amortizes the per-glyph reallocation probing; the
     * exact index total is charged to the command once at the end, so elem_count
     * and the vtx/idx counts land exactly where the per-glyph path would have. */
    int byte_count = (int)(text_end - text_begin);
    if (!eli_draw_list_prim_reserve_capacity(list, byte_count * 6, byte_count * 4))
        return;

    int idx_written = 0;
    if (wrap_width <= 0.0f) {
        idx_written = eli_font__emit_glyph_run(list, font, scale, line_h, pos, col, text_begin,
                                               text_end, cpu_fine_clip_rect);
    } else {
        eli_vec2 line_pos = pos;
        for (const char *line = text_begin;;) {
            float line_w;
            const char *line_end = eli_font_calc_word_wrap_line(font, font_size, line, text_end,
                                                                wrap_width, &line_w);
            idx_written += eli_font__emit_glyph_run(list, font, scale, line_h, line_pos, col, line,
                                                    line_end, cpu_fine_clip_rect);
            if (line_end == text_end)
                break;
            line = line_end + 1;
            line_pos.y += line_h;
        }
    }

    /* Charge exactly the emitted indices to the current command. */
    eli_draw_list_prim_add_idx_to_cmd(list, idx_written);
}

/**
 * Append text using the current context's font and font size.
 *
 * @param list       Target draw list.
 * @param pos        Top-left pen position.
 * @param col        Text color.
 * @param text_begin Start of text.
 * @param text_end   End of text, or NULL for strlen.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_draw_list_add_text(eli_draw_list *list, eli_vec2 pos, eli_col32 col,
                                          const char *text_begin, const char *text_end)
{
    eli_context *ctx = eli_get_current_context();
    if (!ctx || !ctx->font)
        return;
    eli_draw_list_add_text_ex(list, ctx->font, ctx->font_size, pos, col, text_begin, text_end,
                              0.0f, NULL);
}

/**
 * Measure text with the current context font/size.
 *
 * @param text_begin Start of text.
 * @param text_end   End of text, or NULL for strlen.
 * @return           {width, height}, or {0,0} if no current font.
 *
 * Thread-safe: no  Reentrant: yes
 */
static inline eli_vec2 eli_calc_text_size(const char *text_begin, const char *text_end)
{
    eli_context *ctx = eli_get_current_context();
    if (!ctx || !ctx->font)
        return eli_make_vec2(0.0f, 0.0f);
    return eli_font_calc_text_size(ctx->font, ctx->font_size, text_begin, text_end);
}

/* ---------------------------------------------------------------------------
 * Font stack
 * ------------------------------------------------------------------------- */

/** Update the context's derived font size from the active font. */
static inline void eli_font__apply_current(eli_context *ctx)
{
    if (ctx->font) {
        ctx->font_base_size = ctx->font->font_size;
        ctx->font_size = ctx->font->font_size * ctx->io.font_global_scale;
    }
}

/**
 * Push a font as the current font, saving the previous one on the stack.
 *
 * @param font  Font to make current (NULL is accepted and restores nothing).
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_push_font(eli_font *font)
{
    eli_context *ctx = eli_get_current_context();
    if (!ctx || ctx->font_stack_size >= ELI_FONT_STACK_MAX)
        return;
    ctx->font_stack[ctx->font_stack_size++] = ctx->font;
    ctx->font = font;
    eli_font__apply_current(ctx);
}

/**
 * Restore the font saved by the matching eli_push_font.
 *
 * Thread-safe: no  Reentrant: no
 */
static inline void eli_pop_font(void)
{
    eli_context *ctx = eli_get_current_context();
    if (!ctx || ctx->font_stack_size <= 0)
        return;
    ctx->font = ctx->font_stack[--ctx->font_stack_size];
    eli_font__apply_current(ctx);
}

/** @return The current context font, or NULL. */
static inline eli_font *eli_get_font(void)
{
    eli_context *ctx = eli_get_current_context();
    return ctx ? ctx->font : NULL;
}

/** @return The current context font size in pixels, or 0. */
static inline float eli_get_font_size(void)
{
    eli_context *ctx = eli_get_current_context();
    return ctx ? ctx->font_size : 0.0f;
}

/** @return The current font atlas's white-pixel UV, or (0,0) if unavailable. */
static inline eli_vec2 eli_get_font_tex_uv_white_pixel(void)
{
    eli_context *ctx = eli_get_current_context();
    if (ctx && ctx->font && ctx->font->container_atlas)
        return ctx->font->container_atlas->tex_uv_white_pixel;
    return eli_make_vec2(0.0f, 0.0f);
}

#endif /* ELI_FONT_ELI_FONT_TEXT_H */
